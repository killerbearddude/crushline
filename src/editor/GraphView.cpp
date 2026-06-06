#include "editor/GraphView.h"

#include "graph/GraphDocument.h"
#include "renderer/Renderer2D.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace editor
{
Vec2 ScreenToCanvas(Vec2 screen, const GraphViewState& view, Rect canvasRect)
{
    const Vec2 local = {
        screen.x - canvasRect.x,
        screen.y - canvasRect.y
    };

    return {
        (local.x - view.cameraOffset.x) / view.zoom,
        (local.y - view.cameraOffset.y) / view.zoom
    };
}

Vec2 CanvasToScreen(Vec2 canvas, const GraphViewState& view, Rect canvasRect)
{
    return {
        canvasRect.x + canvas.x * view.zoom + view.cameraOffset.x,
        canvasRect.y + canvas.y * view.zoom + view.cameraOffset.y
    };
}

Rect CanvasToScreen(Rect rect, const GraphViewState& view, Rect canvasRect)
{
    const Vec2 position = CanvasToScreen(Vec2{rect.x, rect.y}, view, canvasRect);
    return {
        position.x,
        position.y,
        rect.w * view.zoom,
        rect.h * view.zoom
    };
}

namespace
{

Color ResourceColor(graph::ResourceType resource, const UiTheme& theme)
{
    switch (resource)
    {
        case graph::ResourceType::IronOre:
        case graph::ResourceType::CrushedOre:
        case graph::ResourceType::WashedOre:
        case graph::ResourceType::IronIngot:
            return theme.accentCyan;

        case graph::ResourceType::Slurry:
        case graph::ResourceType::Tailings:
        case graph::ResourceType::Waste:
            return theme.accentAmber;

        case graph::ResourceType::Power:
            return theme.accentRed;

        default:
            return theme.accentGreen;
    }
}

Color NodeAccent(const graph::GraphNode& node, const UiTheme& theme)
{
    if (node.warning)
    {
        return theme.accentAmber;
    }

    switch (node.type)
    {
        case graph::NodeType::Source:
            return theme.accentGreen;
        case graph::NodeType::Storage:
            return theme.accentAmber;
        case graph::NodeType::Output:
        case graph::NodeType::Contract:
            return theme.accentCyan;
        case graph::NodeType::Warning:
            return theme.accentRed;
        default:
            return theme.accentCyan;
    }
}

Rect NodeRect(const NodeVisual& visual)
{
    return {
        visual.position.x,
        visual.position.y,
        visual.size.x,
        visual.size.y
    };
}

bool RectFullyInside(Rect inner, Rect outer)
{
    return
        inner.x >= outer.x &&
        inner.y >= outer.y &&
        inner.x + inner.w <= outer.x + outer.w &&
        inner.y + inner.h <= outer.y + outer.h;
}

bool IsNodeFullyInsideCanvas(const NodeVisual& visual, const GraphViewState& view, Rect canvasRect)
{
    return RectFullyInside(CanvasToScreen(NodeRect(visual), view, canvasRect), canvasRect);
}

float Distance(Vec2 a, Vec2 b)
{
    const Vec2 delta = {a.x - b.x, a.y - b.y};
    return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}

void ClearHoveredPort(GraphViewState& view)
{
    view.hoveredPortNodeId = -1;
    view.hoveredPortId = -1;
}

void ClearHoveredEdge(GraphViewState& view)
{
    view.hoveredEdgeId = -1;
}

void SyncHoveredPort(GraphViewState& view, PortHit hit)
{
    view.hoveredPortNodeId = hit.nodeId;
    view.hoveredPortId = hit.portId;
}

void ClearWireDrag(GraphViewState& view)
{
    view.draggingWire = false;
    view.wireStartNodeId = -1;
    view.wireStartPortId = -1;
}

bool IsOutputPort(const graph::GraphDocument& graph, int nodeId, int portId)
{
    const graph::GraphPort* port = graph::FindPort(graph, nodeId, portId);
    return port != nullptr && port->direction == graph::PortDirection::Output;
}


WireDropFailureReason ClassifyWireDropFailure(
    const graph::GraphDocument& graph,
    int startNodeId,
    int startPortId,
    int targetNodeId,
    int targetPortId
)
{
    const graph::GraphPort* startPort = graph::FindPort(graph, startNodeId, startPortId);

    if (startPort == nullptr)
    {
        return WireDropFailureReason::MissingEndpoint;
    }

    if (targetNodeId < 0 || targetPortId < 0)
    {
        return WireDropFailureReason::EmptyTarget;
    }

    const graph::GraphPort* targetPort = graph::FindPort(graph, targetNodeId, targetPortId);

    if (targetPort == nullptr)
    {
        return WireDropFailureReason::MissingEndpoint;
    }

    if (startNodeId == targetNodeId)
    {
        return WireDropFailureReason::SelfConnection;
    }

    if (startPort->direction != graph::PortDirection::Output ||
        targetPort->direction != graph::PortDirection::Input)
    {
        return WireDropFailureReason::InvalidDirection;
    }

    if (startPort->resource != targetPort->resource)
    {
        return WireDropFailureReason::ResourceMismatch;
    }

    const auto duplicateIt = std::find_if(
        graph.edges.begin(),
        graph.edges.end(),
        [startNodeId, startPortId, targetNodeId, targetPortId](const graph::GraphEdge& edge) {
            return
                edge.fromNodeId == startNodeId &&
                edge.fromPortId == startPortId &&
                edge.toNodeId == targetNodeId &&
                edge.toPortId == targetPortId;
        }
    );

    if (duplicateIt != graph.edges.end())
    {
        return WireDropFailureReason::DuplicateConnection;
    }

    return WireDropFailureReason::InvalidTarget;
}

Color WirePreviewColor(const graph::GraphDocument& graph, const GraphViewState& view, const UiTheme& theme)
{
    if (view.hoveredPortNodeId >= 0 && view.hoveredPortId >= 0)
    {
        const bool canConnect = graph::CanConnect(
            graph,
            view.wireStartNodeId,
            view.wireStartPortId,
            view.hoveredPortNodeId,
            view.hoveredPortId
        );

        return canConnect ? theme.wireActive : theme.accentRed;
    }

    const graph::GraphPort* startPort = graph::FindPort(graph, view.wireStartNodeId, view.wireStartPortId);
    return startPort == nullptr ? theme.wireActive : ResourceColor(startPort->resource, theme);
}

void SyncSelectedNodeVisuals(GraphViewState& view)
{
    for (auto& [nodeId, visual] : view.nodeVisuals)
    {
        visual.selected = nodeId == view.selectedNodeId;
    }
}

bool GraphHasNode(const graph::GraphDocument& graph, int nodeId)
{
    return graph::FindNode(graph, nodeId) != nullptr;
}

void ClearGraphSelectionAndHover(GraphViewState& view)
{
    view.selectedNodeId = -1;
    view.hoveredNodeId = -1;
    view.draggingNodeId = -1;

    view.selectedEdgeId = -1;
    ClearHoveredEdge(view);
    ClearHoveredPort(view);
    ClearWireDrag(view);
}

bool DeleteSelectedNode(GraphViewState& view, graph::GraphDocument& graph)
{
    if (view.selectedNodeId < 0 || !GraphHasNode(graph, view.selectedNodeId))
    {
        return false;
    }

    const int nodeId = view.selectedNodeId;

    graph::RemoveNode(graph, nodeId);
    view.nodeVisuals.erase(nodeId);

    ClearGraphSelectionAndHover(view);
    SyncSelectedNodeVisuals(view);
    return true;
}

int PortIndex(const graph::GraphNode& node, int portId)
{
    for (int i = 0; i < static_cast<int>(node.inputs.size()); ++i)
    {
        if (node.inputs[static_cast<std::size_t>(i)].id == portId)
        {
            return i;
        }
    }

    for (int i = 0; i < static_cast<int>(node.outputs.size()); ++i)
    {
        if (node.outputs[static_cast<std::size_t>(i)].id == portId)
        {
            return i;
        }
    }

    return 0;
}

Vec2 PortPosition(const NodeVisual& visual, const graph::GraphPort& port, int index)
{
    const float y = visual.position.y + 30.0f + static_cast<float>(index) * 16.0f;

    if (port.direction == graph::PortDirection::Input)
    {
        return {visual.position.x, y};
    }

    return {visual.position.x + visual.size.x, y};
}

struct CubicBezier
{
    Vec2 p0{};
    Vec2 p1{};
    Vec2 p2{};
    Vec2 p3{};
};

CubicBezier MakeWireBezier(Vec2 from, Vec2 to)
{
    const float dx = std::abs(to.x - from.x);
    const float handle = std::max(56.0f, dx * 0.45f);

    return {
        from,
        {from.x + handle, from.y},
        {to.x - handle, to.y},
        to
    };
}

Vec2 EvaluateBezier(const CubicBezier& bezier, float t)
{
    const float u = 1.0f - t;

    const float a = u * u * u;
    const float b = 3.0f * u * u * t;
    const float c = 3.0f * u * t * t;
    const float d = t * t * t;

    return {
        a * bezier.p0.x + b * bezier.p1.x + c * bezier.p2.x + d * bezier.p3.x,
        a * bezier.p0.y + b * bezier.p1.y + c * bezier.p2.y + d * bezier.p3.y
    };
}

float DistanceToSegment(Vec2 point, Vec2 a, Vec2 b)
{
    const Vec2 ab = {b.x - a.x, b.y - a.y};
    const Vec2 ap = {point.x - a.x, point.y - a.y};
    const float lengthSquared = ab.x * ab.x + ab.y * ab.y;

    if (lengthSquared <= 0.0001f)
    {
        return Distance(point, a);
    }

    const float t = std::clamp((ap.x * ab.x + ap.y * ab.y) / lengthSquared, 0.0f, 1.0f);
    const Vec2 closest = {a.x + ab.x * t, a.y + ab.y * t};
    return Distance(point, closest);
}

bool GetEdgeEndpoints(
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    const graph::GraphEdge& edge,
    Rect canvasRect,
    Vec2& from,
    Vec2& to
)
{
    const graph::GraphNode* fromNode = graph::FindNode(graph, edge.fromNodeId);
    const graph::GraphNode* toNode = graph::FindNode(graph, edge.toNodeId);
    const graph::GraphPort* fromPort = graph::FindPort(graph, edge.fromNodeId, edge.fromPortId);
    const graph::GraphPort* toPort = graph::FindPort(graph, edge.toNodeId, edge.toPortId);

    if (fromNode == nullptr || toNode == nullptr || fromPort == nullptr || toPort == nullptr)
    {
        return false;
    }

    const auto fromVisualIt = view.nodeVisuals.find(fromNode->id);
    const auto toVisualIt = view.nodeVisuals.find(toNode->id);

    if (fromVisualIt == view.nodeVisuals.end() || toVisualIt == view.nodeVisuals.end())
    {
        return false;
    }

    if (!IsNodeFullyInsideCanvas(fromVisualIt->second, view, canvasRect) ||
        !IsNodeFullyInsideCanvas(toVisualIt->second, view, canvasRect))
    {
        return false;
    }

    const int fromIndex = PortIndex(*fromNode, fromPort->id);
    const int toIndex = PortIndex(*toNode, toPort->id);

    from = CanvasToScreen(PortPosition(fromVisualIt->second, *fromPort, fromIndex), view, canvasRect);
    to = CanvasToScreen(PortPosition(toVisualIt->second, *toPort, toIndex), view, canvasRect);

    return true;
}

void DrawBezierApprox(Renderer2D& renderer, Vec2 from, Vec2 to, Rect clipRect, Color color, float thickness)
{
    const CubicBezier bezier = MakeWireBezier(from, to);

    Vec2 previous = bezier.p0;

    constexpr int segments = 20;
    for (int i = 1; i <= segments; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const Vec2 point = EvaluateBezier(bezier, t);

        if (clipRect.Contains(previous) && clipRect.Contains(point))
        {
            renderer.DrawLine(previous, point, color, thickness);
        }

        previous = point;
    }
}

void DrawNode(Renderer2D& renderer, const graph::GraphNode& node, const NodeVisual& visual, const GraphViewState& view, Rect canvasRect, const UiTheme& theme)
{
    const Rect canvasNode = {
        visual.position.x,
        visual.position.y,
        visual.size.x,
        visual.size.y
    };

    const Rect nodeRect = CanvasToScreen(canvasNode, view, canvasRect);
    const Rect header = {nodeRect.x, nodeRect.y, nodeRect.w, 24.0f * view.zoom};
    const Color accent = NodeAccent(node, theme);

    const bool hovered = view.hoveredNodeId == node.id;
    const Color borderColor = visual.selected
        ? theme.nodeSelected
        : hovered ? theme.accentCyan : theme.nodeBorder;
    const float borderThickness = visual.selected ? 2.0f : hovered ? 1.5f : 1.0f;

    renderer.DrawRect(nodeRect, hovered ? theme.panelAlt : theme.nodeBody);
    renderer.DrawRect(header, theme.nodeHeader);
    renderer.DrawRectOutline(nodeRect, borderColor, borderThickness);
    renderer.DrawRect({nodeRect.x + 8.0f, nodeRect.y + 8.0f, 4.0f, 9.0f}, accent);
    renderer.DrawText({nodeRect.x + 18.0f, nodeRect.y + 5.0f}, node.name, theme.textPrimary);

    if (hovered && !visual.selected)
    {
        renderer.DrawLine(
            {nodeRect.x + 10.0f, nodeRect.y + nodeRect.h - 6.0f},
            {nodeRect.x + nodeRect.w - 10.0f, nodeRect.y + nodeRect.h - 6.0f},
            theme.accentCyan,
            1.0f
        );
    }

    renderer.DrawText({nodeRect.x + 18.0f, nodeRect.y + 30.0f * view.zoom}, node.inputs.empty() ? "SOURCE" : "MACHINE", theme.textMuted);

    const float fill = node.capacity > 0.0f ? std::min(node.throughput / node.capacity, 1.0f) : node.efficiency;
    const Rect barTrack = {nodeRect.x + 12.0f, nodeRect.y + nodeRect.h - 12.0f, nodeRect.w - 24.0f, 3.0f};
    renderer.DrawRect(barTrack, theme.panelHighlight);
    renderer.DrawRect({barTrack.x, barTrack.y, barTrack.w * fill, barTrack.h}, accent);

    if (node.warning)
    {
        renderer.DrawCircle({nodeRect.x + nodeRect.w - 12.0f, nodeRect.y + 12.0f}, 4.0f, theme.accentAmber);
    }

    for (int i = 0; i < static_cast<int>(node.inputs.size()); ++i)
    {
        const graph::GraphPort& port = node.inputs[static_cast<std::size_t>(i)];
        const Vec2 p = CanvasToScreen(PortPosition(visual, port, i), view, canvasRect);
        const bool hoveredPort = view.hoveredPortNodeId == node.id && view.hoveredPortId == port.id;
        const bool dragTarget = view.draggingWire && hoveredPort;

        if (hoveredPort)
        {
            renderer.DrawCircle(p, dragTarget ? 8.0f : 7.0f, dragTarget ? theme.wireActive : theme.panelHighlight);
        }

        renderer.DrawCircle(p, hoveredPort ? 5.0f : 4.0f, ResourceColor(port.resource, theme));
        renderer.DrawLine({p.x + 8.0f, p.y}, {p.x + 34.0f, p.y}, hoveredPort ? theme.wireActive : theme.panelHighlight, 1.0f);
    }

    for (int i = 0; i < static_cast<int>(node.outputs.size()); ++i)
    {
        const graph::GraphPort& port = node.outputs[static_cast<std::size_t>(i)];
        const Vec2 p = CanvasToScreen(PortPosition(visual, port, i), view, canvasRect);
        const bool hoveredPort = view.hoveredPortNodeId == node.id && view.hoveredPortId == port.id;
        const bool dragSource = view.draggingWire && view.wireStartNodeId == node.id && view.wireStartPortId == port.id;

        if (hoveredPort || dragSource)
        {
            renderer.DrawCircle(p, dragSource ? 8.0f : 7.0f, dragSource ? theme.wireActive : theme.panelHighlight);
        }

        renderer.DrawCircle(p, (hoveredPort || dragSource) ? 5.0f : 4.0f, ResourceColor(port.resource, theme));
        renderer.DrawLine({p.x - 34.0f, p.y}, {p.x - 8.0f, p.y}, (hoveredPort || dragSource) ? theme.wireActive : theme.panelHighlight, 1.0f);
    }
}
}

GraphViewState CreateSampleFactoryGraphView(const graph::GraphDocument& graph)
{
    GraphViewState view;

    const Vec2 positions[] = {
        {22.0f, 42.0f},
        {190.0f, 42.0f},
        {358.0f, 42.0f},
        {550.0f, 42.0f},
        {724.0f, 42.0f},
        {358.0f, 158.0f},
        {550.0f, 220.0f}
    };

    for (std::size_t i = 0; i < graph.nodes.size(); ++i)
    {
        const graph::GraphNode& node = graph.nodes[i];
        const Vec2 position = i < std::size(positions)
            ? positions[i]
            : Vec2{22.0f + static_cast<float>(i) * 42.0f, 42.0f};

        view.nodeVisuals[node.id] = NodeVisual{
            .nodeId = node.id,
            .position = position,
            .size = node.outputs.size() > 1 ? Vec2{136.0f, 82.0f} : Vec2{128.0f, 68.0f},
            .selected = node.warning
        };

        if (node.warning && view.selectedNodeId < 0)
        {
            view.selectedNodeId = node.id;
        }
    }

    return view;
}

void EnsureNodeVisuals(GraphViewState& view, const graph::GraphDocument& graph)
{
    for (auto it = view.nodeVisuals.begin(); it != view.nodeVisuals.end();)
    {
        if (GraphHasNode(graph, it->first))
        {
            ++it;
        }
        else
        {
            it = view.nodeVisuals.erase(it);
        }
    }

    if (view.selectedNodeId >= 0 && !GraphHasNode(graph, view.selectedNodeId))
    {
        view.selectedNodeId = -1;
    }

    if (view.hoveredNodeId >= 0 && !GraphHasNode(graph, view.hoveredNodeId))
    {
        view.hoveredNodeId = -1;
    }

    if (view.draggingNodeId >= 0 && !GraphHasNode(graph, view.draggingNodeId))
    {
        view.draggingNodeId = -1;
    }

    for (const graph::GraphNode& node : graph.nodes)
    {
        if (!view.nodeVisuals.contains(node.id))
        {
            view.nodeVisuals[node.id] = NodeVisual{
                .nodeId = node.id,
                .position = {32.0f + static_cast<float>(view.nodeVisuals.size()) * 24.0f, 64.0f},
                .size = {128.0f, 68.0f},
                .selected = false
            };
        }
    }

    SyncSelectedNodeVisuals(view);
}

int HitTestNode(
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    Vec2 canvasPosition,
    Rect canvasRect
)
{
    for (auto it = graph.nodes.rbegin(); it != graph.nodes.rend(); ++it)
    {
        const graph::GraphNode& node = *it;
        const auto visualIt = view.nodeVisuals.find(node.id);

        if (visualIt == view.nodeVisuals.end())
        {
            continue;
        }

        if (!IsNodeFullyInsideCanvas(visualIt->second, view, canvasRect))
        {
            continue;
        }

        if (NodeRect(visualIt->second).Contains(canvasPosition))
        {
            return node.id;
        }
    }

    return -1;
}

PortHit HitTestPort(
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    Vec2 canvasPosition,
    Rect canvasRect
)
{
    const float radius = 8.0f / std::max(view.zoom, 0.0001f);

    for (auto it = graph.nodes.rbegin(); it != graph.nodes.rend(); ++it)
    {
        const graph::GraphNode& node = *it;
        const auto visualIt = view.nodeVisuals.find(node.id);

        if (visualIt == view.nodeVisuals.end())
        {
            continue;
        }

        const NodeVisual& visual = visualIt->second;

        if (!IsNodeFullyInsideCanvas(visual, view, canvasRect))
        {
            continue;
        }

        for (int i = 0; i < static_cast<int>(node.outputs.size()); ++i)
        {
            const graph::GraphPort& port = node.outputs[static_cast<std::size_t>(i)];
            const Vec2 position = PortPosition(visual, port, i);

            if (Distance(position, canvasPosition) <= radius)
            {
                return {node.id, port.id};
            }
        }

        for (int i = 0; i < static_cast<int>(node.inputs.size()); ++i)
        {
            const graph::GraphPort& port = node.inputs[static_cast<std::size_t>(i)];
            const Vec2 position = PortPosition(visual, port, i);

            if (Distance(position, canvasPosition) <= radius)
            {
                return {node.id, port.id};
            }
        }
    }

    return {};
}

int HitTestEdge(
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    Vec2 screenPosition,
    Rect canvasRect
)
{
    if (!canvasRect.Contains(screenPosition))
    {
        return -1;
    }

    const float tolerance = 7.0f;

    for (auto it = graph.edges.rbegin(); it != graph.edges.rend(); ++it)
    {
        Vec2 from{};
        Vec2 to{};

        if (!GetEdgeEndpoints(graph, view, *it, canvasRect, from, to))
        {
            continue;
        }

        const CubicBezier bezier = MakeWireBezier(from, to);
        Vec2 previous = bezier.p0;

        constexpr int segments = 20;
        for (int i = 1; i <= segments; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const Vec2 point = EvaluateBezier(bezier, t);

            if (canvasRect.Contains(previous) &&
                canvasRect.Contains(point) &&
                DistanceToSegment(screenPosition, previous, point) <= tolerance)
            {
                return it->id;
            }

            previous = point;
        }
    }

    return -1;
}

bool UpdateGraphViewInteraction(
    GraphViewState& view,
    graph::GraphDocument& graph,
    const InputState& input,
    Rect canvasRect
)
{
    Vec2 canvasMouse = ScreenToCanvas(input.mousePosition, view, canvasRect);
    view.lastWireDropFailure = WireDropFailureReason::None;

    if (input.keyDeletePressed && view.selectedEdgeId >= 0)
    {
        const std::size_t previousEdgeCount = graph.edges.size();
        graph::RemoveEdge(graph, view.selectedEdgeId);
        view.selectedEdgeId = -1;
        view.hoveredEdgeId = -1;
        SyncSelectedNodeVisuals(view);
        return graph.edges.size() != previousEdgeCount;
    }

    if (input.keyDeletePressed && view.selectedNodeId >= 0)
    {
        return DeleteSelectedNode(view, graph);
    }

    if (view.panningCanvas)
    {
        if (input.middleMouseDown)
        {
            const Vec2 delta = {
                input.mousePosition.x - view.panStartMousePosition.x,
                input.mousePosition.y - view.panStartMousePosition.y
            };

            view.cameraOffset = {
                view.panStartCameraOffset.x + delta.x,
                view.panStartCameraOffset.y + delta.y
            };

            view.hoveredNodeId = -1;
            ClearHoveredPort(view);
            ClearHoveredEdge(view);
            SyncSelectedNodeVisuals(view);
            return false;
        }

        view.panningCanvas = false;
    }

    if (view.draggingNodeId >= 0)
    {
        if (input.leftMouseDown)
        {
            auto visualIt = view.nodeVisuals.find(view.draggingNodeId);

            if (visualIt != view.nodeVisuals.end())
            {
                const Vec2 delta = {
                    canvasMouse.x - view.dragStartCanvasPosition.x,
                    canvasMouse.y - view.dragStartCanvasPosition.y
                };

                visualIt->second.position = {
                    view.dragStartNodePosition.x + delta.x,
                    view.dragStartNodePosition.y + delta.y
                };
            }

            view.hoveredNodeId = view.draggingNodeId;
            ClearHoveredPort(view);
            SyncSelectedNodeVisuals(view);
            return false;
        }

        view.draggingNodeId = -1;
    }

    if (view.draggingWire)
    {
        view.wirePreviewCanvasPosition = canvasMouse;

        if (canvasRect.Contains(input.mousePosition))
        {
            SyncHoveredPort(view, HitTestPort(graph, view, canvasMouse, canvasRect));
        }
        else
        {
            ClearHoveredPort(view);
        }

        if (input.leftMouseDown)
        {
            view.hoveredNodeId = -1;
            SyncSelectedNodeVisuals(view);
            return false;
        }

        bool graphChanged = false;

        if (input.leftMouseReleased)
        {
            const bool canConnect =
                view.hoveredPortNodeId >= 0 &&
                view.hoveredPortId >= 0 &&
                graph::CanConnect(
                    graph,
                    view.wireStartNodeId,
                    view.wireStartPortId,
                    view.hoveredPortNodeId,
                    view.hoveredPortId
                );

            if (canConnect)
            {
                const int edgeId = graph::AddEdge(
                    graph,
                    view.wireStartNodeId,
                    view.wireStartPortId,
                    view.hoveredPortNodeId,
                    view.hoveredPortId
                );

                if (edgeId > 0)
                {
                    view.selectedEdgeId = edgeId;
                    view.selectedNodeId = -1;
                    graphChanged = true;
                }
            }
            else
            {
                view.lastWireDropFailure = ClassifyWireDropFailure(
                    graph,
                    view.wireStartNodeId,
                    view.wireStartPortId,
                    view.hoveredPortNodeId,
                    view.hoveredPortId
                );
            }
        }

        view.hoveredNodeId = -1;
        ClearHoveredEdge(view);
        ClearWireDrag(view);
        ClearHoveredPort(view);
        SyncSelectedNodeVisuals(view);
        return graphChanged;
    }

    if (!canvasRect.Contains(input.mousePosition))
    {
        view.hoveredNodeId = -1;
        ClearHoveredPort(view);
        SyncSelectedNodeVisuals(view);
        return false;
    }

    if (std::abs(input.mouseWheelDelta) > 0.0f)
    {
        const Vec2 anchorCanvas = canvasMouse;
        const float previousZoom = view.zoom;
        const float zoomFactor = std::pow(1.12f, input.mouseWheelDelta);
        view.zoom = std::clamp(previousZoom * zoomFactor, view.minZoom, view.maxZoom);

        if (view.zoom != previousZoom)
        {
            const Vec2 localMouse = {
                input.mousePosition.x - canvasRect.x,
                input.mousePosition.y - canvasRect.y
            };

            view.cameraOffset = {
                localMouse.x - anchorCanvas.x * view.zoom,
                localMouse.y - anchorCanvas.y * view.zoom
            };

            canvasMouse = ScreenToCanvas(input.mousePosition, view, canvasRect);
        }
    }

    if (input.middleMousePressed)
    {
        view.panningCanvas = true;
        view.panStartMousePosition = input.mousePosition;
        view.panStartCameraOffset = view.cameraOffset;
        view.hoveredNodeId = -1;
        ClearHoveredPort(view);
        ClearHoveredEdge(view);
        SyncSelectedNodeVisuals(view);
        return false;
    }

    const PortHit portHit = HitTestPort(graph, view, canvasMouse, canvasRect);
    SyncHoveredPort(view, portHit);
    view.hoveredNodeId = HitTestNode(graph, view, canvasMouse, canvasRect);
    view.hoveredEdgeId = (view.hoveredPortId < 0 && view.hoveredNodeId < 0)
        ? HitTestEdge(graph, view, input.mousePosition, canvasRect)
        : -1;

    if (input.leftMousePressed)
    {
        if (view.hoveredPortNodeId >= 0 &&
            view.hoveredPortId >= 0 &&
            IsOutputPort(graph, view.hoveredPortNodeId, view.hoveredPortId))
        {
            view.draggingWire = true;
            view.wireStartNodeId = view.hoveredPortNodeId;
            view.wireStartPortId = view.hoveredPortId;
            view.wirePreviewCanvasPosition = canvasMouse;
            view.draggingNodeId = -1;
            view.selectedEdgeId = -1;
        }
        else if (view.hoveredNodeId >= 0)
        {
            view.selectedNodeId = view.hoveredNodeId;
            view.selectedEdgeId = -1;
            view.draggingNodeId = view.hoveredNodeId;
            view.dragStartCanvasPosition = canvasMouse;

            const auto visualIt = view.nodeVisuals.find(view.hoveredNodeId);
            if (visualIt != view.nodeVisuals.end())
            {
                view.dragStartNodePosition = visualIt->second.position;
            }
        }
        else if (view.hoveredEdgeId >= 0)
        {
            view.selectedNodeId = -1;
            view.selectedEdgeId = view.hoveredEdgeId;
            view.draggingNodeId = -1;
        }
        else
        {
            view.selectedNodeId = -1;
            view.selectedEdgeId = -1;
            view.draggingNodeId = -1;
        }
    }

    SyncSelectedNodeVisuals(view);
    return false;
}

void DrawGraphView(
    Renderer2D& renderer,
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    Rect canvasRect,
    const UiTheme& theme
)
{
    for (const graph::GraphEdge& edge : graph.edges)
    {
        const graph::GraphPort* fromPort = graph::FindPort(graph, edge.fromNodeId, edge.fromPortId);

        if (fromPort == nullptr)
        {
            continue;
        }

        Vec2 from{};
        Vec2 to{};

        if (!GetEdgeEndpoints(graph, view, edge, canvasRect, from, to))
        {
            continue;
        }

        const bool selected = edge.id == view.selectedEdgeId;
        const bool hovered = edge.id == view.hoveredEdgeId;
        const Color color = selected || hovered
            ? theme.wireActive
            : ResourceColor(fromPort->resource, theme);
        const float thickness = selected ? 3.0f : hovered ? 2.5f : 2.0f;

        DrawBezierApprox(renderer, from, to, canvasRect, color, thickness);

        if (selected)
        {
            const CubicBezier bezier = MakeWireBezier(from, to);
            renderer.DrawCircle(EvaluateBezier(bezier, 0.5f), 4.0f, theme.wireActive);
        }
    }

    if (view.draggingWire)
    {
        const graph::GraphNode* startNode = graph::FindNode(graph, view.wireStartNodeId);
        const graph::GraphPort* startPort = graph::FindPort(graph, view.wireStartNodeId, view.wireStartPortId);

        if (startNode != nullptr && startPort != nullptr)
        {
            const auto startVisualIt = view.nodeVisuals.find(startNode->id);

            if (startVisualIt != view.nodeVisuals.end() &&
                IsNodeFullyInsideCanvas(startVisualIt->second, view, canvasRect))
            {
                const int startIndex = PortIndex(*startNode, startPort->id);
                const Vec2 from = CanvasToScreen(
                    PortPosition(startVisualIt->second, *startPort, startIndex),
                    view,
                    canvasRect
                );
                const Vec2 to = CanvasToScreen(view.wirePreviewCanvasPosition, view, canvasRect);

                DrawBezierApprox(
                    renderer,
                    from,
                    to,
                    canvasRect,
                    WirePreviewColor(graph, view, theme),
                    2.5f
                );

                if (canvasRect.Contains(to))
                {
                    renderer.DrawCircle(to, 4.0f, theme.wireActive);
                }
            }
        }
    }

    for (const graph::GraphNode& node : graph.nodes)
    {
        const auto visualIt = view.nodeVisuals.find(node.id);

        if (visualIt == view.nodeVisuals.end())
        {
            continue;
        }

        if (!IsNodeFullyInsideCanvas(visualIt->second, view, canvasRect))
        {
            continue;
        }

        DrawNode(renderer, node, visualIt->second, view, canvasRect, theme);
    }
}

} // namespace editor

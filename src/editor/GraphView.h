#pragma once

#include "core/Math.h"
#include "core/Rect.h"
#include "graph/GraphTypes.h"
#include "platform/Input.h"
#include "ui/Theme.h"

#include <unordered_map>

class Renderer2D;

namespace editor
{

struct NodeVisual
{
    int nodeId = 0;
    Vec2 position{};
    Vec2 size = {128.0f, 64.0f};
    bool selected = false;
};

struct GraphViewState
{
    Vec2 cameraOffset = {0.0f, 0.0f};
    float zoom = 1.0f;
    float minZoom = 0.45f;
    float maxZoom = 2.25f;

    std::unordered_map<int, NodeVisual> nodeVisuals{};

    int selectedNodeId = -1;
    int hoveredNodeId = -1;
    int draggingNodeId = -1;

    int hoveredEdgeId = -1;
    int selectedEdgeId = -1;

    int hoveredPortNodeId = -1;
    int hoveredPortId = -1;

    bool draggingWire = false;
    int wireStartNodeId = -1;
    int wireStartPortId = -1;
    Vec2 wirePreviewCanvasPosition{};

    Vec2 dragStartCanvasPosition{};
    Vec2 dragStartNodePosition{};

    bool panningCanvas = false;
    Vec2 panStartMousePosition{};
    Vec2 panStartCameraOffset{};
};

GraphViewState CreateSampleFactoryGraphView(const graph::GraphDocument& graph);
void EnsureNodeVisuals(GraphViewState& view, const graph::GraphDocument& graph);

Vec2 ScreenToCanvas(Vec2 screen, const GraphViewState& view, Rect canvasRect);
Vec2 CanvasToScreen(Vec2 canvas, const GraphViewState& view, Rect canvasRect);
Rect CanvasToScreen(Rect rect, const GraphViewState& view, Rect canvasRect);

struct PortHit
{
    int nodeId = -1;
    int portId = -1;
};

int HitTestNode(
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    Vec2 canvasPosition,
    Rect canvasRect
);

PortHit HitTestPort(
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    Vec2 canvasPosition,
    Rect canvasRect
);

int HitTestEdge(
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    Vec2 screenPosition,
    Rect canvasRect
);

bool UpdateGraphViewInteraction(
    GraphViewState& view,
    graph::GraphDocument& graph,
    const InputState& input,
    Rect canvasRect
);

void DrawGraphView(
    Renderer2D& renderer,
    const graph::GraphDocument& graph,
    const GraphViewState& view,
    Rect canvasRect,
    const UiTheme& theme
);

} // namespace editor

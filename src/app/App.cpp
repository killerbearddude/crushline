// Implements the Crushline application loop and dashboard rendering. This file
// coordinates platform input, graph editing, legacy inspector metrics, and the
// production-evaluator data now used by the top bar, panels, and event log.

#include "app/App.h"

#include "platform/Time.h"
#include "graph/GraphDocument.h"
#include "graph/GraphSerializer.h"
#include "graph/SampleGraph.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
constexpr const char* CurrentGraphPath = "data/current_graph.json";

std::string GraphSummary(std::size_t nodeCount, std::size_t edgeCount)
{
    return std::to_string(nodeCount) + " nodes / " + std::to_string(edgeCount) + " links";
}

std::string GraphSummary(const graph::GraphDocument& graph)
{
    return GraphSummary(graph.nodes.size(), graph.edges.size());
}

const graph::GraphEdge* FindEventEdge(const graph::GraphDocument& graph, int edgeId)
{
    const auto it = std::find_if(
        graph.edges.begin(),
        graph.edges.end(),
        [edgeId](const graph::GraphEdge& edge) {
            return edge.id == edgeId;
        }
    );

    return it == graph.edges.end() ? nullptr : &(*it);
}

std::string NodePortEventLabel(const graph::GraphDocument& graph, int nodeId, int portId)
{
    const graph::GraphNode* node = graph::FindNode(graph, nodeId);
    const graph::GraphPort* port = graph::FindPort(graph, nodeId, portId);

    const std::string nodeLabel = node == nullptr
        ? "Node " + std::to_string(nodeId)
        : node->name;

    const std::string portLabel = port == nullptr
        ? "Port " + std::to_string(portId)
        : port->name;

    return nodeLabel + "." + portLabel;
}

std::string LinkEventLabel(const graph::GraphDocument& graph, const graph::GraphEdge& edge)
{
    return
        NodePortEventLabel(graph, edge.fromNodeId, edge.fromPortId) +
        " -> " +
        NodePortEventLabel(graph, edge.toNodeId, edge.toPortId);
}

void QueueGraphMutationEvents(
    std::vector<std::string>& events,
    const graph::GraphDocument& previousGraph,
    const graph::GraphDocument& graph
)
{
    const std::size_t previousNodeCount = previousGraph.nodes.size();
    const std::size_t previousEdgeCount = previousGraph.edges.size();
    const std::size_t nodeCount = graph.nodes.size();
    const std::size_t edgeCount = graph.edges.size();
    const std::string summary = GraphSummary(nodeCount, edgeCount);

    if (nodeCount < previousNodeCount)
    {
        const std::size_t removed = previousNodeCount - nodeCount;
        events.push_back(
            removed == 1
                ? "Node removed: " + summary
                : "Nodes removed: " + std::to_string(removed) + " / " + summary
        );
    }
    else if (nodeCount > previousNodeCount)
    {
        const std::size_t added = nodeCount - previousNodeCount;
        events.push_back(
            added == 1
                ? "Node added: " + summary
                : "Nodes added: " + std::to_string(added) + " / " + summary
        );
    }

    if (edgeCount < previousEdgeCount)
    {
        const std::size_t removed = previousEdgeCount - edgeCount;
        std::string removedLink;

        if (removed == 1)
        {
            for (const graph::GraphEdge& edge : previousGraph.edges)
            {
                if (FindEventEdge(graph, edge.id) == nullptr)
                {
                    removedLink = LinkEventLabel(previousGraph, edge);
                    break;
                }
            }
        }

        events.push_back(
            removed == 1 && !removedLink.empty()
                ? "Link removed: " + removedLink
                : "Links removed: " + std::to_string(removed) + " / " + summary
        );
    }
    else if (edgeCount > previousEdgeCount)
    {
        const std::size_t added = edgeCount - previousEdgeCount;
        std::string createdLink;

        if (added == 1)
        {
            for (const graph::GraphEdge& edge : graph.edges)
            {
                if (FindEventEdge(previousGraph, edge.id) == nullptr)
                {
                    createdLink = LinkEventLabel(graph, edge);
                    break;
                }
            }
        }

        events.push_back(
            added == 1 && !createdLink.empty()
                ? "Link created: " + createdLink
                : "Links created: " + std::to_string(added) + " / " + summary
        );
    }
}

void DrawPanel(Renderer2D& renderer, Rect rect, const UiTheme& theme, Color fill)
{
    renderer.DrawRect(rect, fill);
    renderer.DrawRect(TopSlice(rect, 28.0f), theme.panelHeader);
    renderer.DrawRectOutline(rect, theme.panelBorder, theme.borderThickness);
}

float GridStart(float origin, float offset, float spacing)
{
    float wrapped = std::fmod(offset, spacing);

    if (wrapped < 0.0f)
    {
        wrapped += spacing;
    }

    return origin + wrapped;
}

void DrawGrid(Renderer2D& renderer, Rect rect, const UiTheme& theme, Vec2 cameraOffset, float zoom)
{
    const float spacing = theme.gridSpacing * zoom;
    const float majorSpacing = spacing * 4.0f;
    const float right = rect.x + rect.w;
    const float bottom = rect.y + rect.h;

    for (float x = GridStart(rect.x, cameraOffset.x, spacing); x <= right; x += spacing)
    {
        renderer.DrawLine({x, rect.y}, {x, bottom}, theme.gridMinor, 1.0f);
    }

    for (float y = GridStart(rect.y, cameraOffset.y, spacing); y <= bottom; y += spacing)
    {
        renderer.DrawLine({rect.x, y}, {right, y}, theme.gridMinor, 1.0f);
    }

    for (float x = GridStart(rect.x, cameraOffset.x, majorSpacing); x <= right; x += majorSpacing)
    {
        renderer.DrawLine({x, rect.y}, {x, bottom}, theme.gridMajor, 1.0f);
    }

    for (float y = GridStart(rect.y, cameraOffset.y, majorSpacing); y <= bottom; y += majorSpacing)
    {
        renderer.DrawLine({rect.x, y}, {right, y}, theme.gridMajor, 1.0f);
    }

    renderer.DrawRectOutline(rect, theme.gridMajor, 1.0f);
    renderer.DrawLine({rect.x, rect.y}, {rect.x + majorSpacing, rect.y}, theme.gridMajor, 1.0f);
}

float Clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

Color Mix(Color a, Color b, float t)
{
    const float u = Clamp01(t);

    return {
        a.r + (b.r - a.r) * u,
        a.g + (b.g - a.g) * u,
        a.b + (b.b - a.b) * u,
        a.a + (b.a - a.a) * u
    };
}

Color AlertColor(const UiTheme& theme, float value)
{
    const float v = Clamp01(value);

    if (v >= 0.85f)
    {
        return theme.accentRed;
    }

    if (v >= 0.65f)
    {
        return theme.accentAmber;
    }

    return theme.accentCyan;
}

float SafeRatio(float value, float maxValue)
{
    if (maxValue <= 0.0f)
    {
        return 0.0f;
    }

    return Clamp01(value / maxValue);
}

const graph::ScenarioDef* ActiveScenario(const graph::ScenarioCatalog& scenarios)
{
    return scenarios.Find(graph::production_ids::Tier0IronIngotScenario);
}

const graph::ResourceFlowSummary* FindFlowSummary(
    const graph::ProductionEvaluation& production,
    graph::ResourceId resourceId
)
{
    const auto it = std::find_if(
        production.resources.begin(),
        production.resources.end(),
        [resourceId](const graph::ResourceFlowSummary& summary) {
            return summary.resourceId == resourceId;
        }
    );

    return it == production.resources.end() ? nullptr : &(*it);
}

const graph::ObjectiveStatus* FindObjectiveStatus(
    const graph::ProductionEvaluation& production,
    graph::ObjectiveKind kind,
    graph::ResourceId resourceId
)
{
    const auto it = std::find_if(
        production.objectives.begin(),
        production.objectives.end(),
        [kind, resourceId](const graph::ObjectiveStatus& objective) {
            return objective.kind == kind && objective.resourceId == resourceId;
        }
    );

    return it == production.objectives.end() ? nullptr : &(*it);
}

float ProducedRate(const graph::ProductionEvaluation& production, graph::ResourceId resourceId)
{
    const graph::ResourceFlowSummary* summary = FindFlowSummary(production, resourceId);
    return summary == nullptr ? 0.0f : summary->producedPerMinute;
}

float ConsumedRate(const graph::ProductionEvaluation& production, graph::ResourceId resourceId)
{
    const graph::ResourceFlowSummary* summary = FindFlowSummary(production, resourceId);
    return summary == nullptr ? 0.0f : summary->consumedPerMinute;
}

float SurplusRate(const graph::ProductionEvaluation& production, graph::ResourceId resourceId)
{
    const graph::ResourceFlowSummary* summary = FindFlowSummary(production, resourceId);
    return summary == nullptr ? 0.0f : summary->surplusPerMinute;
}

float ObjectiveRatio(
    const graph::ProductionEvaluation& production,
    graph::ObjectiveKind kind,
    graph::ResourceId resourceId
)
{
    const graph::ObjectiveStatus* objective = FindObjectiveStatus(production, kind, resourceId);
    return objective == nullptr ? 0.0f : Clamp01(objective->satisfactionRatio);
}

std::string FormatWhole(float value)
{
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.0f", value);
    return buffer;
}

std::string FormatRate(float value)
{
    return FormatWhole(value) + "/m";
}

std::string FormatRatePair(float actual, float required)
{
    return FormatWhole(actual) + "/" + FormatWhole(required) + "/m";
}

std::string FormatPower(float value)
{
    return FormatWhole(value) + "kW";
}

std::string FormatPercent(float value)
{
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.0f%%", Clamp01(value) * 100.0f);
    return buffer;
}

std::string FormatCount(std::size_t value)
{
    return std::to_string(value);
}

std::string FormatCount(int value)
{
    return std::to_string(value);
}

struct DashboardMetric
{
    std::string_view label;
    std::string valueText;
    float ratio = 0.0f;
    Color accent{};
};

void DrawProgressBar(Renderer2D& renderer, Rect rect, const UiTheme& theme, float value, Color fill)
{
    const float clamped = Clamp01(value);

    renderer.DrawRect(rect, theme.canvas);
    renderer.DrawRectOutline(rect, theme.panelBorder, 1.0f);

    if (clamped > 0.0f)
    {
        const Rect filled = {
            rect.x + 2.0f,
            rect.y + 2.0f,
            std::max(0.0f, (rect.w - 4.0f) * clamped),
            rect.h - 4.0f
        };

        renderer.DrawRect(filled, fill);
    }
}

void DrawMetricRow(
    Renderer2D& renderer,
    Rect row,
    const UiTheme& theme,
    const DashboardMetric& metric,
    int index
)
{
    const Color rowColor = (index % 2) == 0 ? theme.panelAlt : theme.panel;

    renderer.DrawRect(row, rowColor);
    renderer.DrawRectOutline(row, theme.panelBorder, 1.0f);
    renderer.DrawRect({row.x + 8.0f, row.y + 8.0f, 5.0f, row.h - 16.0f}, metric.accent);

    renderer.DrawText({row.x + 24.0f, row.y + 5.0f}, metric.label, theme.textSecondary);

    // Text rendering is intentionally simple and does not clip per row yet.
    // Approximate right alignment keeps compact values from colliding with
    // labels in narrow side panels until real text measurement lands.
    const float estimatedValueWidth = std::clamp(
        static_cast<float>(metric.valueText.size()) * 6.0f,
        24.0f,
        68.0f
    );
    renderer.DrawText({row.x + row.w - 10.0f - estimatedValueWidth, row.y + 5.0f}, metric.valueText, theme.textMuted);

    const Rect bar = {
        row.x + 24.0f,
        row.y + row.h - 10.0f,
        row.w - 36.0f,
        4.0f
    };

    DrawProgressBar(renderer, bar, theme, metric.ratio, metric.accent);

    const float markerX = bar.x + bar.w * Clamp01(metric.ratio);
    renderer.DrawLine({markerX, row.y + 6.0f}, {markerX, row.y + row.h - 6.0f}, theme.textMuted, 1.0f);
}

void DrawDashboardMetrics(
    Renderer2D& renderer,
    Rect panel,
    const UiTheme& theme,
    const graph::GraphDocument& graph,
    const graph::ProductionEvaluation& production,
    bool primaryPanel
)
{
    const Rect content = InsetRect(panel, theme.panelPadding);
    const float rowHeight = 25.0f;
    const float startY = panel.y + 42.0f;

    const graph::ObjectiveStatus* ingotObjective = FindObjectiveStatus(
        production,
        graph::ObjectiveKind::ProduceAtLeastRate,
        graph::production_ids::IronIngot
    );
    const float ingotProduced = ProducedRate(production, graph::production_ids::IronIngot);
    const float ingotRequired = ingotObjective == nullptr ? 50.0f : ingotObjective->requiredPerMinute;
    const float slurryProduced = ProducedRate(production, graph::production_ids::IronSlurry);
    const float slurryConsumed = ConsumedRate(production, graph::production_ids::IronSlurry);
    const float slurryRatio = ObjectiveRatio(
        production,
        graph::ObjectiveKind::HandleAllProduced,
        graph::production_ids::IronSlurry
    );
    const float invalidRatio = SafeRatio(static_cast<float>(production.invalidConnectionCount), 3.0f);
    const float bottleneckRatio = SafeRatio(static_cast<float>(production.bottleneckCount), 3.0f);
    const float targetRatio = Clamp01(production.targetSatisfactionRatio);
    const float completeRatio = production.scenarioComplete ? 1.0f : 0.0f;

    const std::array<DashboardMetric, 8> primaryMetrics = {{
        {"SCENARIO", production.scenarioComplete ? "DONE" : "OPEN", completeRatio, production.scenarioComplete ? theme.accentGreen : theme.accentAmber},
        {"TARGETS", FormatPercent(targetRatio), targetRatio, Mix(theme.accentAmber, theme.accentGreen, targetRatio)},
        {"INGOT", FormatRatePair(ingotProduced, ingotRequired), SafeRatio(ingotProduced, ingotRequired), theme.accentCyan},
        {"SLURRY", FormatRatePair(slurryConsumed, slurryProduced), slurryRatio, Mix(theme.accentRed, theme.accentGreen, slurryRatio)},
        {"POWER", FormatPower(production.totalPowerKw), SafeRatio(production.totalPowerKw, 600.0f), AlertColor(theme, SafeRatio(production.totalPowerKw, 600.0f))},
        {"WASTE", FormatRate(production.totalWastePerMinute), SafeRatio(production.totalWastePerMinute, 50.0f), AlertColor(theme, SafeRatio(production.totalWastePerMinute, 50.0f))},
        {"BAD LINKS", FormatCount(production.invalidConnectionCount), invalidRatio, production.invalidConnectionCount == 0 ? theme.accentGreen : theme.accentRed},
        {"BLOCKS", FormatCount(production.bottleneckCount), bottleneckRatio, production.bottleneckCount == 0 ? theme.accentGreen : theme.accentAmber}
    }};

    const std::array<DashboardMetric, 8> secondaryMetrics = {{
        {"NODES", FormatCount(graph.nodes.size()), SafeRatio(static_cast<float>(graph.nodes.size()), 8.0f), theme.accentCyan},
        {"LINKS", FormatCount(graph.edges.size()), SafeRatio(static_cast<float>(graph.edges.size()), 8.0f), theme.accentCyan},
        {"INGOT", FormatRate(ingotProduced), SafeRatio(ingotProduced, ingotRequired), theme.accentCyan},
        {"SLURRY OUT", FormatRate(slurryProduced), SafeRatio(slurryProduced, 10.0f), theme.accentAmber},
        {"SLURRY IN", FormatRate(slurryConsumed), slurryRatio, Mix(theme.accentRed, theme.accentGreen, slurryRatio)},
        {"SURPLUS", FormatRate(SurplusRate(production, graph::production_ids::IronIngot)), SafeRatio(SurplusRate(production, graph::production_ids::IronIngot), 50.0f), theme.accentGreen},
        {"GRAPH", production.invalidConnectionCount == 0 && !production.hasCycle ? "CLEAR" : "BLOCKED", production.invalidConnectionCount == 0 && !production.hasCycle ? 1.0f : 0.0f, production.invalidConnectionCount == 0 && !production.hasCycle ? theme.accentGreen : theme.accentRed},
        {"GOALS", FormatCount(static_cast<int>(production.objectives.size())), SafeRatio(static_cast<float>(production.objectives.size()), 4.0f), theme.accentCyan}
    }};

    const std::array<DashboardMetric, 8>& metrics = primaryPanel ? primaryMetrics : secondaryMetrics;

    for (int i = 0; i < 8; ++i)
    {
        const float y = startY + static_cast<float>(i) * (rowHeight + 5.0f);
        const Rect row = {content.x, y, content.w, rowHeight};
        DrawMetricRow(renderer, row, theme, metrics[static_cast<std::size_t>(i)], i);
    }
}

void DrawStatusChips(Renderer2D& renderer, Rect topBar, const UiTheme& theme, const graph::ProductionEvaluation& production)
{
    const float chipY = topBar.y + 16.0f;
    const float chipH = 24.0f;
    const float chipW = 124.0f;
    float x = topBar.x + 250.0f;

    const float targetRatio = Clamp01(production.targetSatisfactionRatio);
    const float ingotRatio = ObjectiveRatio(
        production,
        graph::ObjectiveKind::ProduceAtLeastRate,
        graph::production_ids::IronIngot
    );
    const float slurryRatio = ObjectiveRatio(
        production,
        graph::ObjectiveKind::HandleAllProduced,
        graph::production_ids::IronSlurry
    );
    const float validityRatio = production.invalidConnectionCount == 0 && !production.hasCycle ? 1.0f : 0.0f;

    const float values[] = {
        production.scenarioComplete ? 1.0f : 0.0f,
        targetRatio,
        ingotRatio,
        slurryRatio,
        validityRatio
    };

    const Color accents[] = {
        production.scenarioComplete ? theme.accentGreen : theme.accentAmber,
        Mix(theme.accentAmber, theme.accentGreen, targetRatio),
        Mix(theme.accentAmber, theme.accentGreen, ingotRatio),
        Mix(theme.accentRed, theme.accentGreen, slurryRatio),
        validityRatio >= 1.0f ? theme.accentGreen : theme.accentRed
    };

    const std::string_view labels[] = {
        "SCENE",
        "TARGET",
        "INGOT",
        "SLURRY",
        "GRAPH"
    };

    for (int i = 0; i < 5; ++i)
    {
        const Rect chip = {x, chipY, chipW, chipH};
        renderer.DrawRect(chip, theme.panel);
        renderer.DrawRectOutline(chip, theme.panelBorder, 1.0f);
        renderer.DrawRect({chip.x + 10.0f, chip.y + 7.0f, 4.0f, 10.0f}, accents[i]);
        renderer.DrawText({chip.x + 24.0f, chip.y + 3.0f}, labels[i], theme.textSecondary);
        DrawProgressBar(renderer, {chip.x + 24.0f, chip.y + 17.0f, chip.w - 38.0f, 3.0f}, theme, values[i], accents[i]);
        x += chipW + 10.0f;
    }
}

const char* NodeTypeLabel(graph::NodeType type)
{
    switch (type)
    {
        case graph::NodeType::Source: return "SOURCE";
        case graph::NodeType::Machine: return "MACHINE";
        case graph::NodeType::Storage: return "STORAGE";
        case graph::NodeType::Output: return "OUTPUT";
        case graph::NodeType::Contract: return "CONTRACT";
        case graph::NodeType::Modifier: return "MODIFIER";
        case graph::NodeType::Warning: return "WARNING";
    }

    return "MACHINE";
}

const char* ResourceTypeLabel(graph::ResourceType resource)
{
    switch (resource)
    {
        case graph::ResourceType::IronOre: return "IRON ORE";
        case graph::ResourceType::CrushedOre: return "CRUSHED ORE";
        case graph::ResourceType::WashedOre: return "WASHED ORE";
        case graph::ResourceType::Slurry: return "SLURRY";
        case graph::ResourceType::Tailings: return "TAILINGS";
        case graph::ResourceType::IronIngot: return "IRON INGOT";
        case graph::ResourceType::CopperOre: return "COPPER ORE";
        case graph::ResourceType::CopperCathode: return "COPPER CATHODE";
        case graph::ResourceType::Coal: return "COAL";
        case graph::ResourceType::ScrapMetal: return "SCRAP METAL";
        case graph::ResourceType::Stone: return "STONE";
        case graph::ResourceType::Concrete: return "CONCRETE";
        case graph::ResourceType::Waste: return "WASTE";
        case graph::ResourceType::Power: return "POWER";
        case graph::ResourceType::Water: return "WATER";
    }

    return "RESOURCE";
}

std::string FormatId(int value)
{
    return "ID " + std::to_string(value);
}

const graph::NodeEvalResult* FindNodeEval(const graph::SimulationResult& simulation, int nodeId)
{
    const auto it = std::find_if(
        simulation.nodes.begin(),
        simulation.nodes.end(),
        [nodeId](const graph::NodeEvalResult& result) {
            return result.nodeId == nodeId;
        }
    );

    return it == simulation.nodes.end() ? nullptr : &(*it);
}

const graph::GraphEdge* FindEdge(const graph::GraphDocument& graph, int edgeId)
{
    const auto it = std::find_if(
        graph.edges.begin(),
        graph.edges.end(),
        [edgeId](const graph::GraphEdge& edge) {
            return edge.id == edgeId;
        }
    );

    return it == graph.edges.end() ? nullptr : &(*it);
}

std::string NodeNameOrMissing(const graph::GraphNode* node)
{
    return node == nullptr ? std::string("MISSING NODE") : node->name;
}

std::string PortNameOrMissing(const graph::GraphPort* port)
{
    return port == nullptr ? std::string("MISSING PORT") : port->name;
}

void DrawColumnFrame(
    Renderer2D& renderer,
    Rect column,
    const UiTheme& theme,
    std::string_view title,
    Color accent,
    bool selected
)
{
    renderer.DrawRect(column, selected ? theme.panelAlt : theme.panel);
    renderer.DrawRectOutline(column, theme.panelBorder, 1.0f);
    renderer.DrawText({column.x + 10.0f, column.y + 8.0f}, title, theme.textSecondary);
    renderer.DrawLine(
        {column.x + 10.0f, column.y + 24.0f},
        {column.x + column.w - 10.0f, column.y + 24.0f},
        accent,
        2.0f
    );
}

void DrawLabelValue(
    Renderer2D& renderer,
    Vec2 position,
    std::string_view label,
    const std::string& value,
    const UiTheme& theme,
    Color valueColor
)
{
    renderer.DrawText(position, label, theme.textMuted);
    renderer.DrawText({position.x + 120.0f, position.y}, value, valueColor);
}

void DrawPortList(
    Renderer2D& renderer,
    Rect column,
    const UiTheme& theme,
    const std::vector<graph::GraphPort>& ports
)
{
    if (ports.empty())
    {
        renderer.DrawText({column.x + 12.0f, column.y + 38.0f}, "NONE", theme.textMuted);
        return;
    }

    const int maxRows = 4;
    const int count = std::min(static_cast<int>(ports.size()), maxRows);

    for (int i = 0; i < count; ++i)
    {
        const graph::GraphPort& port = ports[static_cast<std::size_t>(i)];
        const float y = column.y + 38.0f + static_cast<float>(i) * 18.0f;
        renderer.DrawRect({column.x + 12.0f, y + 4.0f, 5.0f, 5.0f}, theme.accentCyan);
        renderer.DrawText({column.x + 24.0f, y}, port.name, theme.textPrimary);
        renderer.DrawText({column.x + column.w - 58.0f, y}, FormatId(port.id), theme.textMuted);
    }

    if (static_cast<int>(ports.size()) > maxRows)
    {
        renderer.DrawText({column.x + 12.0f, column.y + 38.0f + static_cast<float>(maxRows) * 18.0f}, "+ MORE", theme.textMuted);
    }
}

void DrawSelectedNodeInspector(
    Renderer2D& renderer,
    Rect inspector,
    const UiTheme& theme,
    const graph::GraphDocument& graph,
    const editor::GraphViewState& view,
    const graph::SimulationResult& simulation
)
{
    const Rect content = InsetRect(inspector, theme.panelPadding);
    const float columnGap = 12.0f;
    const float columnW = (content.w - columnGap * 3.0f) / 4.0f;
    const float y = inspector.y + 44.0f;
    const float h = inspector.h - 58.0f;

    std::array<Rect, 4> columns{};
    for (int i = 0; i < 4; ++i)
    {
        columns[static_cast<std::size_t>(i)] = {
            content.x + static_cast<float>(i) * (columnW + columnGap),
            y,
            columnW,
            h
        };
    }

    const graph::GraphNode* selectedNode = graph::FindNode(graph, view.selectedNodeId);
    const graph::GraphEdge* selectedEdge = FindEdge(graph, view.selectedEdgeId);
    const graph::NodeEvalResult* nodeEval = selectedNode == nullptr ? nullptr : FindNodeEval(simulation, selectedNode->id);

    if (selectedNode == nullptr && selectedEdge != nullptr)
    {
        const graph::GraphNode* fromNode = graph::FindNode(graph, selectedEdge->fromNodeId);
        const graph::GraphNode* toNode = graph::FindNode(graph, selectedEdge->toNodeId);
        const graph::GraphPort* fromPort = graph::FindPort(graph, selectedEdge->fromNodeId, selectedEdge->fromPortId);
        const graph::GraphPort* toPort = graph::FindPort(graph, selectedEdge->toNodeId, selectedEdge->toPortId);

        const bool directionValid =
            fromPort != nullptr &&
            toPort != nullptr &&
            fromPort->direction == graph::PortDirection::Output &&
            toPort->direction == graph::PortDirection::Input;
        const bool resourceValid =
            fromPort != nullptr &&
            toPort != nullptr &&
            fromPort->resource == toPort->resource;
        const bool edgeValid = fromNode != nullptr && toNode != nullptr && directionValid && resourceValid;
        const Color statusColor = edgeValid ? theme.accentGreen : theme.accentRed;
        const Color resourceColor = resourceValid ? theme.accentCyan : theme.accentAmber;

        DrawColumnFrame(renderer, columns[0], theme, "SELECTED LINK", theme.accentCyan, true);
        DrawColumnFrame(renderer, columns[1], theme, "FROM", theme.panelHighlight, false);
        DrawColumnFrame(renderer, columns[2], theme, "TO", theme.panelHighlight, false);
        DrawColumnFrame(renderer, columns[3], theme, "RESOURCE", resourceColor, false);

        renderer.DrawText({columns[0].x + 12.0f, columns[0].y + 36.0f}, FormatId(selectedEdge->id), theme.textPrimary);
        DrawLabelValue(renderer, {columns[0].x + 12.0f, columns[0].y + 58.0f}, "STATUS", edgeValid ? "VALID" : "BROKEN", theme, statusColor);
        DrawLabelValue(renderer, {columns[0].x + 12.0f, columns[0].y + 76.0f}, "DIRECTION", directionValid ? "OUTPUT > INPUT" : "INVALID", theme, directionValid ? theme.textPrimary : theme.accentRed);
        DrawProgressBar(renderer, {columns[0].x + 12.0f, columns[0].y + columns[0].h - 24.0f, columns[0].w - 24.0f, 8.0f}, theme, edgeValid ? 1.0f : 0.35f, statusColor);

        renderer.DrawText({columns[1].x + 12.0f, columns[1].y + 36.0f}, NodeNameOrMissing(fromNode), fromNode == nullptr ? theme.accentRed : theme.textPrimary);
        renderer.DrawText({columns[1].x + 12.0f, columns[1].y + 56.0f}, FormatId(selectedEdge->fromNodeId), theme.textMuted);
        renderer.DrawText({columns[1].x + 12.0f, columns[1].y + 78.0f}, PortNameOrMissing(fromPort), fromPort == nullptr ? theme.accentRed : theme.textSecondary);
        renderer.DrawText({columns[1].x + 12.0f, columns[1].y + 98.0f}, FormatId(selectedEdge->fromPortId), theme.textMuted);

        renderer.DrawText({columns[2].x + 12.0f, columns[2].y + 36.0f}, NodeNameOrMissing(toNode), toNode == nullptr ? theme.accentRed : theme.textPrimary);
        renderer.DrawText({columns[2].x + 12.0f, columns[2].y + 56.0f}, FormatId(selectedEdge->toNodeId), theme.textMuted);
        renderer.DrawText({columns[2].x + 12.0f, columns[2].y + 78.0f}, PortNameOrMissing(toPort), toPort == nullptr ? theme.accentRed : theme.textSecondary);
        renderer.DrawText({columns[2].x + 12.0f, columns[2].y + 98.0f}, FormatId(selectedEdge->toPortId), theme.textMuted);

        const char* resourceLabel = fromPort == nullptr ? "UNKNOWN" : ResourceTypeLabel(fromPort->resource);
        renderer.DrawText({columns[3].x + 12.0f, columns[3].y + 36.0f}, resourceLabel, resourceColor);
        DrawLabelValue(renderer, {columns[3].x + 12.0f, columns[3].y + 58.0f}, "MATCH", resourceValid ? "YES" : "NO", theme, resourceValid ? theme.accentGreen : theme.accentRed);
        renderer.DrawText({columns[3].x + 12.0f, columns[3].y + 82.0f}, "PRESS DELETE TO REMOVE LINK", theme.textMuted);
        renderer.DrawText({columns[3].x + 12.0f, columns[3].y + 102.0f}, "NODE DATA IS UNCHANGED", theme.textMuted);
        return;
    }

    DrawColumnFrame(renderer, columns[0], theme, "SELECTED NODE", theme.accentCyan, true);
    DrawColumnFrame(renderer, columns[1], theme, "METRICS", theme.panelHighlight, false);
    DrawColumnFrame(renderer, columns[2], theme, "INPUTS", theme.panelHighlight, false);
    DrawColumnFrame(renderer, columns[3], theme, "OUTPUTS", theme.panelHighlight, false);

    if (selectedNode == nullptr)
    {
        renderer.DrawText({columns[0].x + 12.0f, columns[0].y + 38.0f}, "NO SELECTION", theme.textMuted);
        renderer.DrawText({columns[0].x + 12.0f, columns[0].y + 58.0f}, "CLICK A NODE OR LINK", theme.textMuted);
        renderer.DrawText({columns[1].x + 12.0f, columns[1].y + 38.0f}, "SELECT AN ITEM TO VIEW DETAILS", theme.textMuted);
        renderer.DrawText({columns[2].x + 12.0f, columns[2].y + 38.0f}, "NONE", theme.textMuted);
        renderer.DrawText({columns[3].x + 12.0f, columns[3].y + 38.0f}, "NONE", theme.textMuted);
        return;
    }

    const float utilization = nodeEval == nullptr ? SafeRatio(selectedNode->throughput, selectedNode->capacity) : Clamp01(nodeEval->utilization);
    const float powerRatio = SafeRatio(selectedNode->powerUse, simulation.totalPowerCapacity);
    const bool warning = selectedNode->warning || (nodeEval != nullptr && nodeEval->warning);
    const Color statusColor = warning ? theme.accentAmber : theme.accentGreen;

    renderer.DrawText({columns[0].x + 12.0f, columns[0].y + 36.0f}, selectedNode->name, theme.textPrimary);
    renderer.DrawText({columns[0].x + 12.0f, columns[0].y + 56.0f}, NodeTypeLabel(selectedNode->type), theme.textSecondary);
    renderer.DrawText({columns[0].x + 12.0f, columns[0].y + 76.0f}, FormatId(selectedNode->id), theme.textMuted);
    renderer.DrawText({columns[0].x + 118.0f, columns[0].y + 76.0f}, warning ? "WARNING" : "NOMINAL", statusColor);

    DrawLabelValue(renderer, {columns[1].x + 12.0f, columns[1].y + 36.0f}, "RATE", FormatRate(selectedNode->throughput), theme, theme.textPrimary);
    DrawLabelValue(renderer, {columns[1].x + 12.0f, columns[1].y + 54.0f}, "CAPACITY", FormatRate(selectedNode->capacity), theme, theme.textPrimary);
    DrawLabelValue(renderer, {columns[1].x + 12.0f, columns[1].y + 72.0f}, "EFF", FormatPercent(selectedNode->efficiency), theme, Mix(theme.accentAmber, theme.accentGreen, selectedNode->efficiency));
    DrawLabelValue(renderer, {columns[1].x + 12.0f, columns[1].y + 90.0f}, "POWER", FormatPower(selectedNode->powerUse), theme, AlertColor(theme, powerRatio));

    DrawProgressBar(renderer, {columns[1].x + 12.0f, columns[1].y + columns[1].h - 24.0f, columns[1].w - 24.0f, 8.0f}, theme, utilization, AlertColor(theme, utilization));

    DrawPortList(renderer, columns[2], theme, selectedNode->inputs);
    DrawPortList(renderer, columns[3], theme, selectedNode->outputs);

    if (warning)
    {
        const std::string warningText = nodeEval != nullptr && !nodeEval->warningText.empty()
            ? nodeEval->warningText
            : selectedNode->warningText;

        renderer.DrawText({columns[3].x + 12.0f, columns[3].y + columns[3].h - 22.0f}, warningText.empty() ? "WARNING ACTIVE" : warningText, theme.accentAmber);
    }
}


Color EventAccentColor(const UiTheme& theme, const std::string& message)
{
    if (message.find("failed") != std::string::npos ||
        message.find("Failed") != std::string::npos ||
        message.find("Invalid") != std::string::npos ||
        message.find("invalid") != std::string::npos ||
        message.find("Bad") != std::string::npos ||
        message.find("incomplete") != std::string::npos ||
        message.find("blocked") != std::string::npos ||
        message.find("exceeds") != std::string::npos ||
        message.find("Bottlenecks active") != std::string::npos)
    {
        return theme.accentRed;
    }

    if (message.find("Warning") != std::string::npos ||
        message.find("Warnings") != std::string::npos ||
        message.find("Low") != std::string::npos ||
        message.find("removed") != std::string::npos ||
        message.find("Removed") != std::string::npos ||
        message.find("bottleneck") != std::string::npos ||
        message.find("clogged") != std::string::npos)
    {
        return theme.accentAmber;
    }

    if (message.find("saved") != std::string::npos ||
        message.find("loaded") != std::string::npos ||
        message.find("reset") != std::string::npos ||
        message.find("ready") != std::string::npos ||
        message.find("complete") != std::string::npos ||
        message.find("clear") != std::string::npos ||
        message.find("created") != std::string::npos ||
        message.find("added") != std::string::npos)
    {
        return theme.accentGreen;
    }

    return theme.accentCyan;
}

float EstimateEventWidth(const std::string& message)
{
    return std::clamp(30.0f + static_cast<float>(message.size()) * 6.0f, 96.0f, 220.0f);
}

std::string CompactProductionEvent(std::string_view message)
{
    // The evaluator keeps detailed messages for tests and diagnostics. The
    // bottom event strip has limited horizontal space, so UI-facing copies are
    // shortened without changing solver output.
    if (message.find("same node") != std::string_view::npos)
    {
        return "Bad link: same node";
    }

    if (message.find("missing endpoint") != std::string_view::npos)
    {
        return "Bad link: missing end";
    }

    if (message.find("output must target input") != std::string_view::npos)
    {
        return "Bad link direction";
    }

    if (message.find("resource mismatch") != std::string_view::npos)
    {
        return "Bad link: resource";
    }

    if (message.find("one edge per port") != std::string_view::npos)
    {
        return "Bad link: split/merge";
    }

    if (message.find("cycle") != std::string_view::npos || message.find("Cycle") != std::string_view::npos)
    {
        return "Cycle blocked";
    }

    if (message.find("missing machine or recipe") != std::string_view::npos)
    {
        return "Bad node data";
    }

    if (message.find("machine cannot run recipe") != std::string_view::npos)
    {
        return "Bad recipe match";
    }

    return std::string(message);
}

bool IsProductionIssueEvent(std::string_view message)
{
    // Objective messages are rendered as compact rate events by App; this keeps
    // only validation/solver issues from the evaluator event stream.
    return
        message.find("Invalid production") != std::string_view::npos ||
        message.find("cycle") != std::string_view::npos ||
        message.find("Cycle") != std::string_view::npos;
}

const char* WireDropFailureMessage(editor::WireDropFailureReason reason)
{
    switch (reason)
    {
        case editor::WireDropFailureReason::None:
            return "";
        case editor::WireDropFailureReason::EmptyTarget:
            return "Invalid link: release on input port";
        case editor::WireDropFailureReason::MissingEndpoint:
            return "Invalid link: missing endpoint";
        case editor::WireDropFailureReason::InvalidDirection:
            return "Invalid link: output must target input";
        case editor::WireDropFailureReason::SelfConnection:
            return "Invalid link: same node";
        case editor::WireDropFailureReason::ResourceMismatch:
            return "Invalid link: resource mismatch";
        case editor::WireDropFailureReason::DuplicateConnection:
            return "Invalid link: duplicate";
        case editor::WireDropFailureReason::InvalidTarget:
            return "Invalid link rejected";
    }

    return "Invalid link rejected";
}

void DrawEventLogMessages(
    Renderer2D& renderer,
    Rect eventLog,
    const UiTheme& theme,
    const std::vector<std::string>& events
)
{
    if (events.empty())
    {
        renderer.DrawText({eventLog.x + 104.0f, eventLog.y + 7.0f}, "NO EVENTS", theme.textMuted);
        return;
    }

    const float y = eventLog.y + 6.0f;
    const float h = 19.0f;
    const float gap = 8.0f;
    const float right = eventLog.x + eventLog.w - 14.0f;
    float x = eventLog.x + 112.0f;

    const int maxEvents = std::min(static_cast<int>(events.size()), 5);
    for (int i = 0; i < maxEvents; ++i)
    {
        const std::string& message = events[static_cast<std::size_t>(i)];
        const float w = EstimateEventWidth(message);

        if (x + w > right)
        {
            break;
        }

        const Rect pill = {x, y, w, h};
        const Color accent = EventAccentColor(theme, message);

        renderer.DrawRect(pill, i == 0 ? theme.panelAlt : theme.panel);
        renderer.DrawRectOutline(pill, theme.panelBorder, 1.0f);
        renderer.DrawRect({pill.x + 7.0f, pill.y + 5.0f, 4.0f, pill.h - 10.0f}, accent);
        renderer.DrawText({pill.x + 16.0f, pill.y + 4.0f}, message, i == 0 ? theme.textPrimary : theme.textSecondary);

        x += w + gap;
    }
}
}


void App::AddEvent(std::string message)
{
    if (message.empty())
    {
        return;
    }

    if (!m_eventLog.empty() && m_eventLog.front() == message)
    {
        return;
    }

    const auto existing = std::find(m_eventLog.begin(), m_eventLog.end(), message);
    if (existing != m_eventLog.end())
    {
        m_eventLog.erase(existing);
    }

    m_eventLog.insert(m_eventLog.begin(), std::move(message));

    constexpr std::size_t maxEvents = 8;
    if (m_eventLog.size() > maxEvents)
    {
        m_eventLog.resize(maxEvents);
    }
}

void App::MarkGraphDirty()
{
    // Graph-data mutations invalidate both evaluator outputs. View-only state
    // such as pan, zoom, hover, selection, and node visual positions is kept out
    // of this path so the production solver is not recomputed every frame.
    m_legacyEvaluationDirty = true;
    m_productionEvaluationDirty = true;
}

void App::EvaluateGraphIfDirty()
{
    if (m_legacyEvaluationDirty)
    {
        // Keep the legacy evaluator alive for the selected-node inspector while
        // the dashboard migrates to the new production evaluator. Later patches
        // can remove this bridge once all UI panels consume ProductionEvaluation.
        m_simulationResult = graph::EvaluateGraph(m_graph);
        m_legacyEvaluationDirty = false;
    }

    if (!m_productionEvaluationDirty)
    {
        return;
    }

    const graph::ScenarioDef* scenario = ActiveScenario(m_scenarioCatalog);
    if (scenario != nullptr)
    {
        const graph::ProductionEvaluator evaluator(m_resourceCatalog, m_machineCatalog, m_recipeCatalog);
        m_productionEvaluation = evaluator.Evaluate(m_graph, *scenario);
    }
    else
    {
        m_productionEvaluation = {};
        m_productionEvaluation.events.push_back("Scenario missing: Tier 0 Iron Ingot Chain");
    }

    m_productionEvaluationDirty = false;
}

void App::UpdateEventLogFromProduction()
{
    const bool firstUpdate = !m_eventLogPrimed;
    const std::size_t nodeCount = m_graph.nodes.size();
    const std::size_t edgeCount = m_graph.edges.size();

    if (firstUpdate)
    {
        AddEvent("Production evaluator ready");
    }

    if (firstUpdate || nodeCount != m_lastNodeCount)
    {
        AddEvent("Nodes active: " + std::to_string(nodeCount));
    }

    if (firstUpdate || edgeCount != m_lastEdgeCount)
    {
        AddEvent("Links active: " + std::to_string(edgeCount));
    }

    const bool scenarioStateChanged =
        firstUpdate || m_productionEvaluation.scenarioComplete != m_lastScenarioComplete;
    const bool invalidCountChanged =
        firstUpdate || m_productionEvaluation.invalidConnectionCount != m_lastProductionInvalidConnectionCount;
    const bool bottleneckCountChanged =
        firstUpdate || m_productionEvaluation.bottleneckCount != m_lastProductionBottleneckCount;

    if (scenarioStateChanged)
    {
        AddEvent(m_productionEvaluation.scenarioComplete ? "Scenario complete" : "Scenario incomplete");
    }

    if (invalidCountChanged)
    {
        if (m_productionEvaluation.invalidConnectionCount > 0)
        {
            AddEvent("Invalid production links: " + std::to_string(m_productionEvaluation.invalidConnectionCount));
        }
        else if (!firstUpdate || m_lastProductionInvalidConnectionCount > 0)
        {
            AddEvent("Production links clear");
        }
    }

    if (bottleneckCountChanged)
    {
        if (m_productionEvaluation.bottleneckCount > 0)
        {
            AddEvent("Production bottlenecks: " + std::to_string(m_productionEvaluation.bottleneckCount));
        }
        else if (!firstUpdate || m_lastProductionBottleneckCount > 0)
        {
            AddEvent("Production bottlenecks clear");
        }
    }

    const bool targetRatioChanged =
        firstUpdate ||
        std::fabs(m_productionEvaluation.targetSatisfactionRatio - m_lastTargetSatisfactionRatio) > 0.001f;

    if (targetRatioChanged || scenarioStateChanged)
    {
        const graph::ObjectiveStatus* ingotObjective = FindObjectiveStatus(
            m_productionEvaluation,
            graph::ObjectiveKind::ProduceAtLeastRate,
            graph::production_ids::IronIngot
        );
        if (ingotObjective != nullptr)
        {
            AddEvent("Ingot " + FormatRatePair(ingotObjective->actualPerMinute, ingotObjective->requiredPerMinute));
        }

        const graph::ObjectiveStatus* slurryObjective = FindObjectiveStatus(
            m_productionEvaluation,
            graph::ObjectiveKind::HandleAllProduced,
            graph::production_ids::IronSlurry
        );
        if (slurryObjective != nullptr)
        {
            AddEvent("Slurry " + FormatRatePair(slurryObjective->actualPerMinute, slurryObjective->requiredPerMinute));
        }
    }

    if (targetRatioChanged || scenarioStateChanged || invalidCountChanged || bottleneckCountChanged)
    {
        for (auto it = m_productionEvaluation.events.rbegin(); it != m_productionEvaluation.events.rend(); ++it)
        {
            if (IsProductionIssueEvent(*it))
            {
                AddEvent(CompactProductionEvent(*it));
            }
        }
    }

    m_lastNodeCount = nodeCount;
    m_lastEdgeCount = edgeCount;
    m_lastProductionInvalidConnectionCount = m_productionEvaluation.invalidConnectionCount;
    m_lastProductionBottleneckCount = m_productionEvaluation.bottleneckCount;
    m_lastScenarioComplete = m_productionEvaluation.scenarioComplete;
    m_lastTargetSatisfactionRatio = m_productionEvaluation.targetSatisfactionRatio;
    m_eventLogPrimed = true;
}

bool App::Initialize(const AppConfig& config)
{
    m_config = config;

    if (!m_window.Create(m_config.title, m_config.width, m_config.height))
    {
        return false;
    }

    if (!m_renderer.Initialize())
    {
        m_window.Shutdown();
        return false;
    }

    m_graph = graph::CreateSampleFactoryGraph();
    m_graphView = editor::CreateSampleFactoryGraphView(m_graph);
    MarkGraphDirty();
    EvaluateGraphIfDirty();
    UpdateEventLogFromProduction();

    m_lastTimeSeconds = GetSeconds();

    std::cout
        << "Crushline UI initialized at "
        << m_window.Width() << "x" << m_window.Height()
        << ".\n";

    if (m_config.maxFrames > 0)
    {
        std::cout << "Auto-exit enabled after " << m_config.maxFrames << " frame(s).\n";
    }
    else
    {
        std::cout << "Press Escape or close the window to quit. Ctrl+R resets, Ctrl+S saves, Ctrl+L loads.\n";
    }

    return true;
}

void App::RunFrame()
{
    const double now = GetSeconds();
    m_deltaTimeSeconds = static_cast<float>(now - m_lastTimeSeconds);
    m_lastTimeSeconds = now;

    m_window.PollEvents(m_input);

    if (m_input.keyEscapePressed)
    {
        m_shouldClose = true;
    }

    std::vector<std::string> dashboardEvents;

    if (m_input.keyCtrlDown && m_input.keyRPressed)
    {
        m_graph = graph::CreateSampleFactoryGraph();
        m_graphView = editor::CreateSampleFactoryGraphView(m_graph);
        m_lastSelectedEdgeHintId = -1;
        MarkGraphDirty();
        dashboardEvents.push_back("Sample graph reset: " + GraphSummary(m_graph));
        std::cout << "Sample factory graph reset.\n";
    }

    if (m_input.keyCtrlDown && m_input.keySPressed)
    {
        std::string errorMessage;
        if (graph::SaveGraphToFile(m_graph, m_graphView, CurrentGraphPath, &errorMessage))
        {
            dashboardEvents.push_back("Graph saved: " + GraphSummary(m_graph));
            std::cout << "Graph saved to " << CurrentGraphPath << ".\n";
        }
        else
        {
            dashboardEvents.push_back("Graph save failed: current_graph.json");
            std::cerr << "Graph save failed: " << errorMessage << "\n";
        }
    }

    if (m_input.keyCtrlDown && m_input.keyLPressed)
    {
        std::string errorMessage;
        if (graph::LoadGraphFromFile(m_graph, m_graphView, CurrentGraphPath, &errorMessage))
        {
            m_lastSelectedEdgeHintId = -1;
            MarkGraphDirty();
            dashboardEvents.push_back("Graph loaded: " + GraphSummary(m_graph));
            std::cout << "Graph loaded from " << CurrentGraphPath << ".\n";
        }
        else
        {
            dashboardEvents.push_back("Graph load failed: current_graph.json");
            std::cerr << "Graph load failed: " << errorMessage << "\n";
        }
    }

    const DashboardRegions regions = ComputeDashboardLayout(
        m_window.Width(),
        m_window.Height(),
        m_layoutMetrics
    );

    editor::EnsureNodeVisuals(m_graphView, m_graph);
    const graph::GraphDocument previousGraph = m_graph;

    if (editor::UpdateGraphViewInteraction(m_graphView, m_graph, m_input, regions.graphCanvas))
    {
        MarkGraphDirty();
        QueueGraphMutationEvents(dashboardEvents, previousGraph, m_graph);
    }

    if (m_graphView.lastWireDropFailure != editor::WireDropFailureReason::None)
    {
        dashboardEvents.push_back(WireDropFailureMessage(m_graphView.lastWireDropFailure));
    }

    if (m_graphView.selectedEdgeId >= 0)
    {
        if (m_graphView.selectedEdgeId != m_lastSelectedEdgeHintId)
        {
            dashboardEvents.push_back("Link selected: Delete removes link");
            m_lastSelectedEdgeHintId = m_graphView.selectedEdgeId;
        }
    }
    else
    {
        m_lastSelectedEdgeHintId = -1;
    }

    EvaluateGraphIfDirty();
    UpdateEventLogFromProduction();

    for (auto it = dashboardEvents.rbegin(); it != dashboardEvents.rend(); ++it)
    {
        AddEvent(*it);
    }

    m_renderer.BeginFrame(m_window.Width(), m_window.Height(), m_theme.background);

    DrawPanel(m_renderer, regions.topBar, m_theme, m_theme.panelAlt);
    DrawPanel(m_renderer, regions.leftPanel, m_theme, m_theme.panel);
    m_renderer.DrawText({regions.topBar.x + 22.0f, regions.topBar.y + 13.0f}, "CRUSHLINE", m_theme.textPrimary);
    m_renderer.DrawText({regions.leftPanel.x + 14.0f, regions.leftPanel.y + 8.0f}, "OBJECTIVES", m_theme.textSecondary);
    m_renderer.DrawRect(regions.graphCanvas, m_theme.canvas);
    DrawGrid(m_renderer, regions.graphCanvas, m_theme, m_graphView.cameraOffset, m_graphView.zoom);
    m_renderer.DrawRectOutline(regions.graphCanvas, m_theme.panelBorder, m_theme.borderThickness);
    DrawPanel(m_renderer, regions.rightPanel, m_theme, m_theme.panel);
    DrawPanel(m_renderer, regions.inspector, m_theme, m_theme.panel);
    DrawPanel(m_renderer, regions.eventLog, m_theme, m_theme.panelAlt);
    m_renderer.DrawText({regions.graphCanvas.x + 12.0f, regions.graphCanvas.y + 8.0f}, "GRAPH", m_theme.textMuted);
    m_renderer.DrawText({regions.rightPanel.x + 14.0f, regions.rightPanel.y + 8.0f}, "PRODUCTION", m_theme.textSecondary);
    m_renderer.DrawText({regions.inspector.x + 14.0f, regions.inspector.y + 8.0f}, "INSPECTOR", m_theme.textSecondary);
    m_renderer.DrawText({regions.eventLog.x + 14.0f, regions.eventLog.y + 8.0f}, "EVENT LOG", m_theme.textSecondary);
    DrawEventLogMessages(m_renderer, regions.eventLog, m_theme, m_eventLog);

    DrawStatusChips(m_renderer, regions.topBar, m_theme, m_productionEvaluation);
    DrawDashboardMetrics(m_renderer, regions.leftPanel, m_theme, m_graph, m_productionEvaluation, true);
    DrawDashboardMetrics(m_renderer, regions.rightPanel, m_theme, m_graph, m_productionEvaluation, false);
    DrawSelectedNodeInspector(m_renderer, regions.inspector, m_theme, m_graph, m_graphView, m_simulationResult);

    editor::DrawGraphView(m_renderer, m_graph, m_graphView, regions.graphCanvas, m_theme);

    const std::size_t drawCommandCount = m_renderer.CommandCount();
    m_renderer.Flush();

    if (m_config.logEveryNFrames > 0 && (m_frameIndex % m_config.logEveryNFrames) == 0)
    {
        std::cout
            << "frame=" << m_frameIndex
            << " dt=" << m_deltaTimeSeconds
            << " mouse=(" << m_input.mousePosition.x << ", " << m_input.mousePosition.y << ")"
            << " delta=(" << m_input.mouseDelta.x << ", " << m_input.mouseDelta.y << ")"
            << " wheel=" << m_input.mouseWheelDelta
            << " selectedNode=" << m_graphView.selectedNodeId
            << " hoveredNode=" << m_graphView.hoveredNodeId
            << " hoveredEdge=" << m_graphView.hoveredEdgeId
            << " selectedEdge=" << m_graphView.selectedEdgeId
            << " hoveredPort=" << m_graphView.hoveredPortNodeId << ":" << m_graphView.hoveredPortId
            << " draggingNode=" << m_graphView.draggingNodeId
            << " draggingWire=" << (m_graphView.draggingWire ? 1 : 0)
            << " nodes=" << m_graph.nodes.size()
            << " edges=" << m_graph.edges.size()
            << " scenarioComplete=" << (m_productionEvaluation.scenarioComplete ? 1 : 0)
            << " target=" << m_productionEvaluation.targetSatisfactionRatio
            << " productionPower=" << m_productionEvaluation.totalPowerKw
            << " unmanagedWaste=" << m_productionEvaluation.totalWastePerMinute
            << " invalidProductionLinks=" << m_productionEvaluation.invalidConnectionCount
            << " productionBottlenecks=" << m_productionEvaluation.bottleneckCount
            << " panning=" << (m_graphView.panningCanvas ? 1 : 0)
            << " zoom=" << m_graphView.zoom
            << " camera=(" << m_graphView.cameraOffset.x << ", " << m_graphView.cameraOffset.y << ")"
            << " drawCommands=" << drawCommandCount
            << "\n";
    }

    m_renderer.EndFrame();

    m_window.SwapBuffers();

    ++m_frameIndex;

    if (m_config.maxFrames > 0 && m_frameIndex >= m_config.maxFrames)
    {
        m_shouldClose = true;
    }
}

void App::Shutdown()
{
    m_renderer.Shutdown();
    m_window.Shutdown();
    std::cout << "Crushline UI shutdown.\n";
}

bool App::ShouldClose() const
{
    return m_shouldClose || m_window.ShouldClose();
}

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

namespace
{
constexpr const char* CurrentGraphPath = "data/current_graph.json";

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

float PowerRatio(const graph::SimulationResult& simulation)
{
    return SafeRatio(simulation.totalPowerUse, simulation.totalPowerCapacity);
}

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

void DrawMetricRow(Renderer2D& renderer, Rect row, const UiTheme& theme, float value, Color accent, int index)
{
    const Color rowColor = (index % 2) == 0 ? theme.panelAlt : theme.panel;

    renderer.DrawRect(row, rowColor);
    renderer.DrawRectOutline(row, theme.panelBorder, 1.0f);
    renderer.DrawRect({row.x + 8.0f, row.y + 8.0f, 5.0f, row.h - 16.0f}, accent);

    const Rect bar = {
        row.x + 24.0f,
        row.y + row.h - 11.0f,
        row.w - 36.0f,
        5.0f
    };

    DrawProgressBar(renderer, bar, theme, value, accent);

    const float markerX = bar.x + bar.w * Clamp01(value);
    renderer.DrawLine({markerX, row.y + 6.0f}, {markerX, row.y + row.h - 6.0f}, theme.textMuted, 1.0f);
}

void DrawDashboardMetrics(
    Renderer2D& renderer,
    Rect panel,
    const UiTheme& theme,
    const graph::GraphDocument& graph,
    const graph::SimulationResult& simulation,
    bool primaryPanel
)
{
    const Rect content = InsetRect(panel, theme.panelPadding);
    const float rowHeight = 28.0f;
    const float startY = panel.y + 44.0f;

    const float throughputRatio = SafeRatio(simulation.totalThroughput, 240.0f);
    const float powerRatio = PowerRatio(simulation);
    const float efficiencyRatio = Clamp01(simulation.plantEfficiency);
    const float wasteRatio = Clamp01(simulation.wasteStoragePercent);
    const float warningRatio = SafeRatio(static_cast<float>(simulation.warningCount), 5.0f);
    const float bottleneckRatio = SafeRatio(static_cast<float>(simulation.bottleneckCount), 3.0f);
    const float nodeRatio = SafeRatio(static_cast<float>(graph.nodes.size()), 8.0f);
    const float edgeRatio = SafeRatio(static_cast<float>(graph.edges.size()), 8.0f);

    const std::array<float, 8> primaryValues = {
        throughputRatio,
        nodeRatio,
        edgeRatio,
        efficiencyRatio,
        powerRatio,
        wasteRatio,
        warningRatio,
        bottleneckRatio
    };

    const std::array<float, 8> secondaryValues = {
        efficiencyRatio,
        throughputRatio,
        powerRatio,
        1.0f - powerRatio,
        wasteRatio,
        warningRatio,
        bottleneckRatio,
        edgeRatio
    };

    const std::array<Color, 8> primaryColors = {
        theme.accentCyan,
        theme.accentCyan,
        theme.accentGreen,
        Mix(theme.accentAmber, theme.accentGreen, efficiencyRatio),
        AlertColor(theme, powerRatio),
        AlertColor(theme, wasteRatio),
        AlertColor(theme, warningRatio),
        AlertColor(theme, bottleneckRatio)
    };

    const std::array<Color, 8> secondaryColors = {
        Mix(theme.accentAmber, theme.accentGreen, efficiencyRatio),
        theme.accentCyan,
        AlertColor(theme, powerRatio),
        Mix(theme.accentRed, theme.accentGreen, 1.0f - powerRatio),
        AlertColor(theme, wasteRatio),
        AlertColor(theme, warningRatio),
        AlertColor(theme, bottleneckRatio),
        theme.accentCyan
    };

    const std::array<std::string_view, 8> primaryLabels = {
        "THROUGHPUT",
        "NODES",
        "EDGES",
        "EFFICIENCY",
        "POWER USE",
        "WASTE",
        "WARNINGS",
        "BOTTLENECKS"
    };

    const std::array<std::string_view, 8> secondaryLabels = {
        "PLANT EFF",
        "OUTPUT RATE",
        "POWER LOAD",
        "RESERVE",
        "STORAGE",
        "ALERTS",
        "BLOCKS",
        "LINKS"
    };

    const std::array<float, 8>& values = primaryPanel ? primaryValues : secondaryValues;
    const std::array<Color, 8>& colors = primaryPanel ? primaryColors : secondaryColors;
    const std::array<std::string_view, 8>& labels = primaryPanel ? primaryLabels : secondaryLabels;

    for (int i = 0; i < 8; ++i)
    {
        const float y = startY + static_cast<float>(i) * (rowHeight + 6.0f);
        const Rect row = {content.x, y, content.w, rowHeight};
        DrawMetricRow(renderer, row, theme, values[i], colors[i], i);
        renderer.DrawText({row.x + 24.0f, row.y + 5.0f}, labels[i], theme.textSecondary);
    }
}

void DrawStatusChips(Renderer2D& renderer, Rect topBar, const UiTheme& theme, const graph::SimulationResult& simulation)
{
    const float chipY = topBar.y + 14.0f;
    const float chipH = 28.0f;
    const float chipW = 150.0f;
    float x = topBar.x + 250.0f;

    const float powerRatio = PowerRatio(simulation);
    const float warningRatio = SafeRatio(static_cast<float>(simulation.warningCount), 5.0f);

    const float values[] = {
        SafeRatio(simulation.totalThroughput, 240.0f),
        Clamp01(simulation.plantEfficiency),
        powerRatio,
        Clamp01(simulation.wasteStoragePercent),
        warningRatio
    };

    const Color accents[] = {
        theme.accentCyan,
        Mix(theme.accentAmber, theme.accentGreen, Clamp01(simulation.plantEfficiency)),
        AlertColor(theme, powerRatio),
        AlertColor(theme, simulation.wasteStoragePercent),
        AlertColor(theme, warningRatio)
    };

    const std::string_view labels[] = {
        "FLOW",
        "EFF",
        "POWER",
        "WASTE",
        "WARN"
    };

    for (int i = 0; i < 5; ++i)
    {
        const Rect chip = {x, chipY, chipW, chipH};
        renderer.DrawRect(chip, theme.panel);
        renderer.DrawRectOutline(chip, theme.panelBorder, 1.0f);
        renderer.DrawRect({chip.x + 10.0f, chip.y + 8.0f, 4.0f, 12.0f}, accents[i]);
        renderer.DrawText({chip.x + 24.0f, chip.y + 5.0f}, labels[i], theme.textSecondary);
        DrawProgressBar(renderer, {chip.x + 24.0f, chip.y + 19.0f, chip.w - 38.0f, 4.0f}, theme, values[i], accents[i]);
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

std::string FormatWhole(float value)
{
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.0f", value);
    return buffer;
}

std::string FormatRate(float value)
{
    return FormatWhole(value) + " /m";
}

std::string FormatPower(float value)
{
    return FormatWhole(value) + " kW";
}

std::string FormatPercent(float value)
{
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.0f%%", Clamp01(value) * 100.0f);
    return buffer;
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
    const graph::NodeEvalResult* nodeEval = selectedNode == nullptr ? nullptr : FindNodeEval(simulation, selectedNode->id);

    DrawColumnFrame(renderer, columns[0], theme, "SELECTED NODE", theme.accentCyan, true);
    DrawColumnFrame(renderer, columns[1], theme, "METRICS", theme.panelHighlight, false);
    DrawColumnFrame(renderer, columns[2], theme, "INPUTS", theme.panelHighlight, false);
    DrawColumnFrame(renderer, columns[3], theme, "OUTPUTS", theme.panelHighlight, false);

    if (selectedNode == nullptr)
    {
        renderer.DrawText({columns[0].x + 12.0f, columns[0].y + 38.0f}, "NO NODE SELECTED", theme.textMuted);
        renderer.DrawText({columns[0].x + 12.0f, columns[0].y + 58.0f}, "CLICK A NODE", theme.textMuted);
        renderer.DrawText({columns[1].x + 12.0f, columns[1].y + 38.0f}, "SELECT A NODE TO VIEW DETAILS", theme.textMuted);
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
    m_simulationResult = graph::EvaluateGraph(m_graph);

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

    if (m_input.keyCtrlDown && m_input.keyRPressed)
    {
        m_graph = graph::CreateSampleFactoryGraph();
        m_graphView = editor::CreateSampleFactoryGraphView(m_graph);
        std::cout << "Sample factory graph reset.\n";
    }

    if (m_input.keyCtrlDown && m_input.keySPressed)
    {
        std::string errorMessage;
        if (graph::SaveGraphToFile(m_graph, m_graphView, CurrentGraphPath, &errorMessage))
        {
            std::cout << "Graph saved to " << CurrentGraphPath << ".\n";
        }
        else
        {
            std::cerr << "Graph save failed: " << errorMessage << "\n";
        }
    }

    if (m_input.keyCtrlDown && m_input.keyLPressed)
    {
        std::string errorMessage;
        if (graph::LoadGraphFromFile(m_graph, m_graphView, CurrentGraphPath, &errorMessage))
        {
            m_simulationResult = graph::EvaluateGraph(m_graph);
            std::cout << "Graph loaded from " << CurrentGraphPath << ".\n";
        }
        else
        {
            std::cerr << "Graph load failed: " << errorMessage << "\n";
        }
    }

    const DashboardRegions regions = ComputeDashboardLayout(
        m_window.Width(),
        m_window.Height(),
        m_layoutMetrics
    );

    editor::EnsureNodeVisuals(m_graphView, m_graph);
    editor::UpdateGraphViewInteraction(m_graphView, m_graph, m_input, regions.graphCanvas);
    m_simulationResult = graph::EvaluateGraph(m_graph);

    m_renderer.BeginFrame(m_window.Width(), m_window.Height(), m_theme.background);

    DrawPanel(m_renderer, regions.topBar, m_theme, m_theme.panelAlt);
    DrawPanel(m_renderer, regions.leftPanel, m_theme, m_theme.panel);
    m_renderer.DrawText({regions.topBar.x + 22.0f, regions.topBar.y + 13.0f}, "CRUSHLINE", m_theme.textPrimary);
    m_renderer.DrawText({regions.leftPanel.x + 14.0f, regions.leftPanel.y + 8.0f}, "INPUTS", m_theme.textSecondary);
    m_renderer.DrawRect(regions.graphCanvas, m_theme.canvas);
    DrawGrid(m_renderer, regions.graphCanvas, m_theme, m_graphView.cameraOffset, m_graphView.zoom);
    m_renderer.DrawRectOutline(regions.graphCanvas, m_theme.panelBorder, m_theme.borderThickness);
    DrawPanel(m_renderer, regions.rightPanel, m_theme, m_theme.panel);
    DrawPanel(m_renderer, regions.inspector, m_theme, m_theme.panel);
    DrawPanel(m_renderer, regions.eventLog, m_theme, m_theme.panelAlt);
    m_renderer.DrawText({regions.graphCanvas.x + 12.0f, regions.graphCanvas.y + 8.0f}, "GRAPH", m_theme.textMuted);
    m_renderer.DrawText({regions.rightPanel.x + 14.0f, regions.rightPanel.y + 8.0f}, "PLANT", m_theme.textSecondary);
    m_renderer.DrawText({regions.inspector.x + 14.0f, regions.inspector.y + 8.0f}, "INSPECTOR", m_theme.textSecondary);
    m_renderer.DrawText({regions.eventLog.x + 14.0f, regions.eventLog.y + 8.0f}, "EVENT LOG", m_theme.textSecondary);

    DrawStatusChips(m_renderer, regions.topBar, m_theme, m_simulationResult);
    DrawDashboardMetrics(m_renderer, regions.leftPanel, m_theme, m_graph, m_simulationResult, true);
    DrawDashboardMetrics(m_renderer, regions.rightPanel, m_theme, m_graph, m_simulationResult, false);
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
            << " throughput=" << m_simulationResult.totalThroughput
            << " power=" << m_simulationResult.totalPowerUse << "/" << m_simulationResult.totalPowerCapacity
            << " efficiency=" << m_simulationResult.plantEfficiency
            << " warnings=" << m_simulationResult.warningCount
            << " bottlenecks=" << m_simulationResult.bottleneckCount
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

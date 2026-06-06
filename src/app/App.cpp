#include "app/App.h"

#include "platform/Time.h"
#include "graph/GraphSerializer.h"
#include "graph/SampleGraph.h"

#include <algorithm>
#include <array>
#include <cmath>
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

void DrawInspectorBands(Renderer2D& renderer, Rect inspector, const UiTheme& theme)
{
    const Rect content = InsetRect(inspector, theme.panelPadding);
    const float columnGap = 12.0f;
    const float columnW = (content.w - columnGap * 3.0f) / 4.0f;
    const float y = inspector.y + 44.0f;
    const float h = inspector.h - 58.0f;

    for (int i = 0; i < 4; ++i)
    {
        const Rect column = {content.x + static_cast<float>(i) * (columnW + columnGap), y, columnW, h};
        renderer.DrawRect(column, i == 0 ? theme.panelAlt : theme.panel);
        renderer.DrawRectOutline(column, theme.panelBorder, 1.0f);
        renderer.DrawText({column.x + 10.0f, column.y + 8.0f}, i == 0 ? "SELECTED" : "DETAILS", theme.textSecondary);
        renderer.DrawLine(
            {column.x + 10.0f, column.y + 24.0f},
            {column.x + column.w - 10.0f, column.y + 24.0f},
            i == 0 ? theme.accentCyan : theme.panelHighlight,
            2.0f
        );
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
    DrawInspectorBands(m_renderer, regions.inspector, m_theme);

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

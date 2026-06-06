#include "app/App.h"

#include "platform/Time.h"
#include "graph/SampleGraph.h"

#include <cmath>
#include <iostream>

namespace
{
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

void DrawMetricRows(Renderer2D& renderer, Rect panel, const UiTheme& theme, int rows)
{
    const Rect content = InsetRect(panel, theme.panelPadding);
    const float rowHeight = 28.0f;
    const float startY = panel.y + 44.0f;

    for (int i = 0; i < rows; ++i)
    {
        const float y = startY + static_cast<float>(i) * (rowHeight + 6.0f);
        const Rect row = {content.x, y, content.w, rowHeight};
        const Color rowColor = (i % 2) == 0 ? theme.panelAlt : theme.panel;

        renderer.DrawRect(row, rowColor);
        renderer.DrawRectOutline(row, theme.panelBorder, 1.0f);
        renderer.DrawRect({row.x + 8.0f, row.y + 8.0f, 12.0f, 12.0f}, theme.panelHighlight);
        renderer.DrawLine(
            {row.x + row.w * 0.55f, row.y + row.h - 7.0f},
            {row.x + row.w - 10.0f, row.y + row.h - 7.0f},
            i == rows - 1 ? theme.accentAmber : theme.accentCyan,
            2.0f
        );
    }
}

void DrawStatusChips(Renderer2D& renderer, Rect topBar, const UiTheme& theme)
{
    const float chipY = topBar.y + 14.0f;
    const float chipH = 28.0f;
    const float chipW = 150.0f;
    float x = topBar.x + 250.0f;

    const Color fills[] = {
        theme.panel,
        theme.panel,
        theme.panel,
        theme.panel,
        theme.panel
    };

    const Color accents[] = {
        theme.accentCyan,
        theme.accentAmber,
        theme.accentRed,
        theme.accentCyan,
        theme.accentGreen
    };

    for (int i = 0; i < 5; ++i)
    {
        const Rect chip = {x, chipY, chipW, chipH};
        renderer.DrawRect(chip, fills[i]);
        renderer.DrawRectOutline(chip, theme.panelBorder, 1.0f);
        renderer.DrawRect({chip.x + 10.0f, chip.y + 8.0f, 4.0f, 12.0f}, accents[i]);
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
        std::cout << "Press Escape or close the window to quit.\n";
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

    m_renderer.BeginFrame(m_window.Width(), m_window.Height(), m_theme.background);

    const DashboardRegions regions = ComputeDashboardLayout(
        m_window.Width(),
        m_window.Height(),
        m_layoutMetrics
    );

    DrawPanel(m_renderer, regions.topBar, m_theme, m_theme.panelAlt);
    DrawPanel(m_renderer, regions.leftPanel, m_theme, m_theme.panel);
    m_renderer.DrawRect(regions.graphCanvas, m_theme.canvas);
    DrawGrid(m_renderer, regions.graphCanvas, m_theme, m_graphView.cameraOffset, m_graphView.zoom);
    m_renderer.DrawRectOutline(regions.graphCanvas, m_theme.panelBorder, m_theme.borderThickness);
    DrawPanel(m_renderer, regions.rightPanel, m_theme, m_theme.panel);
    DrawPanel(m_renderer, regions.inspector, m_theme, m_theme.panel);
    DrawPanel(m_renderer, regions.eventLog, m_theme, m_theme.panelAlt);

    DrawStatusChips(m_renderer, regions.topBar, m_theme);
    DrawMetricRows(m_renderer, regions.leftPanel, m_theme, 8);
    DrawMetricRows(m_renderer, regions.rightPanel, m_theme, 7);
    DrawInspectorBands(m_renderer, regions.inspector, m_theme);

    editor::EnsureNodeVisuals(m_graphView, m_graph);
    editor::UpdateGraphViewInteraction(m_graphView, m_graph, m_input, regions.graphCanvas);
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

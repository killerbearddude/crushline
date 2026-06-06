#include "app/App.h"

#include "platform/Time.h"
#include "graph/SampleGraph.h"

#include <iostream>

namespace
{
void DrawPanel(Renderer2D& renderer, Rect rect, const UiTheme& theme, Color fill)
{
    renderer.DrawRect(rect, fill);
    renderer.DrawRect(TopSlice(rect, 28.0f), theme.panelHeader);
    renderer.DrawRectOutline(rect, theme.panelBorder, theme.borderThickness);
}

void DrawGrid(Renderer2D& renderer, Rect rect, const UiTheme& theme)
{
    const float spacing = theme.gridSpacing;
    const float majorSpacing = spacing * 4.0f;

    for (float x = rect.x; x <= rect.x + rect.w; x += spacing)
    {
        const int column = static_cast<int>((x - rect.x) / spacing);
        const bool major = (column % 4) == 0;
        renderer.DrawLine({x, rect.y}, {x, rect.y + rect.h}, major ? theme.gridMajor : theme.gridMinor, 1.0f);
    }

    for (float y = rect.y; y <= rect.y + rect.h; y += spacing)
    {
        const int row = static_cast<int>((y - rect.y) / spacing);
        const bool major = (row % 4) == 0;
        renderer.DrawLine({rect.x, y}, {rect.x + rect.w, y}, major ? theme.gridMajor : theme.gridMinor, 1.0f);
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
    DrawGrid(m_renderer, regions.graphCanvas, m_theme);
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

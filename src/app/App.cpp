#include "app/App.h"

#include "platform/Time.h"


#include <iostream>

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

    const Color background = {0.035f, 0.038f, 0.042f, 1.0f};
    const Color panel = {0.070f, 0.075f, 0.080f, 1.0f};
    const Color panelAlt = {0.095f, 0.100f, 0.105f, 1.0f};
    const Color border = {0.190f, 0.205f, 0.215f, 1.0f};
    const Color accent = {0.100f, 0.720f, 0.740f, 1.0f};

    m_renderer.BeginFrame(m_window.Width(), m_window.Height(), background);

    const float width = static_cast<float>(m_window.Width());
    const float height = static_cast<float>(m_window.Height());
    const float topBarHeight = 56.0f;
    const float sideWidth = 220.0f;
    const float inspectorHeight = 148.0f;
    const float eventLogHeight = 44.0f;
    const float gap = 8.0f;

    const Rect topBar = {0.0f, 0.0f, width, topBarHeight};
    const Rect leftPanel = {gap, topBarHeight + gap, sideWidth, height - topBarHeight - inspectorHeight - eventLogHeight - gap * 4.0f};
    const Rect rightPanel = {width - sideWidth - gap, topBarHeight + gap, sideWidth, leftPanel.h};
    const Rect graphPanel = {sideWidth + gap * 2.0f, topBarHeight + gap, width - sideWidth * 2.0f - gap * 4.0f, leftPanel.h};
    const Rect inspector = {gap, topBarHeight + gap * 2.0f + leftPanel.h, width - gap * 2.0f, inspectorHeight};
    const Rect eventLog = {gap, height - eventLogHeight - gap, width - gap * 2.0f, eventLogHeight};

    m_renderer.DrawRect(topBar, panelAlt);
    m_renderer.DrawRect(leftPanel, panel);
    m_renderer.DrawRect(graphPanel, background);
    m_renderer.DrawRect(rightPanel, panel);
    m_renderer.DrawRect(inspector, panel);
    m_renderer.DrawRect(eventLog, panelAlt);

    m_renderer.DrawRectOutline(topBar, border, 1.0f);
    m_renderer.DrawRectOutline(leftPanel, border, 1.0f);
    m_renderer.DrawRectOutline(graphPanel, border, 1.0f);
    m_renderer.DrawRectOutline(rightPanel, border, 1.0f);
    m_renderer.DrawRectOutline(inspector, border, 1.0f);
    m_renderer.DrawRectOutline(eventLog, border, 1.0f);

    m_renderer.DrawLine(
        {graphPanel.x + 24.0f, graphPanel.y + 24.0f},
        {graphPanel.x + graphPanel.w - 24.0f, graphPanel.y + graphPanel.h - 24.0f},
        accent,
        2.0f
    );
    m_renderer.DrawCircle({graphPanel.x + 24.0f, graphPanel.y + 24.0f}, 5.0f, accent);
    m_renderer.DrawCircle({graphPanel.x + graphPanel.w - 24.0f, graphPanel.y + graphPanel.h - 24.0f}, 5.0f, accent);

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

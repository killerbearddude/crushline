#include "app/App.h"

#include "platform/Time.h"

#include <SDL3/SDL_opengl.h>

#include <iostream>

bool App::Initialize()
{
    if (!m_window.Create("Crushline", 1536, 864))
    {
        return false;
    }

    m_lastTimeSeconds = GetSeconds();

    std::cout << "Crushline UI initialized.\n";
    std::cout << "Press Escape or close the window to quit.\n";

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

    glViewport(0, 0, m_window.Width(), m_window.Height());
    glClearColor(0.035f, 0.038f, 0.042f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if ((m_frameIndex % 60) == 0)
    {
        std::cout
            << "frame=" << m_frameIndex
            << " dt=" << m_deltaTimeSeconds
            << " mouse=(" << m_input.mousePosition.x << ", " << m_input.mousePosition.y << ")"
            << " wheel=" << m_input.mouseWheelDelta
            << "\n";
    }

    m_window.SwapBuffers();

    ++m_frameIndex;
}

void App::Shutdown()
{
    m_window.Shutdown();
    std::cout << "Crushline UI shutdown.\n";
}

bool App::ShouldClose() const
{
    return m_shouldClose || m_window.ShouldClose();
}

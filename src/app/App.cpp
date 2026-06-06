#include "app/App.h"

#include <iostream>

bool App::Initialize()
{
    std::cout << "Crushline UI bootstrap initialized.\n";
    return true;
}

void App::RunFrame()
{
    std::cout << "Frame " << m_frameIndex << "\n";

    ++m_frameIndex;

    // Temporary: exit immediately until the platform/window layer exists.
    m_shouldClose = true;
}

void App::Shutdown()
{
    std::cout << "Crushline UI shutdown.\n";
}

bool App::ShouldClose() const
{
    return m_shouldClose;
}

#pragma once

#include "platform/Input.h"
#include "platform/Window.h"

class App
{
public:
    bool Initialize();
    void RunFrame();
    void Shutdown();

    bool ShouldClose() const;

private:
    Window m_window;
    InputState m_input;

    bool m_shouldClose = false;

    int m_frameIndex = 0;

    double m_lastTimeSeconds = 0.0;
    float m_deltaTimeSeconds = 0.0f;
};

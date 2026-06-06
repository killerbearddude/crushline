#pragma once

#include "platform/Input.h"
#include "platform/Window.h"
#include "renderer/Renderer2D.h"

struct AppConfig
{
    const char* title = "Crushline";
    int width = 1536;
    int height = 864;

    // A value <= 0 means run until the user closes the window.
    int maxFrames = 0;

    // A value <= 0 disables periodic frame logging.
    int logEveryNFrames = 60;
};

class App
{
public:
    bool Initialize(const AppConfig& config = {});
    void RunFrame();
    void Shutdown();

    bool ShouldClose() const;

private:
    AppConfig m_config;

    Window m_window;
    InputState m_input;
    Renderer2D m_renderer;

    bool m_shouldClose = false;

    int m_frameIndex = 0;

    double m_lastTimeSeconds = 0.0;
    float m_deltaTimeSeconds = 0.0f;
};

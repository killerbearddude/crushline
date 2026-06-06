#pragma once

#include "platform/Input.h"

#include <SDL3/SDL_video.h>

class Window
{
public:
    bool Create(const char* title, int width, int height);
    void PollEvents(InputState& input);
    void SwapBuffers();
    void Shutdown();

    bool ShouldClose() const;

    int Width() const;
    int Height() const;

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    bool m_shouldClose = false;

    int m_width = 0;
    int m_height = 0;

    bool m_previousLeftMouseDown = false;
    bool m_previousMiddleMouseDown = false;
    bool m_previousRightMouseDown = false;

    bool m_previousDeleteDown = false;
    bool m_previousEscapeDown = false;
};

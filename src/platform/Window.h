#pragma once

#include "platform/Input.h"

#include <SDL3/SDL_video.h>

class Window
{
public:
    bool Create(const char* title, int width, int height);
    void PollEvents(InputState& input);

    // Enables or disables SDL UTF-8 text input events for focused UI widgets.
    // Normal graph editing leaves text input disabled so hotkeys remain simple.
    void SetTextInputEnabled(bool enabled);

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

    bool m_textInputEnabled = false;

    bool m_previousLeftMouseDown = false;
    bool m_previousMiddleMouseDown = false;
    bool m_previousRightMouseDown = false;

    bool m_previousADown = false;
    bool m_previousBackspaceDown = false;
    bool m_previousDeleteDown = false;
    bool m_previousEnterDown = false;
    bool m_previousEscapeDown = false;
    bool m_previousLDown = false;
    bool m_previousRDown = false;
    bool m_previousSDown = false;
    bool m_previousTabDown = false;
};

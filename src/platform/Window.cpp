#include "platform/Window.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

#include <iostream>

bool Window::Create(const char* title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    m_window = SDL_CreateWindow(
        title,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (m_window == nullptr)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);

    if (m_glContext == nullptr)
    {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        SDL_Quit();
        return false;
    }

    if (!SDL_GL_MakeCurrent(m_window, m_glContext))
    {
        std::cerr << "SDL_GL_MakeCurrent failed: " << SDL_GetError() << "\n";
        SDL_GL_DestroyContext(m_glContext);
        m_glContext = nullptr;
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        SDL_Quit();
        return false;
    }

    if (!SDL_GL_SetSwapInterval(1))
    {
        std::cerr << "Warning: SDL_GL_SetSwapInterval failed: " << SDL_GetError() << "\n";
    }

    m_width = width;
    m_height = height;

    return true;
}

void Window::PollEvents(InputState& input)
{
    input.BeginFrame();

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                m_shouldClose = true;
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                m_shouldClose = true;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                SDL_GetWindowSize(m_window, &m_width, &m_height);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                input.mouseWheelDelta += event.wheel.y;
                break;

            default:
                break;
        }
    }

    float mouseX = 0.0f;
    float mouseY = 0.0f;

    const SDL_MouseButtonFlags mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);

    const bool leftDown = (mouseButtons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0;
    const bool middleDown = (mouseButtons & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE)) != 0;
    const bool rightDown = (mouseButtons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;

    const Vec2 newMousePosition = {mouseX, mouseY};

    input.mouseDelta = newMousePosition - input.mousePosition;
    input.mousePosition = newMousePosition;

    input.leftMouseDown = leftDown;
    input.leftMousePressed = leftDown && !m_previousLeftMouseDown;
    input.leftMouseReleased = !leftDown && m_previousLeftMouseDown;

    input.middleMouseDown = middleDown;
    input.middleMousePressed = middleDown && !m_previousMiddleMouseDown;
    input.middleMouseReleased = !middleDown && m_previousMiddleMouseDown;

    input.rightMouseDown = rightDown;
    input.rightMousePressed = rightDown && !m_previousRightMouseDown;
    input.rightMouseReleased = !rightDown && m_previousRightMouseDown;

    const bool* keyboard = SDL_GetKeyboardState(nullptr);

    const bool deleteDown = keyboard[SDL_SCANCODE_DELETE];
    const bool escapeDown = keyboard[SDL_SCANCODE_ESCAPE];
    const bool lDown = keyboard[SDL_SCANCODE_L];
    const bool rDown = keyboard[SDL_SCANCODE_R];
    const bool sDown = keyboard[SDL_SCANCODE_S];

    input.keyDeletePressed = deleteDown && !m_previousDeleteDown;
    input.keyEscapePressed = escapeDown && !m_previousEscapeDown;
    input.keyLPressed = lDown && !m_previousLDown;
    input.keyRPressed = rDown && !m_previousRDown;
    input.keySPressed = sDown && !m_previousSDown;

    input.keyCtrlDown =
        keyboard[SDL_SCANCODE_LCTRL] ||
        keyboard[SDL_SCANCODE_RCTRL];

    input.keyShiftDown =
        keyboard[SDL_SCANCODE_LSHIFT] ||
        keyboard[SDL_SCANCODE_RSHIFT];

    m_previousLeftMouseDown = leftDown;
    m_previousMiddleMouseDown = middleDown;
    m_previousRightMouseDown = rightDown;

    m_previousDeleteDown = deleteDown;
    m_previousEscapeDown = escapeDown;
    m_previousLDown = lDown;
    m_previousRDown = rDown;
    m_previousSDown = sDown;
}

void Window::SwapBuffers()
{
    SDL_GL_SwapWindow(m_window);
}

void Window::Shutdown()
{
    if (m_glContext != nullptr)
    {
        SDL_GL_DestroyContext(m_glContext);
        m_glContext = nullptr;
    }

    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

bool Window::ShouldClose() const
{
    return m_shouldClose;
}

int Window::Width() const
{
    return m_width;
}

int Window::Height() const
{
    return m_height;
}

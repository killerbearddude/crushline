#pragma once

// Stores input sampled by the platform layer for the current frame. The editor
// consumes this state without depending on SDL event structures directly.

#include "core/Math.h"

#include <string>

struct InputState
{
    Vec2 mousePosition{};
    Vec2 mouseDelta{};

    bool leftMouseDown = false;
    bool leftMousePressed = false;
    bool leftMouseReleased = false;

    bool middleMouseDown = false;
    bool middleMousePressed = false;
    bool middleMouseReleased = false;

    bool rightMouseDown = false;
    bool rightMousePressed = false;
    bool rightMouseReleased = false;

    float mouseWheelDelta = 0.0f;

    bool keyAPressed = false;
    bool keyBackspacePressed = false;
    bool keyDeletePressed = false;
    bool keyEnterPressed = false;
    bool keyEscapePressed = false;
    bool keyLPressed = false;
    bool keyRPressed = false;
    bool keySPressed = false;
    bool keyTabPressed = false;
    bool keyCtrlDown = false;
    bool keyShiftDown = false;

    // UTF-8 text committed during this frame. Search UI consumes this instead of
    // raw key codes so future IME or non-US keyboard input has a clear path.
    std::string textInput;

    void BeginFrame()
    {
        mouseDelta = {};
        mouseWheelDelta = 0.0f;

        leftMousePressed = false;
        leftMouseReleased = false;
        middleMousePressed = false;
        middleMouseReleased = false;
        rightMousePressed = false;
        rightMouseReleased = false;

        keyAPressed = false;
        keyBackspacePressed = false;
        keyDeletePressed = false;
        keyEnterPressed = false;
        keyEscapePressed = false;
        keyLPressed = false;
        keyRPressed = false;
        keySPressed = false;
        keyTabPressed = false;

        textInput.clear();
    }
};

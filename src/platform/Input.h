#pragma once

#include "core/Math.h"

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

    bool keyDeletePressed = false;
    bool keyEscapePressed = false;
    bool keyRPressed = false;
    bool keyCtrlDown = false;
    bool keyShiftDown = false;

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

        keyDeletePressed = false;
        keyEscapePressed = false;
        keyRPressed = false;
    }
};

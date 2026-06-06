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
    bool rightMouseDown = false;

    float mouseWheelDelta = 0.0f;

    bool keyDeletePressed = false;
    bool keyEscapePressed = false;
    bool keyCtrlDown = false;
    bool keyShiftDown = false;
};

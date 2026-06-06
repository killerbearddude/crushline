#pragma once

#include "core/Color.h"
#include "core/Math.h"
#include "core/Rect.h"

#include <string>

// Renderer draw commands are the boundary between UI/application code and the
// graphics backend. UI code records intent; Renderer2D decides how to execute it.
enum class DrawCommandType
{
    FilledRect,
    RectOutline,
    Line,
    Circle,
    Text,
    BeginClip,
    EndClip
};

struct DrawCommand
{
    DrawCommandType type = DrawCommandType::FilledRect;

    Rect rect{};
    Vec2 a{};
    Vec2 b{};
    Vec2 position{};

    Color color{};

    float thickness = 1.0f;
    float radius = 0.0f;

    std::string text{};
};

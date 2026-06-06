#pragma once

#include "core/Math.h"

struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool Contains(Vec2 p) const
    {
        return p.x >= x && p.x <= x + w &&
               p.y >= y && p.y <= y + h;
    }
};

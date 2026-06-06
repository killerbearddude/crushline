#pragma once

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

inline Vec2 operator+(Vec2 a, Vec2 b)
{
    return {a.x + b.x, a.y + b.y};
}

inline Vec2 operator-(Vec2 a, Vec2 b)
{
    return {a.x - b.x, a.y - b.y};
}

inline Vec2 operator*(Vec2 v, float s)
{
    return {v.x * s, v.y * s};
}

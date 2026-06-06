#pragma once

#include "core/Color.h"
#include "core/Math.h"
#include "core/Rect.h"
#include "renderer/DrawCommand.h"

#include <cstddef>
#include <string_view>
#include <vector>

class Renderer2D
{
public:
    void BeginFrame(int width, int height, Color clearColor);
    void EndFrame();

    void Submit(const DrawCommand& command);
    void Flush();
    void ClearCommands();

    void DrawRect(Rect rect, Color color);
    void DrawRectOutline(Rect rect, Color color, float thickness);
    void DrawLine(Vec2 a, Vec2 b, Color color, float thickness);
    void DrawCircle(Vec2 center, float radius, Color color);
    void DrawText(Vec2 position, std::string_view text, Color color);

    std::size_t CommandCount() const;

    int Width() const;
    int Height() const;

private:
    std::vector<DrawCommand> m_commands;

    int m_width = 0;
    int m_height = 0;
};

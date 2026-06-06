#include "renderer/Renderer2D.h"

#include <SDL3/SDL_opengl.h>

void Renderer2D::BeginFrame(int width, int height, Color clearColor)
{
    m_width = width;
    m_height = height;

    ClearCommands();

    glViewport(0, 0, m_width, m_height);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer2D::EndFrame()
{
    ClearCommands();
}

void Renderer2D::Submit(const DrawCommand& command)
{
    m_commands.push_back(command);
}

void Renderer2D::Flush()
{
    // The command buffer is intentionally introduced before primitive drawing.
    // Patch 004 will turn these commands into OpenGL geometry. Keeping this as a
    // no-op for now lets application/UI code depend on Renderer2D without
    // coupling itself to immediate OpenGL calls.
}

void Renderer2D::ClearCommands()
{
    m_commands.clear();
}

void Renderer2D::DrawRect(Rect rect, Color color)
{
    DrawCommand command;
    command.type = DrawCommandType::FilledRect;
    command.rect = rect;
    command.color = color;
    Submit(command);
}

void Renderer2D::DrawRectOutline(Rect rect, Color color, float thickness)
{
    DrawCommand command;
    command.type = DrawCommandType::RectOutline;
    command.rect = rect;
    command.color = color;
    command.thickness = thickness;
    Submit(command);
}

void Renderer2D::DrawLine(Vec2 a, Vec2 b, Color color, float thickness)
{
    DrawCommand command;
    command.type = DrawCommandType::Line;
    command.a = a;
    command.b = b;
    command.color = color;
    command.thickness = thickness;
    Submit(command);
}

void Renderer2D::DrawCircle(Vec2 center, float radius, Color color)
{
    DrawCommand command;
    command.type = DrawCommandType::Circle;
    command.position = center;
    command.radius = radius;
    command.color = color;
    Submit(command);
}

void Renderer2D::DrawText(Vec2 position, std::string_view text, Color color)
{
    DrawCommand command;
    command.type = DrawCommandType::Text;
    command.position = position;
    command.text = std::string{text};
    command.color = color;
    Submit(command);
}

std::size_t Renderer2D::CommandCount() const
{
    return m_commands.size();
}

int Renderer2D::Width() const
{
    return m_width;
}

int Renderer2D::Height() const
{
    return m_height;
}

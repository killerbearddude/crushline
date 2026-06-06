#pragma once

#include "core/Color.h"
#include "core/Math.h"
#include "core/Rect.h"
#include "renderer/DrawCommand.h"
#include "renderer/FontAtlas.h"
#include "renderer/Texture.h"

#include <cstddef>
#include <string_view>
#include <vector>

class Renderer2D
{
public:
    bool Initialize();
    void Shutdown();

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
    struct Vertex
    {
        Vec2 position{};
        Color color{};
        Vec2 uv{};
        float useTexture = 0.0f;
    };

    bool BuildPipeline();
    void DestroyPipeline();
    void BuildGeometry();

    void AddRect(Rect rect, Color color);
    void AddRectOutline(Rect rect, Color color, float thickness);
    void AddLine(Vec2 a, Vec2 b, Color color, float thickness);
    void AddCircle(Vec2 center, float radius, Color color);
    void AddText(Vec2 position, std::string_view text, Color color);
    void AddTexturedRect(Rect rect, Vec2 uvMin, Vec2 uvMax, Color color);
    void PushTriangle(Vertex a, Vertex b, Vertex c);

    std::vector<DrawCommand> m_commands;
    std::vector<Vertex> m_vertices;

    unsigned int m_program = 0;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;

    int m_projectionLocation = -1;
    int m_textureLocation = -1;

    renderer::FontAtlas m_fontAtlas;
    renderer::Texture2D m_fontTexture;
    renderer::Texture2D m_whiteTexture;

    int m_width = 0;
    int m_height = 0;
    bool m_initialized = false;
};

#include "renderer/Renderer2D.h"

#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
using GlCreateShaderProc = GLuint (*)(GLenum type);
using GlShaderSourceProc = void (*)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
using GlCompileShaderProc = void (*)(GLuint shader);
using GlGetShaderivProc = void (*)(GLuint shader, GLenum pname, GLint* params);
using GlGetShaderInfoLogProc = void (*)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
using GlDeleteShaderProc = void (*)(GLuint shader);
using GlCreateProgramProc = GLuint (*)();
using GlAttachShaderProc = void (*)(GLuint program, GLuint shader);
using GlLinkProgramProc = void (*)(GLuint program);
using GlGetProgramivProc = void (*)(GLuint program, GLenum pname, GLint* params);
using GlGetProgramInfoLogProc = void (*)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
using GlDeleteProgramProc = void (*)(GLuint program);
using GlUseProgramProc = void (*)(GLuint program);
using GlGetUniformLocationProc = GLint (*)(GLuint program, const GLchar* name);
using GlUniformMatrix4fvProc = void (*)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
using GlUniform1iProc = void (*)(GLint location, GLint v0);
using GlGenVertexArraysProc = void (*)(GLsizei n, GLuint* arrays);
using GlBindVertexArrayProc = void (*)(GLuint array);
using GlDeleteVertexArraysProc = void (*)(GLsizei n, const GLuint* arrays);
using GlGenBuffersProc = void (*)(GLsizei n, GLuint* buffers);
using GlBindBufferProc = void (*)(GLenum target, GLuint buffer);
using GlBufferDataProc = void (*)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
using GlDeleteBuffersProc = void (*)(GLsizei n, const GLuint* buffers);
using GlEnableVertexAttribArrayProc = void (*)(GLuint index);
using GlVertexAttribPointerProc = void (*)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

GlCreateShaderProc glCreateShaderPtr = nullptr;
GlShaderSourceProc glShaderSourcePtr = nullptr;
GlCompileShaderProc glCompileShaderPtr = nullptr;
GlGetShaderivProc glGetShaderivPtr = nullptr;
GlGetShaderInfoLogProc glGetShaderInfoLogPtr = nullptr;
GlDeleteShaderProc glDeleteShaderPtr = nullptr;
GlCreateProgramProc glCreateProgramPtr = nullptr;
GlAttachShaderProc glAttachShaderPtr = nullptr;
GlLinkProgramProc glLinkProgramPtr = nullptr;
GlGetProgramivProc glGetProgramivPtr = nullptr;
GlGetProgramInfoLogProc glGetProgramInfoLogPtr = nullptr;
GlDeleteProgramProc glDeleteProgramPtr = nullptr;
GlUseProgramProc glUseProgramPtr = nullptr;
GlGetUniformLocationProc glGetUniformLocationPtr = nullptr;
GlUniformMatrix4fvProc glUniformMatrix4fvPtr = nullptr;
GlUniform1iProc glUniform1iPtr = nullptr;
GlGenVertexArraysProc glGenVertexArraysPtr = nullptr;
GlBindVertexArrayProc glBindVertexArrayPtr = nullptr;
GlDeleteVertexArraysProc glDeleteVertexArraysPtr = nullptr;
GlGenBuffersProc glGenBuffersPtr = nullptr;
GlBindBufferProc glBindBufferPtr = nullptr;
GlBufferDataProc glBufferDataPtr = nullptr;
GlDeleteBuffersProc glDeleteBuffersPtr = nullptr;
GlEnableVertexAttribArrayProc glEnableVertexAttribArrayPtr = nullptr;
GlVertexAttribPointerProc glVertexAttribPointerPtr = nullptr;

template <typename Proc>
bool LoadGlProc(Proc& proc, const char* name)
{
    proc = reinterpret_cast<Proc>(SDL_GL_GetProcAddress(name));

    if (proc == nullptr)
    {
        std::cerr << "Missing OpenGL function: " << name << "\n";
        return false;
    }

    return true;
}

bool LoadGlFunctions()
{
    return
        LoadGlProc(glCreateShaderPtr, "glCreateShader") &&
        LoadGlProc(glShaderSourcePtr, "glShaderSource") &&
        LoadGlProc(glCompileShaderPtr, "glCompileShader") &&
        LoadGlProc(glGetShaderivPtr, "glGetShaderiv") &&
        LoadGlProc(glGetShaderInfoLogPtr, "glGetShaderInfoLog") &&
        LoadGlProc(glDeleteShaderPtr, "glDeleteShader") &&
        LoadGlProc(glCreateProgramPtr, "glCreateProgram") &&
        LoadGlProc(glAttachShaderPtr, "glAttachShader") &&
        LoadGlProc(glLinkProgramPtr, "glLinkProgram") &&
        LoadGlProc(glGetProgramivPtr, "glGetProgramiv") &&
        LoadGlProc(glGetProgramInfoLogPtr, "glGetProgramInfoLog") &&
        LoadGlProc(glDeleteProgramPtr, "glDeleteProgram") &&
        LoadGlProc(glUseProgramPtr, "glUseProgram") &&
        LoadGlProc(glGetUniformLocationPtr, "glGetUniformLocation") &&
        LoadGlProc(glUniformMatrix4fvPtr, "glUniformMatrix4fv") &&
        LoadGlProc(glUniform1iPtr, "glUniform1i") &&
        LoadGlProc(glGenVertexArraysPtr, "glGenVertexArrays") &&
        LoadGlProc(glBindVertexArrayPtr, "glBindVertexArray") &&
        LoadGlProc(glDeleteVertexArraysPtr, "glDeleteVertexArrays") &&
        LoadGlProc(glGenBuffersPtr, "glGenBuffers") &&
        LoadGlProc(glBindBufferPtr, "glBindBuffer") &&
        LoadGlProc(glBufferDataPtr, "glBufferData") &&
        LoadGlProc(glDeleteBuffersPtr, "glDeleteBuffers") &&
        LoadGlProc(glEnableVertexAttribArrayPtr, "glEnableVertexAttribArray") &&
        LoadGlProc(glVertexAttribPointerPtr, "glVertexAttribPointer");
}

constexpr const char* VertexShaderSource = R"glsl(
#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUv;
layout(location = 3) in float aUseTexture;

uniform mat4 uProjection;

out vec4 vColor;
out vec2 vUv;
out float vUseTexture;

void main()
{
    vColor = aColor;
    vUv = aUv;
    vUseTexture = aUseTexture;
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
}
)glsl";

constexpr const char* FragmentShaderSource = R"glsl(
#version 330 core

in vec4 vColor;
in vec2 vUv;
in float vUseTexture;

uniform sampler2D uTexture;

out vec4 FragColor;

void main()
{
    float alpha = mix(1.0, texture(uTexture, vUv).r, vUseTexture);
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}
)glsl";

GLuint CompileShader(GLenum type, const char* source)
{
    const GLuint shader = glCreateShaderPtr(type);
    glShaderSourcePtr(shader, 1, &source, nullptr);
    glCompileShaderPtr(shader);

    GLint success = 0;
    glGetShaderivPtr(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        std::array<GLchar, 1024> log{};
        glGetShaderInfoLogPtr(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::cerr << "OpenGL shader compile failed: " << log.data() << "\n";
        glDeleteShaderPtr(shader);
        return 0;
    }

    return shader;
}

GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    const GLuint program = glCreateProgramPtr();
    glAttachShaderPtr(program, vertexShader);
    glAttachShaderPtr(program, fragmentShader);
    glLinkProgramPtr(program);

    GLint success = 0;
    glGetProgramivPtr(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE)
    {
        std::array<GLchar, 1024> log{};
        glGetProgramInfoLogPtr(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::cerr << "OpenGL program link failed: " << log.data() << "\n";
        glDeleteProgramPtr(program);
        return 0;
    }

    return program;
}

float Length(Vec2 v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

// UI text is loaded slightly smaller than the original atlas size so the
// dense dashboard panels remain legible without labels colliding with values.
constexpr int UiFontPixelSize = 13;

std::string FindFontPath()
{
    const char* candidates[] = {
        "assets/fonts/Inter-Regular.ttf",
        "assets/fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
    };

    for (const char* candidate : candidates)
    {
        if (std::filesystem::exists(candidate))
        {
            return candidate;
        }
    }

    return {};
}

}

bool Renderer2D::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    if (!LoadGlFunctions())
    {
        return false;
    }

    if (!BuildPipeline())
    {
        return false;
    }

    m_initialized = true;
    return true;
}

void Renderer2D::Shutdown()
{
    DestroyPipeline();
    ClearCommands();
    m_vertices.clear();
    m_initialized = false;
}

bool Renderer2D::BuildPipeline()
{
    const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, VertexShaderSource);
    if (vertexShader == 0)
    {
        return false;
    }

    const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, FragmentShaderSource);
    if (fragmentShader == 0)
    {
        glDeleteShaderPtr(vertexShader);
        return false;
    }

    m_program = LinkProgram(vertexShader, fragmentShader);

    glDeleteShaderPtr(vertexShader);
    glDeleteShaderPtr(fragmentShader);

    if (m_program == 0)
    {
        return false;
    }

    m_projectionLocation = glGetUniformLocationPtr(m_program, "uProjection");
    if (m_projectionLocation < 0)
    {
        std::cerr << "OpenGL program is missing uProjection uniform.\n";
        DestroyPipeline();
        return false;
    }

    m_textureLocation = glGetUniformLocationPtr(m_program, "uTexture");
    if (m_textureLocation < 0)
    {
        std::cerr << "OpenGL program is missing uTexture uniform.\n";
        DestroyPipeline();
        return false;
    }

    glGenVertexArraysPtr(1, &m_vao);
    glGenBuffersPtr(1, &m_vbo);

    glBindVertexArrayPtr(m_vao);
    glBindBufferPtr(GL_ARRAY_BUFFER, m_vbo);

    glEnableVertexAttribArrayPtr(0);
    glVertexAttribPointerPtr(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void*>(offsetof(Vertex, position))
    );

    glEnableVertexAttribArrayPtr(1);
    glVertexAttribPointerPtr(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void*>(offsetof(Vertex, color))
    );

    glEnableVertexAttribArrayPtr(2);
    glVertexAttribPointerPtr(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void*>(offsetof(Vertex, uv))
    );

    glEnableVertexAttribArrayPtr(3);
    glVertexAttribPointerPtr(
        3,
        1,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void*>(offsetof(Vertex, useTexture))
    );

    const std::uint8_t whitePixel = 255;
    if (!m_whiteTexture.CreateAlpha(1, 1, &whitePixel))
    {
        std::cerr << "Failed to create renderer fallback texture.\n";
        DestroyPipeline();
        return false;
    }

    const std::string fontPath = FindFontPath();
    if (!fontPath.empty() && m_fontAtlas.LoadFromFile(fontPath, UiFontPixelSize))
    {
        if (!m_fontTexture.CreateAlpha(m_fontAtlas.Width(), m_fontAtlas.Height(), m_fontAtlas.Pixels().data()))
        {
            std::cerr << "Failed to create font atlas texture for " << fontPath << ".\n";
            m_fontAtlas.Clear();
        }
        else
        {
            std::cout << "Loaded font atlas from " << fontPath << ".\n";
        }
    }
    else
    {
        std::cerr << "Warning: no usable font found. Text commands will be skipped.\n";
    }

    glBindBufferPtr(GL_ARRAY_BUFFER, 0);
    glBindVertexArrayPtr(0);

    return true;
}

void Renderer2D::DestroyPipeline()
{
    if (m_vbo != 0)
    {
        glDeleteBuffersPtr(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_vao != 0)
    {
        glDeleteVertexArraysPtr(1, &m_vao);
        m_vao = 0;
    }

    if (m_program != 0)
    {
        glDeleteProgramPtr(m_program);
        m_program = 0;
    }

    m_fontTexture.Destroy();
    m_whiteTexture.Destroy();
    m_fontAtlas.Clear();

    m_projectionLocation = -1;
    m_textureLocation = -1;
}

void Renderer2D::BeginFrame(int width, int height, Color clearColor)
{
    m_width = width;
    m_height = height;

    ClearCommands();

    glViewport(0, 0, m_width, m_height);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer2D::EndFrame()
{
    ClearCommands();
    m_vertices.clear();
}

void Renderer2D::Submit(const DrawCommand& command)
{
    m_commands.push_back(command);
}

void Renderer2D::Flush()
{
    if (!m_initialized)
    {
        return;
    }

    BuildGeometry();

    if (m_vertices.empty())
    {
        return;
    }

    const std::array<float, 16> projection = {
        2.0f / static_cast<float>(m_width), 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / static_cast<float>(m_height), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    glUseProgramPtr(m_program);
    glUniformMatrix4fvPtr(m_projectionLocation, 1, GL_FALSE, projection.data());
    glUniform1iPtr(m_textureLocation, 0);

    if (m_fontTexture.IsValid())
    {
        m_fontTexture.Bind(0);
    }
    else
    {
        m_whiteTexture.Bind(0);
    }

    glBindVertexArrayPtr(m_vao);
    glBindBufferPtr(GL_ARRAY_BUFFER, m_vbo);
    glBufferDataPtr(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
        m_vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));

    glBindBufferPtr(GL_ARRAY_BUFFER, 0);
    glBindVertexArrayPtr(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgramPtr(0);
}

void Renderer2D::BuildGeometry()
{
    m_vertices.clear();

    for (const DrawCommand& command : m_commands)
    {
        switch (command.type)
        {
            case DrawCommandType::FilledRect:
                AddRect(command.rect, command.color);
                break;

            case DrawCommandType::RectOutline:
                AddRectOutline(command.rect, command.color, command.thickness);
                break;

            case DrawCommandType::Line:
                AddLine(command.a, command.b, command.color, command.thickness);
                break;

            case DrawCommandType::Circle:
                AddCircle(command.position, command.radius, command.color);
                break;

            case DrawCommandType::Text:
                AddText(command.position, command.text, command.color);
                break;

            case DrawCommandType::BeginClip:
            case DrawCommandType::EndClip:
                // Clipping remains a command-buffer concept for a later scissor-stack patch.
                break;
        }
    }
}

void Renderer2D::ClearCommands()
{
    m_commands.clear();
}

void Renderer2D::AddRect(Rect rect, Color color)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f)
    {
        return;
    }

    const Vertex topLeft = {{rect.x, rect.y}, color};
    const Vertex topRight = {{rect.x + rect.w, rect.y}, color};
    const Vertex bottomRight = {{rect.x + rect.w, rect.y + rect.h}, color};
    const Vertex bottomLeft = {{rect.x, rect.y + rect.h}, color};

    PushTriangle(topLeft, topRight, bottomRight);
    PushTriangle(topLeft, bottomRight, bottomLeft);
}

void Renderer2D::AddRectOutline(Rect rect, Color color, float thickness)
{
    const float t = std::max(1.0f, thickness);

    AddRect({rect.x, rect.y, rect.w, t}, color);
    AddRect({rect.x, rect.y + rect.h - t, rect.w, t}, color);
    AddRect({rect.x, rect.y, t, rect.h}, color);
    AddRect({rect.x + rect.w - t, rect.y, t, rect.h}, color);
}

void Renderer2D::AddLine(Vec2 a, Vec2 b, Color color, float thickness)
{
    const Vec2 direction = {b.x - a.x, b.y - a.y};
    const float length = Length(direction);

    if (length <= 0.0001f)
    {
        return;
    }

    const float halfThickness = std::max(1.0f, thickness) * 0.5f;
    const Vec2 normal = {
        -direction.y / length * halfThickness,
        direction.x / length * halfThickness
    };

    const Vertex v0 = {{a.x + normal.x, a.y + normal.y}, color};
    const Vertex v1 = {{b.x + normal.x, b.y + normal.y}, color};
    const Vertex v2 = {{b.x - normal.x, b.y - normal.y}, color};
    const Vertex v3 = {{a.x - normal.x, a.y - normal.y}, color};

    PushTriangle(v0, v1, v2);
    PushTriangle(v0, v2, v3);
}

void Renderer2D::AddCircle(Vec2 center, float radius, Color color)
{
    if (radius <= 0.0f)
    {
        return;
    }

    constexpr int segments = 32;
    constexpr float twoPi = 6.28318530717958647692f;

    for (int i = 0; i < segments; ++i)
    {
        const float a0 = (static_cast<float>(i) / static_cast<float>(segments)) * twoPi;
        const float a1 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * twoPi;

        const Vertex centerVertex = {center, color};
        const Vertex edge0 = {{center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius}, color};
        const Vertex edge1 = {{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius}, color};

        PushTriangle(centerVertex, edge0, edge1);
    }
}


void Renderer2D::AddText(Vec2 position, std::string_view text, Color color)
{
    if (!m_fontAtlas.IsLoaded() || !m_fontTexture.IsValid())
    {
        return;
    }

    Vec2 pen = position;
    const float startX = position.x;
    const float baselineY = position.y + m_fontAtlas.Ascent();

    for (char character : text)
    {
        if (character == '\n')
        {
            pen.x = startX;
            pen.y += m_fontAtlas.LineHeight();
            continue;
        }

        const renderer::Glyph* glyph = m_fontAtlas.FindGlyph(character);
        if (glyph == nullptr)
        {
            continue;
        }

        if (glyph->size.x > 0.0f && glyph->size.y > 0.0f)
        {
            const Rect glyphRect = {
                pen.x + glyph->bearing.x,
                baselineY + pen.y - position.y - glyph->bearing.y,
                glyph->size.x,
                glyph->size.y
            };

            AddTexturedRect(glyphRect, glyph->uvMin, glyph->uvMax, color);
        }

        pen.x += glyph->advance;
    }
}

void Renderer2D::AddTexturedRect(Rect rect, Vec2 uvMin, Vec2 uvMax, Color color)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f)
    {
        return;
    }

    const Vertex topLeft = {{rect.x, rect.y}, color, {uvMin.x, uvMin.y}, 1.0f};
    const Vertex topRight = {{rect.x + rect.w, rect.y}, color, {uvMax.x, uvMin.y}, 1.0f};
    const Vertex bottomRight = {{rect.x + rect.w, rect.y + rect.h}, color, {uvMax.x, uvMax.y}, 1.0f};
    const Vertex bottomLeft = {{rect.x, rect.y + rect.h}, color, {uvMin.x, uvMax.y}, 1.0f};

    PushTriangle(topLeft, topRight, bottomRight);
    PushTriangle(topLeft, bottomRight, bottomLeft);
}

void Renderer2D::PushTriangle(Vertex a, Vertex b, Vertex c)
{
    m_vertices.push_back(a);
    m_vertices.push_back(b);
    m_vertices.push_back(c);
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

#include "renderer/Texture.h"

#include <SDL3/SDL_opengl.h>

#include <utility>

namespace renderer
{
Texture2D::~Texture2D()
{
    Destroy();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
{
    *this = std::move(other);
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this != &other)
    {
        Destroy();

        m_handle = other.m_handle;
        m_width = other.m_width;
        m_height = other.m_height;

        other.m_handle = 0;
        other.m_width = 0;
        other.m_height = 0;
    }

    return *this;
}

bool Texture2D::CreateAlpha(int width, int height, const std::uint8_t* pixels)
{
    Destroy();

    if (width <= 0 || height <= 0 || pixels == nullptr)
    {
        return false;
    }

    GLuint handle = 0;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        width,
        height,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        pixels
    );

    glBindTexture(GL_TEXTURE_2D, 0);

    m_handle = handle;
    m_width = width;
    m_height = height;

    return m_handle != 0;
}

void Texture2D::Destroy()
{
    if (m_handle != 0)
    {
        GLuint handle = m_handle;
        glDeleteTextures(1, &handle);
        m_handle = 0;
    }

    m_width = 0;
    m_height = 0;
}

void Texture2D::Bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_handle);
}

bool Texture2D::IsValid() const
{
    return m_handle != 0;
}

unsigned int Texture2D::Handle() const
{
    return m_handle;
}

int Texture2D::Width() const
{
    return m_width;
}

int Texture2D::Height() const
{
    return m_height;
}
}

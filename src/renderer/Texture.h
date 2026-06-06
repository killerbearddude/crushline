#pragma once

#include <cstdint>

namespace renderer
{
class Texture2D
{
public:
    Texture2D() = default;
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    bool CreateAlpha(int width, int height, const std::uint8_t* pixels);
    void Destroy();
    void Bind(unsigned int unit = 0) const;

    bool IsValid() const;
    unsigned int Handle() const;
    int Width() const;
    int Height() const;

private:
    unsigned int m_handle = 0;
    int m_width = 0;
    int m_height = 0;
};
}

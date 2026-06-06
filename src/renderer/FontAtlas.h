#pragma once

#include "core/Math.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace renderer
{
struct Glyph
{
    Vec2 uvMin{};
    Vec2 uvMax{};
    Vec2 size{};
    Vec2 bearing{};

    float advance = 0.0f;
    bool valid = false;
};

class FontAtlas
{
public:
    static constexpr int FirstGlyph = 32;
    static constexpr int LastGlyph = 126;
    static constexpr int GlyphCount = LastGlyph - FirstGlyph + 1;

    bool LoadFromFile(std::string_view path, int pixelSize);
    void Clear();

    const Glyph* FindGlyph(char character) const;

    const std::vector<std::uint8_t>& Pixels() const;
    int Width() const;
    int Height() const;
    int PixelSize() const;
    float Ascent() const;
    float LineHeight() const;
    const std::string& SourcePath() const;

    bool IsLoaded() const;

private:
    std::array<Glyph, GlyphCount> m_glyphs{};
    std::vector<std::uint8_t> m_pixels;

    std::string m_sourcePath;

    int m_width = 0;
    int m_height = 0;
    int m_pixelSize = 0;

    float m_ascent = 0.0f;
    float m_lineHeight = 0.0f;
    bool m_loaded = false;
};
}

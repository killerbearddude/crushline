#include "renderer/FontAtlas.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <iostream>

namespace renderer
{
namespace
{
constexpr int AtlasWidth = 512;
constexpr int AtlasHeight = 512;
constexpr int GlyphPadding = 2;

int GlyphIndex(char character)
{
    const int code = static_cast<unsigned char>(character);
    return code - FontAtlas::FirstGlyph;
}
}

bool FontAtlas::LoadFromFile(std::string_view path, int pixelSize)
{
    Clear();

    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0)
    {
        std::cerr << "FT_Init_FreeType failed.\n";
        return false;
    }

    FT_Face face = nullptr;
    const std::string pathString{path};
    if (FT_New_Face(library, pathString.c_str(), 0, &face) != 0)
    {
        FT_Done_FreeType(library);
        return false;
    }

    if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixelSize)) != 0)
    {
        std::cerr << "FT_Set_Pixel_Sizes failed for " << pathString << ".\n";
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    m_width = AtlasWidth;
    m_height = AtlasHeight;
    m_pixelSize = pixelSize;
    m_sourcePath = pathString;
    m_pixels.assign(static_cast<std::size_t>(m_width * m_height), 0);

    m_ascent = static_cast<float>(face->size->metrics.ascender >> 6);
    m_lineHeight = static_cast<float>(face->size->metrics.height >> 6);

    int penX = GlyphPadding;
    int penY = GlyphPadding;
    int rowHeight = 0;

    for (int code = FirstGlyph; code <= LastGlyph; ++code)
    {
        if (FT_Load_Char(face, static_cast<FT_ULong>(code), FT_LOAD_RENDER) != 0)
        {
            continue;
        }

        const FT_GlyphSlot slot = face->glyph;
        const FT_Bitmap& bitmap = slot->bitmap;

        const int bitmapWidth = static_cast<int>(bitmap.width);
        const int bitmapHeight = static_cast<int>(bitmap.rows);

        if (penX + bitmapWidth + GlyphPadding >= m_width)
        {
            penX = GlyphPadding;
            penY += rowHeight + GlyphPadding;
            rowHeight = 0;
        }

        if (penY + bitmapHeight + GlyphPadding >= m_height)
        {
            std::cerr << "Font atlas overflow while loading " << pathString << ".\n";
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            Clear();
            return false;
        }

        for (int row = 0; row < bitmapHeight; ++row)
        {
            for (int column = 0; column < bitmapWidth; ++column)
            {
                const int destinationX = penX + column;
                const int destinationY = penY + row;
                const std::size_t destinationIndex = static_cast<std::size_t>(destinationY * m_width + destinationX);
                const std::size_t sourceIndex = static_cast<std::size_t>(row * bitmap.pitch + column);
                m_pixels[destinationIndex] = bitmap.buffer[sourceIndex];
            }
        }

        Glyph glyph;
        glyph.uvMin = {
            static_cast<float>(penX) / static_cast<float>(m_width),
            static_cast<float>(penY) / static_cast<float>(m_height)
        };
        glyph.uvMax = {
            static_cast<float>(penX + bitmapWidth) / static_cast<float>(m_width),
            static_cast<float>(penY + bitmapHeight) / static_cast<float>(m_height)
        };
        glyph.size = {static_cast<float>(bitmapWidth), static_cast<float>(bitmapHeight)};
        glyph.bearing = {static_cast<float>(slot->bitmap_left), static_cast<float>(slot->bitmap_top)};
        glyph.advance = static_cast<float>(slot->advance.x >> 6);
        glyph.valid = true;

        m_glyphs[static_cast<std::size_t>(code - FirstGlyph)] = glyph;

        penX += bitmapWidth + GlyphPadding;
        rowHeight = std::max(rowHeight, bitmapHeight);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    m_loaded = true;
    return true;
}

void FontAtlas::Clear()
{
    m_glyphs = {};
    m_pixels.clear();
    m_sourcePath.clear();
    m_width = 0;
    m_height = 0;
    m_pixelSize = 0;
    m_ascent = 0.0f;
    m_lineHeight = 0.0f;
    m_loaded = false;
}

const Glyph* FontAtlas::FindGlyph(char character) const
{
    const int index = GlyphIndex(character);
    if (index < 0 || index >= GlyphCount)
    {
        return nullptr;
    }

    const Glyph& glyph = m_glyphs[static_cast<std::size_t>(index)];
    return glyph.valid ? &glyph : nullptr;
}

const std::vector<std::uint8_t>& FontAtlas::Pixels() const
{
    return m_pixels;
}

int FontAtlas::Width() const
{
    return m_width;
}

int FontAtlas::Height() const
{
    return m_height;
}

int FontAtlas::PixelSize() const
{
    return m_pixelSize;
}

float FontAtlas::Ascent() const
{
    return m_ascent;
}

float FontAtlas::LineHeight() const
{
    return m_lineHeight;
}

const std::string& FontAtlas::SourcePath() const
{
    return m_sourcePath;
}

bool FontAtlas::IsLoaded() const
{
    return m_loaded;
}
}

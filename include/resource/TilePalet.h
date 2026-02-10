#ifndef TILE_PALET_H
#define TILE_PALET_H

#include <vector>
#include <cstdint>

#include "resource/TileColor.h"
#include "video/Buffer.h"

enum class ColorDepth
{
    UINT_32BIT = 32,
    UINT_15BIT = 16,
};

class TilePalet
{
    private:
        uint32_t m_maxColors;
        uint32_t m_currentSize = 0;
        ColorDepth m_colorDepth;
        Buffer   *m_buffer;

        // bool checkAccess(uint32_t x, uint32_t y);

    public:
        TilePalet(ColorDepth depth, uint32_t maxColors);
        ~TilePalet();
        TilePalet(const TilePalet&) = delete;

        TileColor getColor(uint32_t index);
        uint32_t addColor(TileColor color);

        const VkDescriptorBufferInfo getDescriptorBufferInfo();
};

#endif //TILE_PALET_H
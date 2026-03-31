#include "resource/TilePalet.h"

#include <SDL3/SDL_log.h>

uint32_t inline getIndexOffset(uint32_t index, ColorDepth depth)
{
    return index /** ((uint32_t)depth / 8)*/;
}

TilePalet::TilePalet(ColorDepth depth, uint32_t maxColors) 
    : m_colorDepth(depth), m_maxColors(maxColors)
{
    m_buffer = new Buffer(StorageBuffer, m_maxColors, ((uint32_t)m_colorDepth / 8));
}


TilePalet::~TilePalet()
{
    delete m_buffer;
}


TileColor TilePalet::getColor(uint32_t index)
{
    TileColor color;
    if (index > m_currentSize)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Accessing the index %d of a palet of size %d.", index, m_currentSize);
        return color;
    }
    color.color = m_buffer->get(getIndexOffset(index, m_colorDepth));
    return color;
}

uint32_t TilePalet::addColor(TileColor color)
{
    for (size_t i = 0; i < m_currentSize; i++)
    {
        if (color.color == getColor(i).color)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Tryign to add the same color %x twice in a palet.", color);
            return i;
        }
    }
    uint32_t index = m_currentSize;
    m_currentSize++;
    m_buffer->set(getIndexOffset(index, m_colorDepth), color.color);
    return index;
}

const VkDescriptorBufferInfo TilePalet::getDescriptorBufferInfo()
{
    VkDescriptorBufferInfo paletInfo = {};
    paletInfo.buffer = m_buffer->getBuffer();
    paletInfo.offset = 0;
    paletInfo.range = m_buffer->getSize();
    return paletInfo;
}

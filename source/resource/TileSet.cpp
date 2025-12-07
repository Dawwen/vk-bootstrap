#include "resource/TileSet.h"

TileSet::TileSet(uint32_t width, uint32_t maxSize)
{
    m_width = width;
    m_max_size = maxSize;
    m_buffer = new Buffer(StagingBuffer, width * width * maxSize, sizeof(uint32_t));
}

TileSet::~TileSet()
{
    delete m_buffer;
}

uint32_t inline getOffset(uint32_t index, uint32_t x, uint32_t y, uint32_t width)
{
    return (index * width * width) + (y * width + x); 
}

bool TileSet::checkAccess(uint32_t index, uint32_t x, uint32_t y)
{
    bool error = false;
    if (index >= m_max_size)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Trying to access a tile with index %d in a TileSet of size=%d.", index, m_max_size);
        error = true;
    }
    if (x >= m_width)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Trying to access x=%d of a TileSet of size width=%d.", x, m_width);
        error = true;
    }
    if (y >= m_width)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Trying to access y=%d of a TileSet of size height=%d.", y, m_width);
        error = true;
    }
    return error;
}

uint32_t TileSet::get(uint32_t index, uint32_t x, uint32_t y)
{
    if (checkAccess(index, x, y))
    {
        return 0;
    }
    
    return m_buffer->get(getOffset(index, x, y, m_width));
}

void TileSet::set(uint32_t index, uint32_t x, uint32_t y, uint32_t value)
{
    if (checkAccess(index, x, y))
    {
        return;
    }
    
    m_buffer->set(getOffset(index, x, y, m_width), value);
}
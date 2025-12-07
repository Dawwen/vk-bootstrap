#include "resource/TileMap.h"

#include <SDL3/SDL_log.h>

TileMap::TileMap(uint32_t x, uint32_t y, uint32_t size)
{
    width = x;
    height = y;
    tile_size = size;
    buffer = new Buffer(BufferType::StagingBuffer, x * y * size, sizeof(uint32_t));
    data = new uint32_t[x*y]();
}

TileMap::~TileMap()
{
    delete data;
    delete buffer;
}

bool TileMap::checkAccess(uint32_t x, uint32_t y)
{
    bool error = false;
    if (x >= width)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Trying to access x=%d of a Tilemap of size width=%d.", x, width);
        error = true;
    }
    if (y >= height)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Trying to access y=%d of a Tilemap of size height=%d.", y, height);
        error = true;
    }
    return error;
}

uint32_t TileMap::get(uint32_t x, uint32_t y)
{
    if (checkAccess(x, y))
    {
        return 0;
    }
    
    return data[width * y + x];
}

void TileMap::set(uint32_t x, uint32_t y, uint32_t value)
{
    if (checkAccess(x, y))
    {
        return;
    }
    
    data[width * y + x] = value;
}

void TileMap::updateBuffer()
{
    bool result = buffer->copyToStagingBuffer(data, width*height*sizeof(uint32_t));

    if (result)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not update Staging buffer of the tilemap");
    }
}

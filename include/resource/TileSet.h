#ifndef TILE_SET_H
#define TILE_SET_H

#include <cstdint>

#include <video/Buffer.h>

class TileSet
{
private:

    bool checkAccess(uint32_t index, uint32_t x, uint32_t y);

    uint32_t m_width;
    uint32_t m_max_size;
    Buffer   *m_buffer;

public:
    TileSet(uint32_t width, uint32_t maxSize);
    ~TileSet();
    
    uint32_t get(uint32_t index, uint32_t x, uint32_t y);
    void set(uint32_t index, uint32_t x, uint32_t y, uint32_t value);

};

#endif // TILE_SET_H
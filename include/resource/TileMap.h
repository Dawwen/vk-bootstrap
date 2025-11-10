#ifndef TILE_MAP_H
#define TILE_MAP_H

#include <cstdint>

#include "video/Buffer.h"

class TileMap
{
    private:
        uint32_t width;
        uint32_t height;
        uint32_t *data;
        Buffer* buffer; 

        bool checkAccess(uint32_t x, uint32_t y);

    public:
        TileMap(uint32_t x, uint32_t y);
        ~TileMap();
        TileMap(const TileMap&) = delete;

        uint32_t getWidth()     {return width;}
        uint32_t getHeight()    {return height;}
        uint32_t get(uint32_t x, uint32_t y);
        void set(uint32_t x, uint32_t y, uint32_t value);
        void updateBuffer();
};

#endif //TILE_MAP_H
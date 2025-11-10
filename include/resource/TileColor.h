#ifndef TILE_COLOR_H
#define TILE_COLOR_H

#include <cstdint>

// TODO: Move to allow different type of RGB 
union TileColor
{
    uint32_t color;
    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
};


#endif //TILE_COLOR_H
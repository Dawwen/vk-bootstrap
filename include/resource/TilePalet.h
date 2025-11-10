#ifndef TILE_PALET_H
#define TILE_PALET_H

#include <vector>
#include <cstdint>

#include "resource/TileColor.h"

class TilePalet
{
    private:
        std::vector<TileColor> palet;

        // bool checkAccess(uint32_t x, uint32_t y);

    public:
        TilePalet(/* Choosing Color Depth*/);
        ~TilePalet();
        TilePalet(const TilePalet&) = delete;

        TileColor getColor(uint32_t index);
        uint32_t addColor(TileColor color);
};

#endif //TILE_PALET_H
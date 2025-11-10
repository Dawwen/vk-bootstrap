#include "resource/TilePalet.h"

#include <SDL3/SDL_log.h>

TilePalet::TilePalet()
{
}


TilePalet::~TilePalet()
{
}


TileColor TilePalet::getColor(uint32_t index)
{
    if (index > palet.size())
    {
        TileColor color;
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Accessing the index %d of a palet of size %d.", index, palet.size());
        return color;
    }
    return palet[index];    
}

uint32_t TilePalet::addColor(TileColor color)
{
    for (size_t i = 0; i < palet.size(); i++)
    {
        if (color.color == palet[i].color)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Tryign to add the same color %x twice in a palet.", color);
            return i;
        }
    }
    uint32_t index = palet.size();
    palet.push_back(color);
    return index;
}
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>

#include "video/Renderer.h"
#include "video/Vertex.h"

#include <vector>
#include <iostream>

const uint32_t SCREEN_WIDTH = 120;
const uint32_t SCREEN_HEIGHT = 120;


const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, //2, 3, 0
};


int main(int argc, char const *argv[])
{
    Renderer renderer;

    if (renderer.init(SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to init Renderer");
        return true;
    }

    renderer.createVertexBuffer(vertices);
    renderer.createIndicesBuffer(indices);

    renderer.recordCommandBuffer();
    SDL_Event event;
    while (event.type != SDL_EVENT_QUIT)
    {
        SDL_PollEvent(&event);
        if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            renderer.resize();
            renderer.recordCommandBuffer();
        }
        
        int res = renderer.drawFrame();
        if (res != 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to draw frame ");
            return true;
        }
    }

    return 0;
}

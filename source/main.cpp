#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>

#include "video/Renderer.h"
#include "video/Vertex.h"

#include <vector>

const uint32_t SCREEN_WIDTH = 800;
const uint32_t SCREEN_HEIGHT = 600;


const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};


int main(int argc, char const *argv[])
{
    Renderer renderer;

    renderer.init(SCREEN_WIDTH, SCREEN_HEIGHT);

    renderer.createVertexBuffer(vertices);
    renderer.createIndicesBuffer(indices);
    SDL_Event event;
    while (event.type != SDL_EVENT_QUIT)
    {
        SDL_PollEvent(&event);
        int res = renderer.drawFrame();
        if (res != 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to draw frame ");
            return true;
        }
    }

    return 0;
}

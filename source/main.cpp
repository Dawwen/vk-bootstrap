#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>

#include <chrono>
#include <vector>
#include <iostream>

#include "video/Renderer.h"
#include "video/Vertex.h"
#include "video/UniformBuffer.h"

const uint32_t SCREEN_WIDTH = 120;
const uint32_t SCREEN_HEIGHT = 120;


const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

void calculateNewUniformBuffer(UniformBufferObject& ubo, uint32_t width, uint32_t height)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), width / (float) height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
}

int main(int argc, char const *argv[])
{
    Renderer renderer;
    UniformBufferObject ubo = {};
    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);
    ubo.proj = glm::mat4(1.0f);

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
        // calculateNewUniformBuffer(ubo, SCREEN_WIDTH, SCREEN_HEIGHT);
        renderer.updateUniformBuffer(ubo);
        int res = renderer.drawFrame();
        if (res != 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to draw frame ");
            return true;
        }
    }

    return 0;
}

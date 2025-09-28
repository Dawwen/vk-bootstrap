#ifndef RENDERER_H
#define RENDERER_H

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_mem_alloc.h>

#include <vector>

#include "video/renderer_struct.h"
#include "video/render_data.h"
#include "video/Vertex.h"
#include "video/Buffer.h"

class Renderer
{
    private:
        VulkanContext m_ctx;
        RenderData m_render_data;
        
        Buffer* m_index_buffer;

    public:
        Renderer(/* args */);
        ~Renderer();
        bool init(uint32_t width, uint32_t height);
        bool drawFrame();
        bool resize();

        bool createVertexBuffer(const std::vector<Vertex>& vertices);
        bool createIndicesBuffer(const std::vector<uint16_t>& indices);
        bool recordCommandBuffer(const std::vector<uint16_t>& indicies);

};

#endif //RENDERER_H
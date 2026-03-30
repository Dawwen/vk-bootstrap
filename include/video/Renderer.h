#ifndef RENDERER_H
#define RENDERER_H

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_mem_alloc.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"

#include <vector>

#include "video/renderer_struct.h"
#include "video/render_data.h"
#include "video/Vertex.h"
#include "video/Buffer.h"
#include "video/UniformBuffer.h"

#include "resource/TileSet.h"
#include "resource/TilePalet.h"


class Renderer
{
    private:
        VulkanContext m_ctx;
        RenderData m_render_data;
        
    public:
        Renderer(/* args */);
        ~Renderer();
        bool init(uint32_t width, uint32_t height);
        bool drawFrame();
        bool resize();

        bool createVertexBuffer(const std::vector<Vertex>& vertices);
        bool createIndicesBuffer(const std::vector<uint16_t>& indices);
        bool createUniformBuffers(size_t buffer_size);
        bool updateUniformBuffer(const UniformBufferObject& ubo);

        bool recordCommandBuffer();
        void createTileTexture(VkImage &texture, VkImageView &textureView, VmaAllocation &textureAllocation, VmaAllocationInfo& allocationInfo, TileSet &tileset, TilePalet &palet);
        void cleanTileTexture(VkImage& texture, VkImageView& textureView, VmaAllocation& textureAllocation);
        bool renderTileSet(Buffer& buffer, VkImage &texture, VkImageView& textureView, TileSet &tileset, TilePalet &palet);
};

#endif //RENDERER_H
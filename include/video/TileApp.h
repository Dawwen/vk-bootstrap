#ifndef TILE_APP_H
#define TILE_APP_H

#include "video/VulkanApp.h"
#include "resource/TileSet.h"
#include "resource/TilePalet.h"

#include <vk_mem_alloc.h>
#include <memory>
using std::shared_ptr;

#include <vector>


struct Resource
{
    TileSet& tileset;
    TilePalet& palet;
    VkImage texture;
    VkImageView textureView;
    VmaAllocation textureAllocation;
    VmaAllocationInfo allocationInfo;
    VkDescriptorSet computeDescriptorSet;
} typedef Resource_t;

class TileApp : public VulkanApp
{
    public:
        TileApp(shared_ptr<VulkanContext> ctx);
        ~TileApp();

        bool init();
        bool run(Buffer& buffer);
        bool addResource(TileSet& tileSet, TilePalet& palet);

    private:
        static constexpr uint32_t MAX_RESOURCES = 16;

        std::vector<Resource_t> resources;

        VkPipeline computePipeline;
        VkPipelineLayout computePipelineLayout;

        VkDescriptorSetLayout computeDescriptorSetLayout;
        VkDescriptorPool computeDescriptorPool;
};

#endif //TILE_APP_H
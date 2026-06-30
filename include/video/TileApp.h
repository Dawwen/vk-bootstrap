#ifndef TILE_APP_H
#define TILE_APP_H

#include "video/VulkanApp.h"
#include "resource/TileSet.h"
#include "resource/TilePalet.h"

#include <vk_mem_alloc.h>
#include <memory>
using std::shared_ptr;

class TileApp : public VulkanApp
{
    public:
        TileApp(shared_ptr<VulkanContext> ctx, TileSet& tileset, TilePalet& palet);
        ~TileApp();

        bool init();
        bool run(Buffer& buffer);

    private:
        // Add any TileApp specific members here
        VkImage texture;
        VkImageView textureView;
        VmaAllocation textureAllocation;
        VmaAllocationInfo allocationInfo;
        TileSet& tileset;
        TilePalet& palet;


        VkPipeline computePipeline;
        VkPipelineLayout computePipelineLayout;

        VkDescriptorSetLayout computeDescriptorSetLayout;
        VkDescriptorSet computeDescriptorSet;
        VkDescriptorPool computeDescriptorPool;
};

#endif //TILE_APP_H
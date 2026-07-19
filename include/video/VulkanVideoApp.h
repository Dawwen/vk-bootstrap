#ifndef VULKAN_VIDEO_APP_H
#define VULKAN_VIDEO_APP_H

#include "video/VulkanApp.h"
#include "video/Buffer.h"

struct BaseVideoVulkan
{
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkDescriptorSet> descriptor_sets;
    vkb::Swapchain swapchain;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;

} typedef BaseVideoVulkan_t;

class VulkanVideoApp : public VulkanApp
{
    public:
        VulkanVideoApp(shared_ptr<VulkanContext> ctx);
        bool init(uint32_t width, uint32_t height);
        bool render();
        bool resize();

        ~VulkanVideoApp();

    protected:
        BaseVideoVulkan_t m_base_video_vulkan;

    private:
        bool create_swapchain(uint32_t width, uint32_t height);
        bool recreate_swapchain(uint32_t width, uint32_t height);
        bool create_graphics_pipeline();
        bool create_pipeline_layout();
        bool create_uniform_buffers();
        bool create_descriptor_set();
        bool create_descriptor_pool();
        bool create_sync_objects();
        void update_uniform_buffer(uint32_t currentImage);
        bool recordCommandBuffers(uint32_t imageIndex);

        Buffer* m_vertex_buffer;
        Buffer* m_index_buffer;
        std::vector<Buffer*> m_uniform_buffers;

        uint32_t m_current_frame = 0;

        uint32_t width = 800;
        uint32_t height = 600;

};

#endif // VULKAN_VIDEO_APP_H
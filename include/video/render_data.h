#ifndef RENDER_DATA_H
#define RENDER_DATA_H

#include <vector>
#include "video/Buffer.h"

struct RenderData {

    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkFramebuffer> framebuffers;

    Buffer* vertex_buffer = nullptr;
    Buffer* index_buffer  = nullptr;

    //TODO fix by using buffer class
    std::vector<Buffer*> uniformBuffers;

    VkRenderPass render_pass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    std::vector<VkCommandBuffer> command_buffers;

    std::vector<VkSemaphore> available_semaphores;
    std::vector<VkSemaphore> finished_semaphore;
    std::vector<VkFence> in_flight_fences;
    std::vector<VkFence> image_in_flight;
    size_t current_frame = 0;
};

#endif //RENDER_DATA_H
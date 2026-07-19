#include "video/VulkanVideoApp.h"
#include "video/Vertex.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <cstring>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};


void transition_image_layout(
        VulkanContext&          ctx,
        VkImage                 swapChainImage,
	    VkCommandBuffer         commandBuffer,
	    VkImageLayout           old_layout,
	    VkImageLayout           new_layout,
	    VkAccessFlags           src_access_mask,
	    VkAccessFlags           dst_access_mask,
	    VkPipelineStageFlags    src_stage_mask,
	    VkPipelineStageFlags    dst_stage_mask)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = src_access_mask;
    barrier.dstAccessMask = dst_access_mask;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = swapChainImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    ctx.disp.cmdPipelineBarrier(commandBuffer, src_stage_mask, dst_stage_mask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

bool create_gpu_buffer(VulkanContext& ctx, BufferType type, Buffer** buffer, const void *content, uint32_t number_of_elements, size_t size_per_element)
{
    size_t buffer_size = number_of_elements * size_per_element;
    // Creating the staging buffer
    Buffer* staging_buffer;
    try
    {
        staging_buffer = new Buffer(BufferType::StagingBuffer, number_of_elements, size_per_element);
    }
    catch(const std::runtime_error& e)
    {
        return true;
    }
 
    // Creating the actual buffer
    try
    {
        *buffer = new Buffer(type, number_of_elements, size_per_element);
    }
    catch(const std::exception& e)
    {
        delete staging_buffer;
        return true;
    }

    // Copying it to staging buffer
    if (staging_buffer->copyToStagingBuffer(content, buffer_size))
    {
        delete *buffer;
        delete staging_buffer;
        buffer = nullptr;
        return true;
    }

    // Copying it to GPU buffer
    if (Buffer::copyTo(ctx, *staging_buffer, **buffer))
    {
        delete *buffer;
        delete staging_buffer;
        buffer = nullptr;
        return true;
    }

    delete staging_buffer;

    return false;
}

// Swapchain

bool VulkanVideoApp::create_swapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchain_builder{ m_ctx->device };
    auto swap_ret = swapchain_builder.set_desired_extent(width, height).set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR).set_old_swapchain(m_base_video_vulkan.swapchain).build();
    if (!swap_ret)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, swap_ret.error().message().c_str());
        return true;
    }
    vkb::destroy_swapchain(m_base_video_vulkan.swapchain);
    m_base_video_vulkan.swapchain = swap_ret.value();
    m_base_video_vulkan.swapchain_images = m_base_video_vulkan.swapchain.get_images().value();
    m_base_video_vulkan.swapchain_image_views = m_base_video_vulkan.swapchain.get_image_views().value();

    m_base_video_vulkan.command_buffers.resize(m_base_video_vulkan.swapchain_images.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_ctx->graphics_command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_base_video_vulkan.command_buffers.size();

    if (m_ctx->disp.allocateCommandBuffers(&allocInfo, m_base_video_vulkan.command_buffers.data()) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to allocate command buffers");
        return true;
    }

    return false;
}

bool VulkanVideoApp::recreate_swapchain(uint32_t width, uint32_t height)
{
    m_ctx->disp.deviceWaitIdle();

    
    m_base_video_vulkan.swapchain.destroy_image_views(m_base_video_vulkan.swapchain_image_views);
    
    if (create_swapchain(width, height))  return true;
    m_ctx->disp.resetCommandPool(m_ctx->graphics_command_pool, 0);
    
    return false;
}

bool VulkanVideoApp::create_pipeline_layout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (m_ctx->disp.createDescriptorSetLayout(&layoutInfo, nullptr, &m_base_video_vulkan.descriptor_set_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    return false;
}

bool VulkanVideoApp::create_graphics_pipeline()
{

    // Vertex input

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Shaders
    VkShaderModule vertShaderModule = createShaderModule(*m_ctx, readFile("../shaders/triangle.vert.spv"));
    VkShaderModule fragShaderModule = createShaderModule(*m_ctx, readFile("../shaders/triangle.frag.spv"));

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Viewport, scissor and rasterizer

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // VkRect2D scissor = {};
    // scissor.offset = { 0, 0 };
    // scissor.extent = m_base_video_vulkan.swapchain.extent;

    // VkViewport viewport = {};
    // viewport.x = 0.0f;
    // viewport.y = 0.0f;
    // viewport.width = (float)m_base_video_vulkan.swapchain.extent.width;
    // viewport.height = (float)m_base_video_vulkan.swapchain.extent.height;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.pScissors = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Multisampling

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Pipeline creation

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &m_base_video_vulkan.descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;

    m_ctx->disp.createPipelineLayout(&pipeline_layout_info, nullptr, &m_base_video_vulkan.pipeline_layout);

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &m_base_video_vulkan.swapchain.image_format;


    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shader_stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_base_video_vulkan.pipeline_layout;
    pipelineInfo.renderPass = VK_NULL_HANDLE; // Using dynamic rendering, so no render pass is needed
    pipelineInfo.pNext = &pipelineRenderingCreateInfo;

    if (m_ctx->disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_base_video_vulkan.graphics_pipeline) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create graphics pipeline");
        return true;
    }

    m_ctx->disp.destroyShaderModule(fragShaderModule, nullptr);
    m_ctx->disp.destroyShaderModule(vertShaderModule, nullptr);

    return false;
}

bool VulkanVideoApp::recordCommandBuffers(uint32_t imageIndex)
{
    VkCommandBuffer currentCommandBuffer = m_base_video_vulkan.command_buffers[imageIndex];
    VkImage swapChainImage = m_base_video_vulkan.swapchain_images[imageIndex];
    VkImageView swapChainImageView = m_base_video_vulkan.swapchain_image_views[imageIndex];
    VkDescriptorSet descriptorSet = m_base_video_vulkan.descriptor_sets[imageIndex];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;

    m_ctx->disp.beginCommandBuffer(currentCommandBuffer, &beginInfo);

    transition_image_layout(*m_ctx, swapChainImage, currentCommandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    
    VkClearValue clearColor = { 0.1f, 0.1f, 0.3f, 1.0f };
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapChainImageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearColor;

    VkRenderingInfo renderingInfo = {
    .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea           = {.offset = {0, 0}, .extent = m_base_video_vulkan.swapchain.extent},
    .layerCount           = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &colorAttachment};

    m_ctx->disp.cmdBeginRendering(currentCommandBuffer, &renderingInfo);

    m_ctx->disp.cmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_base_video_vulkan.graphics_pipeline);
    
    VkViewport viewport{0.0f, 0.0f, (float)m_base_video_vulkan.swapchain.extent.width, (float)m_base_video_vulkan.swapchain.extent.height, 0.0f, 1.0f};
    VkRect2D scissor{{0, 0}, m_base_video_vulkan.swapchain.extent};
    m_ctx->disp.cmdSetViewport(currentCommandBuffer, 0, 1, &viewport);
    m_ctx->disp.cmdSetScissor(currentCommandBuffer, 0, 1, &scissor);

    m_ctx->disp.cmdBindDescriptorSets(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_base_video_vulkan.pipeline_layout, 0, 1, &descriptorSet, 0, nullptr);
    VkDeviceSize offset = 0;
    m_ctx->disp.cmdBindVertexBuffers(currentCommandBuffer, 0, 1, &m_vertex_buffer->getBuffer(), &offset);
    m_ctx->disp.cmdBindIndexBuffer(currentCommandBuffer, m_index_buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);

    m_ctx->disp.cmdDrawIndexed(currentCommandBuffer, 6, 1, 0, 0, 0);

    m_ctx->disp.cmdEndRendering(currentCommandBuffer);

    // After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
    transition_image_layout(
        *m_ctx,
        swapChainImage,
        currentCommandBuffer,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
    );

    m_ctx->disp.endCommandBuffer(currentCommandBuffer);


    return true;
}

bool VulkanVideoApp::create_uniform_buffers()
{
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);
    m_uniform_buffers.resize(m_base_video_vulkan.swapchain_images.size());

    for (size_t i = 0; i < m_base_video_vulkan.swapchain_images.size(); i++)
    {
        try
        {
            m_uniform_buffers[i] = new Buffer(BufferType::UniformBuffer, 1, buffer_size);
        }
        catch(...)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create uniform buffer %zu", i);
            return true;
        }
    }

    return false;
}

bool VulkanVideoApp::create_descriptor_pool()
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(m_base_video_vulkan.swapchain_images.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(m_base_video_vulkan.swapchain_images.size());

    if (m_ctx->disp.createDescriptorPool(&poolInfo, nullptr, &m_base_video_vulkan.descriptor_pool) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create descriptor pool");
        return true;
    }

    return false;
}

bool VulkanVideoApp::create_descriptor_set()
{
    uint32_t num_images = static_cast<uint32_t>(m_base_video_vulkan.swapchain_images.size());
    m_base_video_vulkan.descriptor_sets.resize(num_images);

    std::vector<VkDescriptorSetLayout> layouts(num_images, m_base_video_vulkan.descriptor_set_layout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_base_video_vulkan.descriptor_pool;
    allocInfo.descriptorSetCount = num_images;
    allocInfo.pSetLayouts = layouts.data();

    if (m_ctx->disp.allocateDescriptorSets(&allocInfo, m_base_video_vulkan.descriptor_sets.data()) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to allocate descriptor sets");
        return true;
    }

    for (size_t i = 0; i < num_images; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniform_buffers[i]->getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_base_video_vulkan.descriptor_sets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        m_ctx->disp.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
    }

    return false;
}


const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

VulkanVideoApp::VulkanVideoApp(shared_ptr<VulkanContext> ctx) : VulkanApp(ctx), m_base_video_vulkan({}), m_vertex_buffer(nullptr), m_index_buffer(nullptr)
{
    size_t buffer_size = sizeof(vertices[0]) * vertices.size();;
    create_gpu_buffer(*m_ctx, BufferType::VertexBuffer, &m_vertex_buffer, static_cast<const void*>(vertices.data()), vertices.size(), sizeof(vertices[0]));

    buffer_size = sizeof(indices[0]) * indices.size();
    create_gpu_buffer(*m_ctx, BufferType::IndiceBuffer, &m_index_buffer, static_cast<const void*>(indices.data()), indices.size(), sizeof(indices[0]));
}

bool VulkanVideoApp::init(uint32_t width, uint32_t height)
{
    width = width;
    height = height;

    if (create_swapchain(width, height))    return true;
    if (create_uniform_buffers())           return true;
    if (create_descriptor_pool())           return true;
    if (create_pipeline_layout())           return true;
    if (create_descriptor_set())            return true;
    if (create_graphics_pipeline())         return true;
    if (create_sync_objects())              return true;


    return false;
}

bool VulkanVideoApp::create_sync_objects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (m_ctx->disp.createSemaphore(&semaphoreInfo, nullptr, &m_base_video_vulkan.image_available_semaphore) != VK_SUCCESS ||
        m_ctx->disp.createSemaphore(&semaphoreInfo, nullptr, &m_base_video_vulkan.render_finished_semaphore) != VK_SUCCESS ||
        m_ctx->disp.createFence(&fenceInfo, nullptr, &m_base_video_vulkan.in_flight_fence) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create synchronization objects");
        return true;
    }

    return false;
}

bool VulkanVideoApp::resize()
{
    int width, height;
    bool result = SDL_GetWindowSizeInPixels(m_ctx->window, &width, &height);
    recreate_swapchain(width, height);
    return result;
}

void VulkanVideoApp::update_uniform_buffer(uint32_t currentImage)
{
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f/(currentImage+1));
    ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), m_base_video_vulkan.swapchain.extent.width / (float)m_base_video_vulkan.swapchain.extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(m_uniform_buffers[currentImage]->getMappedData(), &ubo, sizeof(ubo));
}

bool VulkanVideoApp::render()
{
    m_ctx->disp.waitForFences(1, &m_base_video_vulkan.in_flight_fence, VK_TRUE, UINT64_MAX);
    m_ctx->disp.resetFences(1, &m_base_video_vulkan.in_flight_fence);

    uint32_t imageIndex;
    VkSwapchainKHR swapchain_handle = m_base_video_vulkan.swapchain;
    VkResult result = m_ctx->disp.acquireNextImageKHR(swapchain_handle, UINT64_MAX, m_base_video_vulkan.image_available_semaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swapchain(m_base_video_vulkan.swapchain.extent.width, m_base_video_vulkan.swapchain.extent.height);
        return true;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to acquire swapchain image");
        return true;
    }

    m_ctx->disp.deviceWaitIdle();

    update_uniform_buffer(imageIndex);

    m_ctx->disp.resetCommandBuffer(m_base_video_vulkan.command_buffers[imageIndex], 0);
    recordCommandBuffers(imageIndex);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {m_base_video_vulkan.render_finished_semaphore};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_base_video_vulkan.image_available_semaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_base_video_vulkan.command_buffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (m_ctx->disp.queueSubmit(m_ctx->graphics_queue, 1, &submitInfo, m_base_video_vulkan.in_flight_fence) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to submit draw command buffer");
        return true;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_handle;
    presentInfo.pImageIndices = &imageIndex;

    result = m_ctx->disp.queuePresentKHR(m_ctx->present_queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreate_swapchain(m_base_video_vulkan.swapchain.extent.width, m_base_video_vulkan.swapchain.extent.height);
    }

    m_current_frame = imageIndex;
    return true;
}
VulkanVideoApp::~VulkanVideoApp()
{
    m_ctx->disp.deviceWaitIdle();

    delete m_vertex_buffer;
    delete m_index_buffer;

    for (auto& buffer : m_uniform_buffers)
        delete buffer;

    m_ctx->disp.destroySemaphore(m_base_video_vulkan.image_available_semaphore, nullptr);
    m_ctx->disp.destroySemaphore(m_base_video_vulkan.render_finished_semaphore, nullptr);
    m_ctx->disp.destroyFence(m_base_video_vulkan.in_flight_fence, nullptr);

    m_ctx->disp.destroyDescriptorPool(m_base_video_vulkan.descriptor_pool, nullptr);
    m_ctx->disp.destroyPipeline(m_base_video_vulkan.graphics_pipeline, nullptr);
    m_ctx->disp.destroyPipelineLayout(m_base_video_vulkan.pipeline_layout, nullptr);
    m_ctx->disp.destroyDescriptorSetLayout(m_base_video_vulkan.descriptor_set_layout, nullptr);
    m_base_video_vulkan.swapchain.destroy_image_views(m_base_video_vulkan.swapchain_image_views);
    vkb::destroy_swapchain(m_base_video_vulkan.swapchain);
}
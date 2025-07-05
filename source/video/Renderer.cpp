#include "video/Renderer.h"

#include <fstream>


const int MAX_FRAMES_IN_FLIGHT = 3;
#define SHADER_FOLDER "../shaders/"

// Util function

std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(file_size));

    file.close();

    return buffer;
}

// SDL functions

SDL_Window* create_window(const char* window_name, uint32_t width, uint32_t height, bool resize = true) {
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't initialize SDL: %s", SDL_GetError());
    }

    SDL_Window* window = SDL_CreateWindow("examples/renderer/clear", width, height, SDL_WINDOW_VULKAN);
    if (window == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create Vulkan window: %s", SDL_GetError());
    }
    return window;
}

VkSurfaceKHR create_surface(VkInstance instance, SDL_Window* window, VkAllocationCallbacks* allocator = nullptr) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    auto res = SDL_Vulkan_CreateSurface(window, instance, allocator, &surface);
    if (!res)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Could'n create SDL Surface");
        surface = VK_NULL_HANDLE;
    }
    return surface;
}

void destroy_window(SDL_Window* window)
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// Vulkan functions

bool device_initialization(VulkanContext& ctx, uint32_t width, uint32_t height)
{
    ctx.window = create_window("Vulkan Triangle", width, height, true);

    vkb::InstanceBuilder instance_builder;
    auto instance_ret = instance_builder.require_api_version(1,3,0).request_validation_layers().build();
    if (!instance_ret)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, instance_ret.error().message().c_str());
        return true;
    }

    ctx.instance = instance_ret.value();
    ctx.inst_disp = ctx.instance.make_table();

    ctx.surface = create_surface(ctx.instance, ctx.window);
    if (ctx.surface == VK_NULL_HANDLE)
    {
        return true;
    }
    

    vkb::PhysicalDeviceSelector phys_device_selector(ctx.instance);
    auto phys_device_ret = phys_device_selector.set_surface(ctx.surface).select();
    if (!phys_device_ret)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, phys_device_ret.error().message().c_str());
        return true;
    }
    vkb::PhysicalDevice physical_device = phys_device_ret.value();

    vkb::DeviceBuilder device_builder{ physical_device };
    auto device_ret = device_builder.build();
    if (!device_ret)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, device_ret.error().message().c_str());
        return true;
    }
    ctx.device = device_ret.value();
    ctx.disp = ctx.device.make_table();

    return false;
}

bool create_swapchain(VulkanContext& ctx, uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchain_builder{ ctx.device };
    auto swap_ret = swapchain_builder.set_desired_extent(width, height).set_old_swapchain(ctx.swapchain).build();
    if (!swap_ret)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, swap_ret.error().message().c_str());
        return true;
    }
    vkb::destroy_swapchain(ctx.swapchain);
    ctx.swapchain = swap_ret.value();
    return false;
}

bool get_queues(VulkanContext& ctx, RenderData& data)
{
    auto gq = ctx.device.get_queue(vkb::QueueType::graphics);
    if (!gq.has_value())
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, gq.error().message().c_str());
        return true;
    }
    data.graphics_queue = gq.value();

    auto pq = ctx.device.get_queue(vkb::QueueType::present);
    if (!pq.has_value())
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, pq.error().message().c_str());
        return true;
    }
    data.present_queue = pq.value();
    return false;
}

bool create_render_pass(VulkanContext& ctx, RenderData& data)
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = ctx.swapchain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (ctx.disp.createRenderPass(&render_pass_info, nullptr, &data.render_pass) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create render pass\n");
        return true;
    }

    return false;
}

VkShaderModule createShaderModule(VulkanContext& ctx, const std::vector<char>& code)
{
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (ctx.disp.createShaderModule(&create_info, nullptr, &shaderModule) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to create shader module");
        return VK_NULL_HANDLE; // failed to create shader module
    }

    return shaderModule;
}

bool create_graphics_pipeline(VulkanContext& ctx, RenderData& data)
{
    std::vector<char> vert_code;
    std::vector<char> frag_code;
    try
    {
        vert_code = readFile(std::string(SHADER_FOLDER) + "/triangle.vert.spv");
        frag_code = readFile(std::string(SHADER_FOLDER) + "/triangle.frag.spv");
    }
    catch(const std::exception& e)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to read shader files");
        return true;
    }

    VkShaderModule vert_module = createShaderModule(ctx, vert_code);
    VkShaderModule frag_module = createShaderModule(ctx, frag_code);
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create shader module");
        return true;
    }

    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_module;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage_info, frag_stage_info };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertex_input_info.pVertexBindingDescriptions = &bindingDescription;
    vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)ctx.swapchain.extent.width;
    viewport.height = (float)ctx.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = ctx.swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &colorBlendAttachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 0;

    if (ctx.disp.createPipelineLayout(&pipeline_layout_info, nullptr, &data.pipeline_layout) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create pipeline layout");
        return true;
    }

    std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_info.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.layout = data.pipeline_layout;
    pipeline_info.renderPass = data.render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (ctx.disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &data.graphics_pipeline) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create pipline");
        return true;
    }

    ctx.disp.destroyShaderModule(frag_module, nullptr);
    ctx.disp.destroyShaderModule(vert_module, nullptr);
    return 0;
}

bool create_framebuffers(VulkanContext& ctx, RenderData& data)
{
    data.swapchain_images = ctx.swapchain.get_images().value();
    data.swapchain_image_views = ctx.swapchain.get_image_views().value();
    data.framebuffers.resize(data.swapchain_image_views.size());

    for (size_t i = 0; i < data.swapchain_image_views.size(); i++)
    {
        VkImageView attachments[] = { data.swapchain_image_views[i] };

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = data.render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = ctx.swapchain.extent.width;
        framebuffer_info.height = ctx.swapchain.extent.height;
        framebuffer_info.layers = 1;

        if (ctx.disp.createFramebuffer(&framebuffer_info, nullptr, &data.framebuffers[i]) != VK_SUCCESS)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create framebuffer");
            return true;
        }
    }
    return 0;
}

bool create_command_pool(VulkanContext& ctx, RenderData& data)
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = ctx.device.get_queue_index(vkb::QueueType::graphics).value();

    if (ctx.disp.createCommandPool(&pool_info, nullptr, &data.command_pool) != VK_SUCCESS)\
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create command pool");
        return true;
    }
    return false;
}

bool create_command_buffers(VulkanContext& ctx, RenderData& data, uint32_t indicesSize)
{
    data.command_buffers.resize(data.framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = data.command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)data.command_buffers.size();

    if (ctx.disp.allocateCommandBuffers(&allocInfo, data.command_buffers.data()) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to allocate command buffers");
        return true;
    }

    for (size_t i = 0; i < data.command_buffers.size(); i++)
    {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (ctx.disp.beginCommandBuffer(data.command_buffers[i], &begin_info) != VK_SUCCESS)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to begin recording command buffer");
            return true;
        }

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = data.render_pass;
        render_pass_info.framebuffer = data.framebuffers[i];
        render_pass_info.renderArea.offset = { 0, 0 };
        render_pass_info.renderArea.extent = ctx.swapchain.extent;
        VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clearColor;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)ctx.swapchain.extent.width;
        viewport.height = (float)ctx.swapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = ctx.swapchain.extent;

        ctx.disp.cmdSetViewport(data.command_buffers[i], 0, 1, &viewport);
        ctx.disp.cmdSetScissor(data.command_buffers[i], 0, 1, &scissor);

        VkDeviceSize offset = 0;
        ctx.disp.cmdBeginRenderPass(data.command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        ctx.disp.cmdBindPipeline(data.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, data.graphics_pipeline);
        ctx.disp.cmdBindVertexBuffers(data.command_buffers[i], 0, 1, &ctx.vertex_buffer, &offset);
        ctx.disp.cmdBindIndexBuffer(data.command_buffers[i], ctx.indices_buffer, 0, VK_INDEX_TYPE_UINT16);
        ctx.disp.cmdDrawIndexed(data.command_buffers[i], indicesSize, 1, 0, 0, 0);
        
        ctx.disp.cmdEndRenderPass(data.command_buffers[i]);

        if (ctx.disp.endCommandBuffer(data.command_buffers[i]) != VK_SUCCESS)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to record command buffer");
            return true;
        }
    }
    return false;
}

bool create_sync_objects(VulkanContext& ctx, RenderData& data)
{
    data.available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    data.finished_semaphore.resize(MAX_FRAMES_IN_FLIGHT);
    data.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    data.image_in_flight.resize(ctx.swapchain.image_count, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (ctx.disp.createSemaphore(&semaphore_info, nullptr, &data.available_semaphores[i]) != VK_SUCCESS ||
            ctx.disp.createSemaphore(&semaphore_info, nullptr, &data.finished_semaphore[i]) != VK_SUCCESS ||
            ctx.disp.createFence(&fence_info, nullptr, &data.in_flight_fences[i]) != VK_SUCCESS)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create sync objects");
            return true;
        }
    }
    return false;
}

void copyBuffer(VulkanContext& ctx, RenderData& data, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = data.command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(ctx.device.device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(data.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(data.graphics_queue);

    vkFreeCommandBuffers(ctx.device.device, data.command_pool, 1, &commandBuffer);
}

bool create_vertex_buffer(VulkanContext& ctx, RenderData& data, VmaAllocator& allocator, const std::vector<Vertex>& vertices)
{
    size_t buffer_size = sizeof(vertices[0]) * vertices.size();

    VkBufferCreateInfo buffer_create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_create_info.size = buffer_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;//VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo staging_allocation_create_info = {};
    staging_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    staging_allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer staging_buffer;
    VmaAllocation staging_allocation;
    VmaAllocationInfo allocation_info;

    // Staging buffer
    auto result = vmaCreateBuffer(allocator, &buffer_create_info, &staging_allocation_create_info, &staging_buffer, &staging_allocation, &allocation_info);
    if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create staging buffer");
        return true;
    }
    
    //TODO try me
    // vmaCopyMemoryToAllocation(allocator, vertices.data(), staging_allocation, 0, buffer_size);
    memcpy(allocation_info.pMappedData, vertices.data(), buffer_size);
    

    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo vertex_allocation_create_info = {};
    vertex_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    vertex_allocation_create_info.flags  = 0;

    vmaCreateBuffer(allocator, &buffer_create_info, &vertex_allocation_create_info, &ctx.vertex_buffer, &ctx.vertex_buffer_allocation, nullptr);
    
    copyBuffer(ctx, data, staging_buffer, ctx.vertex_buffer, buffer_size);

    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
    return false;
}


bool create_indices_buffer(VulkanContext& ctx, RenderData& data, VmaAllocator& allocator, const std::vector<uint16_t>& indices)
{
    size_t buffer_size = sizeof(indices[0]) * indices.size();

    VkBufferCreateInfo buffer_create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_create_info.size = buffer_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;//VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo staging_allocation_create_info = {};
    staging_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    staging_allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer staging_buffer;
    VmaAllocation staging_allocation;
    VmaAllocationInfo allocation_info;

    // Staging buffer
    auto result = vmaCreateBuffer(allocator, &buffer_create_info, &staging_allocation_create_info, &staging_buffer, &staging_allocation, &allocation_info);
    if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create staging buffer");
        return true;
    }
    
    // vmaCopyMemoryToAllocation(allocator, vertices.data(), staging_allocation, 0, buffer_size);
    memcpy(allocation_info.pMappedData, indices.data(), buffer_size);
    

    buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo indices_allocation_create_info = {};
    indices_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    indices_allocation_create_info.flags  = 0;

    vmaCreateBuffer(allocator, &buffer_create_info, &indices_allocation_create_info, &ctx.indices_buffer, &ctx.indices_buffer_allocation, nullptr);
    
    copyBuffer(ctx, data, staging_buffer, ctx.indices_buffer, buffer_size);

    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
    return false;
}



bool recreate_swapchain(VulkanContext& ctx, RenderData& data, uint32_t width, uint32_t height)
{
    ctx.disp.deviceWaitIdle();

    ctx.disp.destroyCommandPool(data.command_pool, nullptr);

    for (auto framebuffer : data.framebuffers)
    {
        ctx.disp.destroyFramebuffer(framebuffer, nullptr);
    }

    ctx.swapchain.destroy_image_views(data.swapchain_image_views);

    if (!create_swapchain(ctx, width, height))  return true;
    if (!create_framebuffers(ctx, data))        return true;
    if (!create_command_pool(ctx, data))        return true;
    if (!create_command_buffers(ctx, data, 6))  return true; //TODO FIX
    return false;
}

int draw_frame(VulkanContext& ctx, RenderData& data, uint32_t width, uint32_t height)
{
    ctx.disp.waitForFences(1, &data.in_flight_fences[data.current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index = 0;
    VkResult result = ctx.disp.acquireNextImageKHR(
        ctx.swapchain, UINT64_MAX, data.available_semaphores[data.current_frame], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return recreate_swapchain(ctx, data, width, height);
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to acquire swapchain image");
        return true;
    }

    if (data.image_in_flight[image_index] != VK_NULL_HANDLE)
    {
        ctx.disp.waitForFences(1, &data.image_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }
    data.image_in_flight[image_index] = data.in_flight_fences[data.current_frame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = { data.available_semaphores[data.current_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &data.command_buffers[image_index];

    VkSemaphore signal_semaphores[] = { data.finished_semaphore[data.current_frame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    ctx.disp.resetFences(1, &data.in_flight_fences[data.current_frame]);

    if (ctx.disp.queueSubmit(data.graphics_queue, 1, &submitInfo, data.in_flight_fences[data.current_frame]) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to submit draw command buffer");
        return true;
    }

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapChains[] = { ctx.swapchain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapChains;

    present_info.pImageIndices = &image_index;

    result = ctx.disp.queuePresentKHR(data.present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)\
    {
        return recreate_swapchain(ctx, data, width, height);
    }
    else if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to present swapchain image");
        return true;
    }

    data.current_frame = (data.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    return 0;
}


void cleanup(VulkanContext& ctx, RenderData& data, VmaAllocator allocator)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        ctx.disp.destroySemaphore(data.finished_semaphore[i], nullptr);
        ctx.disp.destroySemaphore(data.available_semaphores[i], nullptr);
        ctx.disp.destroyFence(data.in_flight_fences[i], nullptr);
    }

    ctx.disp.destroyCommandPool(data.command_pool, nullptr);

    for (auto framebuffer : data.framebuffers)
    {
        ctx.disp.destroyFramebuffer(framebuffer, nullptr);
    }

    ctx.disp.destroyPipeline(data.graphics_pipeline, nullptr);
    ctx.disp.destroyPipelineLayout(data.pipeline_layout, nullptr);
    ctx.disp.destroyRenderPass(data.render_pass, nullptr);

    ctx.swapchain.destroy_image_views(data.swapchain_image_views);

    vmaDestroyBuffer(allocator, ctx.vertex_buffer, ctx.vertex_buffer_allocation);
    vmaDestroyBuffer(allocator, ctx.indices_buffer, ctx.indices_buffer_allocation);

    vkb::destroy_swapchain(ctx.swapchain);
    vmaDestroyAllocator(allocator);
    vkb::destroy_device(ctx.device);
    vkb::destroy_surface(ctx.instance, ctx.surface);
    vkb::destroy_instance(ctx.instance);
    destroy_window(ctx.window);
}

// End util function


Renderer::Renderer(/* args */)
{
}

Renderer::~Renderer()
{
    m_ctx.disp.deviceWaitIdle();

    cleanup(m_ctx, m_render_data, allocator);
}

bool Renderer::init(uint32_t width, uint32_t height)
{
    if (!device_initialization(m_ctx, width, height)) return true;

    if (!allocatorCreated)
    {
        allocatorCreated = true;
        vulkanFunctions.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice = m_ctx.device.physical_device;
        allocatorCreateInfo.device = m_ctx.device.device;
        allocatorCreateInfo.instance = m_ctx.instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
        VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &allocator);

        if (result != VK_SUCCESS)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Could not create a VmaAllocator");
            return true;
        }
    }

    if (!create_swapchain           (m_ctx, width, height))     return true;
    if (!get_queues                 (m_ctx, m_render_data))     return true;
    if (!create_render_pass         (m_ctx, m_render_data))     return true;
    if (!create_graphics_pipeline   (m_ctx, m_render_data))     return true;
    if (!create_framebuffers        (m_ctx, m_render_data))     return true;
    if (!create_command_pool        (m_ctx, m_render_data))     return true;
    if (!create_command_buffers     (m_ctx, m_render_data, 6))  return true; //TODO FIX
    if (!create_sync_objects        (m_ctx, m_render_data))     return true;
    return false;
}

bool Renderer::recreateSwapChain(uint32_t width, uint32_t height)
{
    return recreate_swapchain(m_ctx, m_render_data, width, height);
}

bool Renderer::drawFrame()
{
    return draw_frame(m_ctx, m_render_data, 800, 600); //TODO fixme
}

bool Renderer::createVertexBuffer(const std::vector<Vertex> &vertices)
{
    return create_vertex_buffer(m_ctx, m_render_data, allocator, vertices);
}

bool Renderer::createIndicesBuffer(const std::vector<uint16_t> &indices)
{
    return create_indices_buffer(m_ctx, m_render_data, allocator, indices);
}

#include "video/TileApp.h"

#include "video/VmaUsage.h"

#include <array>
#include <vector>
#include <fstream>

#define SHADER_FOLDER "../shaders/"

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


TileApp::TileApp(shared_ptr<VulkanContext> ctx, TileSet& tileset, TilePalet& palet) : VulkanApp(ctx), tileset(tileset), palet(palet)
{
    
}

TileApp::~TileApp()
{
    m_ctx->disp.destroyImageView(textureView, nullptr);
    vmaDestroyImage(getAllocator(), texture, textureAllocation);
}

bool TileApp::init()
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(tileset.getWidth());
    imageInfo.extent.height = static_cast<uint32_t>(tileset.getHeight() * tileset.getMaxSize());
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;//static_cast<uint32_t>(tileset.getMaxSize());
    imageInfo.format = VK_FORMAT_R8G8B8A8_UINT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage =  VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    VmaAllocationCreateInfo vmaCreateImageInfo = {};
    vmaCreateImageInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImageInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    // vmaCreateImageInfo.usage = VMA_MEMORY_USAGE_CPU_COPY;
    // vmaCreateImageInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    // vmaCreateImageInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    // VmaAllocation allocation;
    vmaCreateImage(getAllocator(), &imageInfo, &vmaCreateImageInfo, &texture, &textureAllocation, &allocationInfo);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UINT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (m_ctx->disp.createImageView(&viewInfo, nullptr, &textureView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    // Transition the image layout to VK_IMAGE_LAYOUT_GENERAL for compute shader access
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_ctx->compute_command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (m_ctx->disp.allocateCommandBuffers(&allocInfo, &commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (m_ctx->disp.beginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

    m_ctx->disp.cmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    if (m_ctx->disp.endCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (m_ctx->disp.queueSubmit(m_ctx->compute_queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit command buffer!");
    }

    m_ctx->disp.queueWaitIdle(m_ctx->compute_queue);
    m_ctx->disp.freeCommandBuffers(m_ctx->compute_command_pool, 1, &commandBuffer);

    return true;
}

bool TileApp::run(Buffer& buffer)
{
    std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    layoutBindings[0].pImmutableSamplers = nullptr;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    layoutBindings[1].pImmutableSamplers = nullptr;

    layoutBindings[2].binding = 2;
    layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    layoutBindings[2].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = layoutBindings.data();

    if (m_ctx->disp.createDescriptorSetLayout(&layoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create compute descriptor set layout!");
    }


    VkDescriptorPoolSize poolSizes;
    poolSizes.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes.descriptorCount = 2;
    VkDescriptorPoolSize poolSizes2;
    poolSizes2.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes2.descriptorCount = 1;

    std::array<VkDescriptorPoolSize, 2> poolSizesArray{poolSizes, poolSizes2};

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizesArray.size());
    poolInfo.pPoolSizes = poolSizesArray.data();
    poolInfo.maxSets = 1;

    if (m_ctx->disp.createDescriptorPool(&poolInfo, nullptr, &computeDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create compute descriptor pool!");
    }

    VkDescriptorSetAllocateInfo descAllocInfo{};
    descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAllocInfo.descriptorPool = computeDescriptorPool;
    descAllocInfo.descriptorSetCount = 1;
    descAllocInfo.pSetLayouts = &computeDescriptorSetLayout;
    // data.descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
    if (m_ctx->disp.allocateDescriptorSets(&descAllocInfo, &computeDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

    VkDescriptorBufferInfo paletInfo = palet.getDescriptorBufferInfo();
    VkDescriptorBufferInfo tilesetInfo = tileset.getDescriptorBufferInfo();

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = computeDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &paletInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = computeDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &tilesetInfo;

    VkDescriptorImageInfo textureInfo = {};
    textureInfo.imageView = textureView;
    textureInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    textureInfo.sampler = VK_NULL_HANDLE;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = computeDescriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pImageInfo = &textureInfo;

    m_ctx->disp.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

    // Create pipeline layout -
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (m_ctx->disp.createPipelineLayout(&pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create compute pipeline layout!");
    }

    // Create the compute pipeline


    std::vector<char> computeShaderCode;
    try
    {
        computeShaderCode = readFile(std::string(SHADER_FOLDER) + "/tileset.spv");
    }
    catch(const std::exception& e)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to read shader file: %s", e.what());
        return true;
    }
    VkShaderModule computeShaderModule = createShaderModule(*m_ctx, computeShaderCode);

    
    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.stage = computeShaderStageInfo;
    computePipelineInfo.layout = computePipelineLayout;

    if (m_ctx->disp.createComputePipelines(VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create compute pipeline!");
    }


    // Record compute commands in a command buffer

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_ctx->compute_command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer computeCommandBuffer;
    if (m_ctx->disp.allocateCommandBuffers(&allocInfo, &computeCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate compute command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (m_ctx->disp.beginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }


    vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, 0);

    vkCmdDispatch(computeCommandBuffer, 2, 1, 1);

    VkBufferMemoryBarrier bufferBarrier{};
    bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.buffer = buffer.getBuffer();
    bufferBarrier.offset = 0;
    bufferBarrier.size = buffer.getSize();

    vkCmdPipelineBarrier(
        computeCommandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        1, &bufferBarrier,
        0, nullptr
    );

    VkBufferImageCopy copyRegion = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            static_cast<uint32_t>(tileset.getWidth()),
            static_cast<uint32_t>(tileset.getHeight() * tileset.getMaxSize()),
            1
        }
    };

    vkCmdCopyImageToBuffer(
        computeCommandBuffer,
        texture,
        VK_IMAGE_LAYOUT_GENERAL,
        buffer.getBuffer(),
        1,
        &copyRegion
    );


    if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record compute command buffer!");
    }

    
    // Submit compute command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &computeCommandBuffer;

    if (m_ctx->disp.queueSubmit(m_ctx->compute_queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit compute command buffer!");
    }

    //Should move to a fence and wait on it instead of waiting for the whole queue to be idle
    vkDeviceWaitIdle(m_ctx->device.device);

    // Handle image layout transitions
    // Cleanup

    m_ctx->disp.destroyPipeline(computePipeline, nullptr);
    m_ctx->disp.destroyPipelineLayout(computePipelineLayout, nullptr);
    m_ctx->disp.destroyDescriptorPool(computeDescriptorPool, nullptr);
    m_ctx->disp.destroyDescriptorSetLayout(computeDescriptorSetLayout, nullptr);
    m_ctx->disp.destroyShaderModule(computeShaderModule, nullptr);
    
    // for (size_t i = 0; i < tileset.getWidth() * tileset.getHeight() * tileset.getMaxSize(); i++)
    // {
    //     uint32_t value = buffer->get(i);
    //     std::cout << std::dec << "Index " << i << ": ";
    //     std::cout << std::hex << (value & 0xFF) << " ";
    //     std::cout << std::hex << ((value >> 8) & 0xFF) << " ";
    //     std::cout << std::hex << ((value >> 16) & 0xFF) << " ";
    //     std::cout << std::hex << ((value >> 24) & 0xFF) << " " << std::endl;
    // }
    
    // std::cout << "Done!" << std::endl;




    // VkPipelineShaderStageCreateInfo shaderStageInfo = {};
    // shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    // shaderStageInfo.module = tileset.getComputeShaderModule(m_ctx);
    // shaderStageInfo.pName = "main";
    

    // VkComputePipelineCreateInfo pipelineInfo;
    // pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    // pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    // pipelineInfo.stage.module = tileset.getComputeShaderModule(m_ctx);
    // pipelineInfo.stage.pName = "main";
    // pipelineInfo.layout = tileset.getPipelineLayout(m_ctx, palet);


    // VkPipeline computePipeline;
    // vkCreateComputePipeline(m_ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline);
    return true;
}
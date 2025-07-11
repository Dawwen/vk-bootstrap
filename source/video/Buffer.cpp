#include "video/Buffer.h"
#include "video/VmaUsage.h"

Buffer::Buffer(BufferType type, uint32_t size)
{
    VmaAllocator& allocator = getAllocator();

    m_bufferType = type;
    m_size = size;

    VkBufferCreateInfo buffer_create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_create_info.size = size;

    VmaAllocationCreateInfo allocation_create_info = {};
    allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;

    switch (type)
    {
        case StagingBuffer:
            buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        case VertexBuffer:
            buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            allocation_create_info.flags = 0;
            break;
        case IndiceBuffer:
            buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            allocation_create_info.flags = 0;
            break;
        default:
            buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            allocation_create_info.flags = 0;
            break;
    }

    // Staging buffer
    auto result = vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &m_buffer, &m_allocation, &m_allocation_info);
    if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create buffer");
    }
}

Buffer::~Buffer()
{
    VmaAllocator& allocator = getAllocator();
    vmaDestroyBuffer(allocator, m_buffer, m_allocation);
}

size_t Buffer::getSize()
{
    return m_size;
}

VkBuffer &Buffer::getBuffer()
{
    return m_buffer;
}

bool Buffer::copyToStagingBuffer(const void *buffer, size_t size, VkDeviceSize offset)
{
    VmaAllocator& allocator = getAllocator();
    auto result = vmaCopyMemoryToAllocation(allocator, buffer, m_allocation, offset, size);
    if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to copy to staging buffer");
        return true;
    }
    return false;
    
}

bool Buffer::copyTo(VulkanContext& ctx, RenderData& data, Buffer& src, Buffer& dst)
{
    if (src.getSize() != dst.getSize())
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Tried to copy to a buffer of a different size");
        return true;
    }
    

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
    copyRegion.size = src.getSize();
    vkCmdCopyBuffer(commandBuffer, src.m_buffer, dst.m_buffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(data.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(data.graphics_queue);

    vkFreeCommandBuffers(ctx.device.device, data.command_pool, 1, &commandBuffer);
    return false;
}

#ifndef BUFFER_H
#define BUFFER_H

#include <vk_mem_alloc.h>

#include "video/vulkan_context.h"

enum BufferType
{
    UniformBuffer,
    StagingBuffer,
    VertexBuffer,
    IndiceBuffer,
    StorageBuffer,
};

class Buffer
{
    private:
        BufferType m_bufferType;

        size_t m_size;
        uint32_t m_number_elements;
        VkBuffer m_buffer;
        VmaAllocation m_allocation;
        VmaAllocationInfo m_allocation_info;
        

    public:
        Buffer(BufferType type, uint32_t nb_elements, size_t size);
        ~Buffer();

        size_t getSize();
        VkBuffer& getBuffer();
        uint32_t getNumberOfElements();
        uint32_t get(uint32_t offset);
        void     set(uint32_t offset, uint32_t value);
        bool copyToStagingBuffer(const void* buffer, size_t size, VkDeviceSize offset=0);
        static bool copyTo(VulkanContext& ctx, Buffer& src, Buffer& dst);


};

#endif //BUFFER_H

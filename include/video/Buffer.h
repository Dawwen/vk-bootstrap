#ifndef BUFFER_H
#define BUFFER_H

#include <vk_mem_alloc.h>

#include "video/renderer_struct.h"

enum BufferType
{
    StagingBuffer,
    VertexBuffer,
    IndiceBuffer,
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
        Buffer(BufferType type, uint32_t size);
        ~Buffer();

        size_t getSize();
        VkBuffer& getBuffer();
        uint32_t getNumberOfElements();
        bool copyToStagingBuffer(const void* buffer, size_t size, VkDeviceSize offset=0);
        static bool copyTo(VulkanContext& ctx, RenderData& data, Buffer& src, Buffer& dst);


};

#endif //BUFFER_H

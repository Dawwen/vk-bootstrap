#ifndef VMA_USAGE_H
#define VMA_USAGE_H

#include <vk_mem_alloc.h>
#include "video/vulkan_context.h"

bool createAllocator(uint32_t apiVersion, VulkanContext ctx);
VmaAllocator& getAllocator();
bool destroyAllocator();

#endif //VMA_USAGE_H


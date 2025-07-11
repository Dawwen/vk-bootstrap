#ifndef VMA_USAGE_H
#define VMA_USAGE_H

#include <vk_mem_alloc.h>
#include "video/renderer_struct.h"

bool createAllocator(VulkanContext ctx);
VmaAllocator& getAllocator();
bool destroyAllocator();

#endif //VMA_USAGE_H


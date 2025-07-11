#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1002000 // Vulkan 1.2

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "video/renderer_struct.h"
#include "video/VmaUsage.h"

static bool allocatorCreated = false;
static VmaVulkanFunctions vulkanFunctions {};
static VmaAllocator allocator;

bool createAllocator(VulkanContext ctx)
{
    vulkanFunctions.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorCreateInfo.physicalDevice = ctx.device.physical_device;
    allocatorCreateInfo.device = ctx.device.device;
    allocatorCreateInfo.instance = ctx.instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
    VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &allocator);

    if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Could not create a VmaAllocator");
        return true;
    }
    allocatorCreated = true;
    return false;
}

VmaAllocator& getAllocator()
{
    if (!allocatorCreated)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "VmaAllocator get but not created");
    }
    return allocator;
}
bool destroyAllocator()
{
    allocatorCreated = false;
    vmaDestroyAllocator(allocator);
    return false;
}
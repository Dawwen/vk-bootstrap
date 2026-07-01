#include "video/VideoInit.h"

#include "video/VmaUsage.h"

#include <iostream>

// Section: SDL


// Utils

VkSurfaceKHR create_surface(VkInstance instance, SDL_Window* window, VkAllocationCallbacks* allocator = nullptr) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    auto res = SDL_Vulkan_CreateSurface(window, instance, allocator, &surface);
    if (!res)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create SDL Surface");
        surface = VK_NULL_HANDLE;
    }
    return surface;
}

// Main SDL functions

bool InitSDL(SDL_init_t& init, SDL_ctx_t& ctx)
{
    SDL_SetAppMetadata(init.app_name, init.app_version, init.app_identifier);
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't initialize SDL: %s", SDL_GetError());
        return true;
    }

    ctx.window = SDL_CreateWindow("examples/renderer/clear", init.width, init.height, init.window_flags);
    if (ctx.window == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create Vulkan window: %s", SDL_GetError());
        return true;
    }
    return false;
}

void DestroySDL(SDL_ctx_t& ctx)
{
    SDL_DestroyWindow(ctx.window);
    SDL_Quit();
}



// Section: Vulkan

// Utils

bool get_vulkan_queues(VulkanContext& ctx)
{
    auto gq = ctx.device.get_queue(vkb::QueueType::graphics);
    if (!gq.has_value())
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, gq.error().message().c_str());
        return true;
    }
    ctx.graphics_queue = gq.value();

    auto pq = ctx.device.get_queue(vkb::QueueType::present);
    if (!pq.has_value())
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, pq.error().message().c_str());
        return true;
    }
    ctx.present_queue = pq.value();

    // std::cout << "Looking for compute" << std::endl;
    // auto cq = ctx.device.get_queue(vkb::QueueType::compute);
    // if (!cq.has_value())
    // {
    //     SDL_LogError(SDL_LOG_CATEGORY_VIDEO, cq.error().message().c_str());
    //     return true;
    // }
    ctx.compute_queue = gq.value();
    std::cout << "Found compute" << std::endl;

    return false;
}

// Main Vulkan functions

bool InitVulkan(const Vulkan_init_t& init, VulkanContext& ctx)
{
    ctx.window = init.window;

    vkb::InstanceBuilder instance_builder;
    instance_builder.set_app_name(init.app_name)
                    .set_app_version(init.app_version_major, init.app_version_minor, init.app_version_patch)
                    .set_engine_name(init.engine_name)
                    .set_engine_version(init.engine_version_major, init.engine_version_minor, init.engine_version_patch)
                    .require_api_version(init.api_version_major, init.api_version_minor, init.api_version_patch)
                    .request_validation_layers();
    
    auto instance_ret = instance_builder.build();
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

    if (get_vulkan_queues(ctx))
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to get Vulkan queues");
        return true;
    }

    VkCommandPoolCreateInfo graphics_pool_info = {};
    graphics_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    graphics_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    graphics_pool_info.queueFamilyIndex = ctx.device.get_queue_index(vkb::QueueType::graphics).value();

    std::cout << "Command Pool" << std::endl;
    if (ctx.disp.createCommandPool(&graphics_pool_info, nullptr, &ctx.graphics_command_pool) != VK_SUCCESS)\
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create command pool");
        return true;
    }

    std::cout << "Command Pool Info" << std::endl;
    VkCommandPoolCreateInfo compute_pool_info = {};
    compute_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    compute_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    compute_pool_info.queueFamilyIndex = ctx.device.get_queue_index(vkb::QueueType::graphics).value();

    if (ctx.disp.createCommandPool(&compute_pool_info, nullptr, &ctx.compute_command_pool) != VK_SUCCESS)\
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to create command pool");
        return true;
    }

    createAllocator(init.vma_api_version, ctx);
    return false;
}

void DestroyVulkan(VulkanContext& ctx)
{

    vkDestroyCommandPool(ctx.device.device, ctx.graphics_command_pool, nullptr);
    vkDestroyCommandPool(ctx.device.device, ctx.compute_command_pool, nullptr);

    destroyAllocator();
    vkb::destroy_device(ctx.device);
    vkb::destroy_surface(ctx.instance, ctx.surface);
    vkb::destroy_instance(ctx.instance);
}

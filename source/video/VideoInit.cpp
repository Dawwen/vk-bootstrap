#include "video/VideoInit.h"

#include "video/VmaUsage.h"

// Section: SDL

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

VkSurfaceKHR create_surface(VkInstance instance, SDL_Window* window, VkAllocationCallbacks* allocator) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    auto res = SDL_Vulkan_CreateSurface(window, instance, allocator, &surface);
    if (!res)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create SDL Surface");
        surface = VK_NULL_HANDLE;
    }
    return surface;
}


// Section: Vulkan

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

    createAllocator(init.vma_api_version, ctx);
    return false;
}

void DestroyVulkan(VulkanContext& ctx)
{
    destroyAllocator();
    vkb::destroy_device(ctx.device);
    vkb::destroy_surface(ctx.instance, ctx.surface);
    vkb::destroy_instance(ctx.instance);
}

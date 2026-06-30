#ifndef VIDEO_INIT_H
#define VIDEO_INIT_H

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <VkBootstrap.h>

#include "video/vulkan_context.h"

struct SDL_init
{
    const char* app_name = "BasicSample";
    const char* app_version = "1.0";
    const char* app_identifier = "default.identifier";
    
    uint32_t width;
    uint32_t height;
    SDL_WindowFlags window_flags;
} typedef SDL_init_t;

struct SDL_ctx
{
    SDL_Window* window;
} typedef SDL_ctx_t;

struct Vulkan_init
{
    SDL_Window* window;

    uint32_t width;
    uint32_t height;

    const char* app_name = "Vulkan Renderer";
    uint32_t app_version_major = 1;
    uint32_t app_version_minor = 0;
    uint32_t app_version_patch = 0;

    const char* engine_name = "VkBootstrap";
    uint32_t engine_version_major = 1;
    uint32_t engine_version_minor = 0;
    uint32_t engine_version_patch = 0;

    uint32_t api_version_major = 1;
    uint32_t api_version_minor = 3;
    uint32_t api_version_patch = 0;

    uint32_t vma_api_version = VK_API_VERSION_1_3;
} typedef Vulkan_init_t;

// struct VulkanContext {
//     vkb::Instance instance;
//     vkb::InstanceDispatchTable inst_disp;

//     vkb::Device device;
//     vkb::DispatchTable disp;

//     VkQueue graphics_queue;
//     VkQueue present_queue;

//     // vkb::Swapchain swapchain;
//     // VkCommandPool command_pool;

//     SDL_Window* window;
//     VkSurfaceKHR surface;
// };

bool InitSDL(SDL_init_t& init, SDL_ctx_t& ctx);
void DestroySDL(SDL_ctx_t& ctx);
VkSurfaceKHR create_surface(VkInstance instance, SDL_Window* window, VkAllocationCallbacks* allocator = nullptr);
bool InitVulkan(const Vulkan_init_t& init, VulkanContext& ctx);
void DestroyVulkan(VulkanContext& ctx);

#endif //VIDEO_INIT_H
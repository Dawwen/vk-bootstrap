#ifndef RENDERER_STRUCT_H
#define RENDERER_STRUCT_H

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <VkBootstrap.h>


struct VulkanContext {
    vkb::Instance instance;
    vkb::InstanceDispatchTable inst_disp;
    VkSurfaceKHR surface;
    vkb::Device device;
    vkb::DispatchTable disp;
    // vkb::Swapchain swapchain;

    VkQueue graphics_queue;
    VkCommandPool graphics_command_pool;

    VkQueue present_queue;

    VkQueue compute_queue;
    VkCommandPool compute_command_pool;

    SDL_Window* window;
};

#endif //RENDERER_STRUCT_H
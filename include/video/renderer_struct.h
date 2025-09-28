#ifndef RENDERER_STRUCT_H
#define RENDERER_STRUCT_H

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <VkBootstrap.h>


struct VulkanContext {
    SDL_Window* window;
    vkb::Instance instance;
    vkb::InstanceDispatchTable inst_disp;
    VkSurfaceKHR surface;
    vkb::Device device;
    vkb::DispatchTable disp;
    vkb::Swapchain swapchain;
    VkCommandPool command_pool;

    VkQueue graphics_queue;
    VkQueue present_queue;
};

#endif //RENDERER_STRUCT_H
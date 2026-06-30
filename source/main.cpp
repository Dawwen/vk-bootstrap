#include "video/VideoInit.h"
#include "video/VulkanApp.h"

uint32_t SCREEN_WIDTH = 800;
uint32_t SCREEN_HEIGHT = 600;

int main(int argc, char const *argv[])
{
    SDL_init_t sdl_init = {
        app_name: "Vulkan Renderer",
        width: SCREEN_WIDTH,
        height: SCREEN_HEIGHT,
        window_flags: SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE,
    };

    SDL_ctx_t sdl_ctx;
    if (InitSDL(sdl_init, sdl_ctx))
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to init SDL");
        return -1;
    }

    Vulkan_init_t vulkan_init = {
        window: sdl_ctx.window,
        width: SCREEN_WIDTH,
        height: SCREEN_HEIGHT,
        app_name: "Vulkan Renderer",
    };

    VulkanContext ctx;

    if (InitVulkan(vulkan_init, ctx))
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to init Vulkan");
        return -1;
    }

    VulkanApp app(ctx);

    DestroyVulkan(ctx);
    DestroySDL(sdl_ctx);
}

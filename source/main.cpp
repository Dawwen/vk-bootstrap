#include "video/VideoInit.h"
#include "video/VulkanApp.h"
#include "video/TileApp.h"

#include "resource/TileSet.h"
#include "resource/TilePalet.h"

#include <memory>
using std::shared_ptr;

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void dumpTexture(Buffer& buffer, TileSet& tileset)
{
    int width = tileset.getWidth();
	int height = tileset.getHeight() * tileset.getMaxSize();

    unsigned char *data_debug = new unsigned char[height * width * 4];

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            size_t idx = (y * width + x) * 4;
            uint32_t color = buffer.get((y * width + x));

            data_debug[idx + 0] = (color >> 0) & 0xFF; // R
            data_debug[idx + 1] = (color >> 8) & 0xFF; // G
            data_debug[idx + 2] = (color >> 16) & 0xFF; // B
            data_debug[idx + 3] = (color >> 24) & 0xFF; // A
        }
    }

    // stbi_flip_vertically_on_write(1);
    stbi_write_bmp("gpu_render.bmp", width, height, 4, data_debug);
    delete[] data_debug;
}


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

    auto ctx = std::make_shared<VulkanContext>();

    if (InitVulkan(vulkan_init, *ctx))
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to init Vulkan");
        return -1;
    }

    {
        TilePalet palet {ColorDepth::UINT_32BIT, 16};
        TileColor color;
        color.color = 0xFF00FF00;
        // color.color = 0xFFFFFF00;

        palet.addColor(color);
        
        color.color = 0xFFFF00FF;

        palet.addColor(color);
        
        uint32_t WIDTH = 8;
        uint32_t MAX_TILES = 2;
        TileSet tileset {WIDTH, MAX_TILES};

        for (size_t k = 0; k < MAX_TILES; k++)
        {   
            for (size_t i = 0; i < tileset.getHeight(); i++)
            {
                for (size_t j = 0; j < tileset.getWidth(); j++)
                {
                    if (k == 0)
                    {
                        uint32_t value = (j%2 + i%2)%2 ;
                        tileset.set(k, j, i, value);
                    }
                    else
                    {
                        uint32_t value = (i == 0 || i == tileset.getHeight() - 1 || j == 0 || j == tileset.getWidth() - 1) ? 1 : 0;
                        tileset.set(k, j, i, value);
                    }
                }
            }
        }
        
        Buffer buffer (BufferType::StagingBuffer, tileset.getHeight() * tileset.getWidth() * tileset.getMaxSize(), sizeof(uint32_t));

        TileApp app(ctx);
        app.init();
        app.addResource(tileset, palet);
        app.run(buffer);
        dumpTexture(buffer, tileset);
    }

    DestroyVulkan(*ctx);
    DestroySDL(sdl_ctx);
}

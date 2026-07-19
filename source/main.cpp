#include "video/VideoInit.h"
#include "video/VulkanApp.h"
#include "video/VulkanVideoApp.h"
#include "video/TileApp.h"

#include "resource/TileSet.h"
#include "resource/TilePalet.h"

#include <memory>
using std::shared_ptr;

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void dumpTexture(const char* filename, Buffer& buffer, TileSet& tileset)
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
    stbi_write_bmp(filename, width, height, 4, data_debug);
    delete[] data_debug;
}


uint32_t SCREEN_WIDTH = 32;
uint32_t SCREEN_HEIGHT = 32;

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
        TilePalet palet_1 {ColorDepth::UINT_32BIT, 16};
        TileColor color;
        // color.color = 0xFFFFFF00;
        color.color = 0xFF00FF00;
        palet_1.addColor(color);
        
        color.color = 0xFFFF00FF;
        palet_1.addColor(color);
        
        uint32_t WIDTH = 8;
        uint32_t MAX_TILES = 2;
        TileSet tileset_1 {WIDTH, MAX_TILES};

        for (size_t k = 0; k < MAX_TILES; k++)
        {   
            for (size_t i = 0; i < tileset_1.getHeight(); i++)
            {
                for (size_t j = 0; j < tileset_1.getWidth(); j++)
                {
                    if (k == 0)
                    {
                        uint32_t value = (j%2 + i%2)%2 ;
                        tileset_1.set(k, j, i, value);
                    }
                    else
                    {
                        uint32_t value = (i == 0 || i == tileset_1.getHeight() - 1 || j == 0 || j == tileset_1.getWidth() - 1) ? 1 : 0;
                        tileset_1.set(k, j, i, value);
                    }
                }
            }
        }

        TilePalet palet_2 {ColorDepth::UINT_32BIT, 16};
        color.color = 0xFFFFFF00;
        palet_2.addColor(color);
        
        color.color = 0x000000FF;
        palet_2.addColor(color);
        

        TileSet tileset_2 {WIDTH, MAX_TILES};

        for (size_t k = 0; k < MAX_TILES; k++)
        {   
            for (size_t i = 0; i < tileset_2.getHeight(); i++)
            {
                for (size_t j = 0; j < tileset_2.getWidth(); j++)
                {
                    if (k == 0)
                    {
                        uint32_t value = i%2 ;
                        tileset_2.set(k, j, i, value);
                    }
                    else
                    {
                        uint32_t value = j%2;
                        tileset_2.set(k, j, i, value);
                    }
                }
            }
        }

        Buffer buffer_0 (BufferType::StagingBuffer, tileset_1.getHeight() * tileset_1.getWidth() * tileset_1.getMaxSize(), sizeof(uint32_t));
        Buffer buffer_1 (BufferType::StagingBuffer, tileset_1.getHeight() * tileset_1.getWidth() * tileset_1.getMaxSize(), sizeof(uint32_t));
        Buffer buffer_2 (BufferType::StagingBuffer, tileset_2.getHeight() * tileset_2.getWidth() * tileset_2.getMaxSize(), sizeof(uint32_t));
        Buffer buffer_3 (BufferType::StagingBuffer, tileset_2.getHeight() * tileset_2.getWidth() * tileset_2.getMaxSize(), sizeof(uint32_t));


        TileApp app(ctx);
        std::vector<Buffer*> buffers;

        app.init();

        app.addResource(tileset_1, palet_1);
        buffers.push_back(&buffer_0);

        app.addResource(tileset_1, palet_2);
        buffers.push_back(&buffer_1);
        
        app.addResource(tileset_2, palet_1);
        buffers.push_back(&buffer_2);
        
        app.addResource(tileset_2, palet_2);
        buffers.push_back(&buffer_3);
        
        app.run(buffers);

        dumpTexture("gpu_render_0.bmp", buffer_0, tileset_1);
        dumpTexture("gpu_render_1.bmp", buffer_1, tileset_1);
        dumpTexture("gpu_render_2.bmp", buffer_2, tileset_1);
        dumpTexture("gpu_render_3.bmp", buffer_3, tileset_1);
    }

    ctx->disp.queueWaitIdle(ctx->compute_queue);

    {
        VulkanVideoApp app(ctx);
        app.init(SCREEN_WIDTH, SCREEN_HEIGHT);

        bool running = true;
        while (running)
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {

                if (event.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    app.resize();
                }
                if (event.type == SDL_EVENT_QUIT)
                    running = false;
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
                    running = false;
            }

            app.render();
        }
    }
    DestroyVulkan(*ctx);
    DestroySDL(sdl_ctx);
}

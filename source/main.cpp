#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"

#include <chrono>
#include <vector>
#include <iostream>

#include "resource/TileSet.h"
#include "resource/TilePalet.h"

#include "video/Renderer.h"
#include "video/Vertex.h"
#include "video/UniformBuffer.h"
#include "video/VmaUsage.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const uint32_t SCREEN_WIDTH = 800;
const uint32_t SCREEN_HEIGHT = 600;

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

void calculateNewUniformBuffer(UniformBufferObject& ubo, uint32_t width, uint32_t height, float scale)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.model = ubo.model * glm::mat4(glm::mat3(/*0.5f + time* 0.1f*/ scale));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), width / (float) height, 0.1f, 10.0f);
    // ubo.proj[1][1] *= -1;
}

void cpuRender(TileSet& tileset, TilePalet& palet)
{
	int width = tileset.getWidth();
	int height = tileset.getHeight() * tileset.getMaxSize();

	
    unsigned char *data_debug = new unsigned char[height * width * 4];

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            uint32_t tileValue = tileset.get(y/tileset.getHeight(), x, y % tileset.getHeight());
            TileColor tc = palet.getColor(tileValue);
            size_t idx = (y * width + x) * 4;
            data_debug[idx + 0] = tc.r; // R
            data_debug[idx + 1] = tc.g; // G
            data_debug[idx + 2] = tc.b; // B
            data_debug[idx + 3] = tc.a; // A
        }
    }

    // stbi_flip_vertically_on_write(1);
    stbi_write_bmp("cpu_render.bmp", width, height, 4, data_debug);
    delete[] data_debug;
}

void gpuDump(TileSet& tileset, TilePalet& palet)
{
    int width = tileset.getWidth();
	int height = tileset.getHeight() * tileset.getMaxSize();

    unsigned char *data_debug = new unsigned char[height * width * 4];

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            uint32_t tileValue = tileset.get(y/tileset.getHeight(), x, y);
            TileColor tc = palet.getColor(tileValue);
            size_t idx = (y * width + x) * 4;
            data_debug[idx + 0] = tc.r; // R
            data_debug[idx + 1] = tc.g; // G
            data_debug[idx + 2] = tc.b; // B
            data_debug[idx + 3] = tc.a; // A
        }
    }

    // stbi_flip_vertically_on_write(1);
    stbi_write_bmp("gpu_render.bmp", width, height, 4, data_debug);
    delete[] data_debug;
}


int main(int argc, char const *argv[])
{
    Renderer renderer;
    UniformBufferObject ubo = {};
    ubo.model = glm::mat4(glm::mat3(0.3f));
    ubo.view = glm::mat4(1.0f);
    ubo.proj = glm::mat4(1.0f);

    if (renderer.init(SCREEN_WIDTH, SCREEN_HEIGHT))
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to init Renderer");
        return true;
    }

    renderer.createVertexBuffer(vertices);
    renderer.createIndicesBuffer(indices);

    bool my_tool_active = true;
    float scale = 1.0;

    auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime;

    TilePalet palet {ColorDepth::UINT_32BIT, 16};
    TileColor color;
    // color.color = 0xFFFFFFFF;
    color.color = 0xFF00FF00;

    palet.addColor(color);
    std::cout << "Added color: " << std::hex << color.color << std::dec << std::endl;
    std::cout << "Added color: " << std::hex << palet.getColor(0).color << std::dec << std::endl;
    
    color.color = 0xFFFF00FF;

    palet.addColor(color);
    std::cout << "Added color: " << std::hex << color.color << std::dec << std::endl;
    std::cout << "Added color: " << std::hex << palet.getColor(1).color << std::dec << std::endl;
    
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
                    uint32_t value = 1;
                    tileset.set(k, j, i, value);
                }
            }
        }
    }

    cpuRender(tileset, palet);

    // tileset.updateBuffer();

    // TileMap tilemap {16, 16};
    // for (size_t i = 0; i < 16; i++)
    // {
    //     for (size_t j = 0; j < 16; j++)
    //     {
    //         tilemap.set(j, i, j + i%2);
    //     }
    // }
    // tilemap.updateBuffer();
    
    VkImage texture;
    VkImageView textureView;
    VmaAllocation textureAllocation;

    renderer.createTileTexture(texture, textureView, textureAllocation, tileset, palet);

    SDL_Event event;
    while (event.type != SDL_EVENT_QUIT)
    {
        currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();

        SDL_PollEvent(&event);
        ImGui_ImplSDL3_ProcessEvent(&event); // Forward your event to backend
        
        if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            renderer.resize();
        }

        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_F12)
            {
                my_tool_active = true;
            }
        }


        // (After event loop)
        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (my_tool_active)
        {
            // ImGui::Show(); // Show demo window! :)
            // Create a window called "My First Tool", with a menu bar.
            ImGui::Begin("Vk Bootstrap", &my_tool_active, 0/*ImGuiWindowFlags_MenuBar*/);
            // if (ImGui::BeginMenuBar())
            // {
            //     if (ImGui::BeginMenu("File"))
            //     {
            //         if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
            //         if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
            //         if (ImGui::MenuItem("Close", "Ctrl+W"))  { my_tool_active = false; }
            //         ImGui::EndMenu();
            //     }
            //     ImGui::EndMenuBar();
            // }
            ImGui::Text("Framerate %.2f fps", 1/deltaTime);
            ImGui::SliderFloat("float", &scale, 0.0f, 4.0f);
            ImGui::End();
        }
        // Render ImGui
        ImGui::Render();
        calculateNewUniformBuffer(ubo, SCREEN_WIDTH, SCREEN_HEIGHT, scale);
        renderer.updateUniformBuffer(ubo);
        // std::cout << "Before render " << std::endl;
        renderer.renderTileSet(texture, textureView, tileset, palet);
        // gpuDump(texture, textureView);
        // std::cout << "After render " << std::endl;
        int res = renderer.drawFrame();
        if (res != 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "failed to draw frame ");
            return true;
        }
        lastTime = currentTime;
    }

    renderer.cleanTileTexture(texture, textureView, textureAllocation);
    return 0;
}

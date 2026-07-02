#include "video/VulkanApp.h"

#include <fstream>

std::vector<char> VulkanApp::readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(file_size));

    file.close();

    return buffer;
}

VkShaderModule VulkanApp::createShaderModule(VulkanContext& ctx, const std::vector<char>& code)
{
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (ctx.disp.createShaderModule(&create_info, nullptr, &shaderModule) != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to create shader module");
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

VulkanApp::VulkanApp(shared_ptr<VulkanContext> ctx) : m_ctx(ctx)
{
    // Initialize Vulkan resources here
}

VulkanApp::~VulkanApp()
{
    // Clean up Vulkan resources here
}
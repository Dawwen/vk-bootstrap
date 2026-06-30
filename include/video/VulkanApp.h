#ifndef VULKAN_APP_H
#define VULKAN_APP_H

#include "video/vulkan_context.h"

class VulkanApp {
    public:
        VulkanApp(VulkanContext& ctx);
        ~VulkanApp();

    private:
        VulkanContext& ctx;
};

#endif // VULKAN_APP_H
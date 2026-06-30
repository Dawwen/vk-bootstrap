#ifndef VULKAN_APP_H
#define VULKAN_APP_H

#include <memory>
using std::shared_ptr;

#include "video/vulkan_context.h"
class VulkanApp {
    public:
        VulkanApp(shared_ptr<VulkanContext> ctx);
        ~VulkanApp();

    protected:
        shared_ptr<VulkanContext> m_ctx;
};

#endif // VULKAN_APP_H
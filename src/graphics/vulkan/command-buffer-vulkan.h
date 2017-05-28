#ifndef _AK_COMMANDBUFFER_VULKAN_H_
#define _AK_COMMANDBUFFER_VULKAN_H_
#include "graphics/graphics.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>

namespace ak {

class CommandBufferVulkan : public CommandBuffer
{
   public:
    void reset() final;
    bool begin_render_pass() final;
    void end_render_pass() final;

   private:
    friend class GraphicsVulkan;

    GraphicsVulkan* _graphics = nullptr;

    VkCommandPool _pool = VK_NULL_HANDLE;
    VkCommandBuffer _buffer = VK_NULL_HANDLE;
    VkFence _fence = VK_NULL_HANDLE;
    bool _open = false;
};

}  // namespace ak

#endif  // _AK_COMMANDBUFFER_VULKAN_H_

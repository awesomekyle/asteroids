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
    void set_vertex_constant_data(uint32_t slot, void const* upload_data, size_t size) final;
    void set_pixel_constant_data(void const* upload_data, size_t size) final;
    void set_render_state(RenderState* const state) final;
    void set_vertex_buffer(Buffer* const buffer) final;
    void set_index_buffer(Buffer* const buffer) final;
    void draw(uint32_t vertex_count) final;
    void draw_indexed(uint32_t index_count) final;
    void end_render_pass() final;

   private:
    friend class GraphicsVulkan;

    GraphicsVulkan* _graphics = nullptr;

    class RenderStateVulkan* _current_render_state = nullptr;
    VkCommandPool _pool = VK_NULL_HANDLE;
    VkCommandBuffer _buffer = VK_NULL_HANDLE;
    VkFence _fence = VK_NULL_HANDLE;
    bool _open = false;
};

}  // namespace ak

#endif  // _AK_COMMANDBUFFER_VULKAN_H_

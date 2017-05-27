#ifndef _AK_GRAPHICS_VULKAN_H_
#define _AK_GRAPHICS_VULKAN_H_
#include "graphics/graphics.h"

#include <array>
#include <atomic>
#include <gsl/gsl_assert>

#include <vulkan/vulkan.hpp>

#include "command-buffer-vulkan.h"

namespace ak {

class GraphicsVulkan : public Graphics
{
   public:
    GraphicsVulkan();
    ~GraphicsVulkan() final;

    API api_type() const final;
    bool create_swap_chain(void* window, void*) final;
    bool resize(int, int) final;
    bool present() final;
    CommandBuffer* command_buffer() final;
    int num_available_command_buffers() final;
    bool execute(CommandBuffer* command_buffer) final;

   private:
    friend class CommandBufferVulkan;

    GraphicsVulkan(const GraphicsVulkan&) = delete;
    GraphicsVulkan& operator=(const GraphicsVulkan&) = delete;
    GraphicsVulkan(GraphicsVulkan&&) = delete;
    GraphicsVulkan& operator=(GraphicsVulkan&&) = delete;
};

/// @brief Creates a Vulkan Graphics device
ScopedGraphics create_graphics_vulkan();

}  // namespace ak

#endif  // _AK_GRAPHICS_VULKAN_H_

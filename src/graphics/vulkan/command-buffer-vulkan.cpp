#include "command-buffer-vulkan.h"
#include "graphics-vulkan.h"

#include <gsl/gsl>

namespace ak {

void CommandBufferVulkan::reset()
{
    VkResult result = _graphics->vkResetFences(_graphics->_device, 1, &_fence);
    assert(VK_SUCCEEDED(result) && "Could not reset fence");
    result = _graphics->vkResetCommandPool(_graphics->_device, _pool, 0);
    assert(VK_SUCCEEDED(result) && "Could not reset pool");
    result = _graphics->vkResetCommandBuffer(_buffer, 0);
    assert(VK_SUCCEEDED(result) && "Could not reset buffer");
    _open = false;
}

bool CommandBufferVulkan::begin_render_pass()
{
    if (_graphics->_swap_chain == VK_NULL_HANDLE) {
        return false;
    }
    auto const back_buffer_index = _graphics->get_back_buffer();
    constexpr VkClearValue clear_values[] = {
        {
            // color
            {
                {0.0f, 0.75f, 1.0f, 1.0f},
            },
            // depthStencil
            {
                0.0f,  // depth
                0x0,   // stencil
            },
        },
    };
    constexpr uint32_t num_clear_values = array_length(clear_values);

    VkRenderPassBeginInfo const render_pass_begin_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,     // sType
        nullptr,                                      // pNext
        _graphics->_render_pass,                      // renderPass
        _graphics->_framebuffers[back_buffer_index],  // framebuffer
        // renderArea
        {
            // offset
            {
                0,  // x
                0,  // y
            },
            _graphics->_surface_capabilities.currentExtent,  // extent
        },
        num_clear_values,  // clearValueCount
        clear_values,      // pClearValues
    };
    _graphics->vkCmdBeginRenderPass(_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    return true;
}

void CommandBufferVulkan::end_render_pass()
{
    _graphics->vkCmdEndRenderPass(_buffer);
}

}  // namespace ak

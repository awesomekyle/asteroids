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
    return false;
}

void CommandBufferVulkan::end_render_pass()
{
}

}  // namespace ak

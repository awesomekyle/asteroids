#include "command-buffer-vulkan.h"
#include "graphics-vulkan.h"

#include <gsl/gsl>

namespace ak {

void CommandBufferVulkan::reset()
{
}

bool CommandBufferVulkan::begin_render_pass()
{
    return false;
}

void CommandBufferVulkan::end_render_pass()
{
}

}  // namespace ak

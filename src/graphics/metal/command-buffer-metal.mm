#include "command-buffer-metal.h"
#include "graphics-metal.h"

#include <gsl/gsl>

namespace ak {

void CommandBufferMetal::reset()
{
    _buffer = nil;
}

bool CommandBufferMetal::begin_render_pass()
{
    if (_graphics->_layer == nil) {
        return false;
    }
    Expects(_buffer);
    auto* const descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    auto* const color_attachment = descriptor.colorAttachments[0];
    color_attachment.texture = _graphics->get_next_drawable().texture;
    color_attachment.loadAction = MTLLoadActionClear;
    color_attachment.storeAction = MTLStoreActionStore;
    constexpr float clear_color[] = {
        0.0f, 0.75f, 1.0f, 1.0f,
    };
    color_attachment.clearColor = MTLClearColorMake(clear_color[0], clear_color[1],
                                                    clear_color[2], clear_color[3]);
    _render_encoder = [_buffer renderCommandEncoderWithDescriptor:descriptor];
    return true;
}

void CommandBufferMetal::end_render_pass()
{
    [_render_encoder endEncoding];
    _render_encoder = nil;
}

}  // namespace ak

#include "graphics-metal.h"
#include "graphics/graphics.h"

#import <AppKit/AppKit.h>
#include <gsl/gsl>

#define UNUSED(v) ((void)(v))

namespace {

}  // anonymous namespace

namespace ak {

GraphicsMetal::GraphicsMetal()
    : _device(MTLCreateSystemDefaultDevice())
    , _render_queue([_device newCommandQueueWithMaxCommandBufferCount:kMaxCommandBuffers])
{
    _render_queue.label = @"Render Queue";

    for (auto& buffer : _command_buffers) {
        buffer._graphics = this;
    }
}
GraphicsMetal::~GraphicsMetal()
{
}

Graphics::API GraphicsMetal::api_type() const
{
    return kMetal;
}

bool GraphicsMetal::create_swap_chain(void* window, void* /*application*/)
{
    _window = (__bridge NSWindow*)(window);
    _layer = [CAMetalLayer layer];
    _layer.device = _device;
    _layer.opaque = true;
    _layer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

    _window.contentView.wantsLayer = true;
    _window.contentView.layer = _layer;

    return true;
}

bool GraphicsMetal::resize(int /*width*/, int /*height*/)
{
    if (_layer == nil) {
        return false;
    }

    NSRect const window_size = [_window convertRectToBacking:_window.frame];
    _layer.drawableSize = window_size.size;
    return true;
}

bool GraphicsMetal::present()
{
    if (_layer == nil) {
        return false;
    }
    [_current_drawable present];
    _current_drawable = nil;
    return true;
}

CommandBuffer* GraphicsMetal::command_buffer()
{
    Expects(_device);
    uint_fast32_t const curr_index = _current_command_buffer.fetch_add(1) % kMaxCommandBuffers;
    auto& buffer = gsl::at(_command_buffers, curr_index);
    if (buffer._buffer != nil) {
        /// @TODO: This case needs to be handled. The oldest command buffer hasn't
        /// yet finished.
        return nullptr;
    }
    buffer._buffer = [_render_queue commandBuffer];
    return &buffer;
}
int GraphicsMetal::num_available_command_buffers()
{
    Expects(_device);
    int free_buffers = 0;
    for (auto const& command_buffer : _command_buffers) {
        if (command_buffer._buffer == nil) {
            free_buffers++;
        }
    }
    return free_buffers;
}
bool GraphicsMetal::execute(CommandBuffer* command_buffer)
{
    auto* const metal_buffer = static_cast<CommandBufferMetal*>(command_buffer);
    [metal_buffer->_buffer addCompletedHandler:^(id<MTLCommandBuffer> /*buffer*/) {
        metal_buffer->reset();
    }];
    [metal_buffer->_buffer commit];
    return true;
}

std::unique_ptr<RenderState> GraphicsMetal::create_render_state(RenderStateDesc const& /*desc*/)
{
    // UNIMPLEMENTED
    return std::unique_ptr<RenderState>();
}
std::unique_ptr<Buffer> GraphicsMetal::create_vertex_buffer(uint32_t /*size*/, void const * /*data*/)
{
    // UNIMPLEMENTED
    return std::unique_ptr<Buffer>();
}
std::unique_ptr<Buffer> GraphicsMetal::create_index_buffer(uint32_t /*size*/, void const* /*data*/)
{
    // UNIMPLEMENTED
    return std::unique_ptr<Buffer>();
}

id<CAMetalDrawable> GraphicsMetal::get_next_drawable()
{
    if (_current_drawable == nil) {
        _current_drawable = _layer.nextDrawable;
    }
    return _current_drawable;
}

ScopedGraphics create_graphics_metal()
{
    return std::make_unique<GraphicsMetal>();
}

}  // namespace ak

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
    return nullptr;
}
int GraphicsMetal::num_available_command_buffers()
{
    return 0;
}
bool GraphicsMetal::execute(CommandBuffer* /*command_buffer*/)
{
    return false;
}

ScopedGraphics create_graphics_metal()
{
    return std::make_unique<GraphicsMetal>();
}

}  // namespace ak

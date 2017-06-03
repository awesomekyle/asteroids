#ifndef _AK_GRAPHICS_METAL_H_
#define _AK_GRAPHICS_METAL_H_
#include "graphics/graphics.h"

#include <array>
#include <gsl/gsl_assert>

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "command-buffer-metal.h"

namespace ak {


class GraphicsMetal : public Graphics
{
   public:
    GraphicsMetal();
    ~GraphicsMetal() final;

    API api_type() const final;
    bool create_swap_chain(void* window, void*) final;
    bool resize(int, int) final;
    bool present() final;
    CommandBuffer* command_buffer() final;
    int num_available_command_buffers() final;
    bool execute(CommandBuffer* command_buffer) final;
    std::unique_ptr<RenderState> GraphicsMetal::create_render_state(RenderStateDesc const& /*desc*/) final;
    std::unique_ptr<Buffer> create_vertex_buffer(uint32_t size, void const* data) final;
    std::unique_ptr<Buffer> create_index_buffer(uint32_t size, void const* data) final;

   private:
    friend class CommandBufferMetal;

    GraphicsMetal(const GraphicsMetal&) = delete;
    GraphicsMetal& operator=(const GraphicsMetal&) = delete;
    GraphicsMetal(GraphicsMetal&&) = delete;
    GraphicsMetal& operator=(GraphicsMetal&&) = delete;

    id<CAMetalDrawable> get_next_drawable();

    //
    // Constants
    //

    //
    // Types
    //
    id<MTLDevice> _device = nil;
    id<MTLCommandQueue> _render_queue = nil;

    NSWindow* _window = nil;
    CAMetalLayer* _layer = nil;

    std::atomic<int_fast32_t> _current_command_buffer = {};

    id<CAMetalDrawable> _current_drawable = nil;

    std::array<CommandBufferMetal, kMaxCommandBuffers> _command_buffers;

    //
    // Data members
    //
};

/// @brief Creates a Metal Graphics device
ScopedGraphics create_graphics_metal();

}  // namespace ak

#endif  // _AK_GRAPHICS_METAL_H_

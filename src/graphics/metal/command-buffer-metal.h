#ifndef _AK_COMMANDBUFFER_METAL_H_
#define _AK_COMMANDBUFFER_METAL_H_
#include "graphics/graphics.h"

#include <cassert>
#import <Metal/Metal.h>

namespace ak {

class GraphicsMetal;

class CommandBufferMetal : public CommandBuffer
{
   public:
    void reset() final;
    bool begin_render_pass() final;
    void set_vertex_constant_data(uint32_t /*slot*/, void const* /*upload_data*/, size_t /*size*/) final
    {
        // UNIMPLEMENTED
    }
    void set_pixel_constant_data(void const* /*upload_data*/, size_t /*size*/) final
    {
        // UNIMPLEMENTED
    }
    void set_render_state(RenderState* const /*state*/) final
    {
        // UNIMPLEMENTED
    }
    void set_vertex_buffer(Buffer* const /*buffer*/) final
    {
        // UNIMPLEMENTED
    }
    void set_index_buffer(Buffer* const /*buffer*/) final
    {
        // UNIMPLEMENTED
    }
    void draw(uint32_t /*vertex_count*/) final
    {
        // UNIMPLEMENTED
    }
    void draw_indexed(uint32_t /*index_count*/) final
    {
        // UNIMPLEMENTED
    }
    void end_render_pass() final;

   private:
    friend class GraphicsMetal;

    GraphicsMetal* _graphics = nullptr;

    id<MTLCommandBuffer> _buffer = nil;
    id<MTLRenderCommandEncoder> _render_encoder = nil;
};

}  // namespace ak

#endif  // _AK_COMMANDBUFFER_METAL_H_

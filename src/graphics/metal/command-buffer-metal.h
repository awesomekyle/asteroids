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
    void end_render_pass() final;

   private:
    friend class GraphicsMetal;

    GraphicsMetal* _graphics = nullptr;

    id<MTLCommandBuffer> _buffer = nil;
    id<MTLRenderCommandEncoder> _render_encoder = nil;
};

}  // namespace ak

#endif  // _AK_COMMANDBUFFER_METAL_H_

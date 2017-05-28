#ifndef _AK_COMMANDBUFFER_METAL_H_
#define _AK_COMMANDBUFFER_METAL_H_
#include "graphics/graphics.h"

#include <cassert>

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

};

}  // namespace ak

#endif  // _AK_COMMANDBUFFER_METAL_H_

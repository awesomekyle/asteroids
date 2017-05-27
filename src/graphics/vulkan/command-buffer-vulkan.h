#ifndef _AK_COMMANDBUFFER_VULKAN_H_
#define _AK_COMMANDBUFFER_VULKAN_H_
#include "graphics/graphics.h"

namespace ak {

class CommandBufferVulkan : public CommandBuffer
{
   public:
    void reset() final;
    bool begin_render_pass() final;
    void end_render_pass() final;

   private:
    friend class GraphicsVulkan;
};

}  // namespace ak

#endif  // _AK_COMMANDBUFFER_VULKAN_H_

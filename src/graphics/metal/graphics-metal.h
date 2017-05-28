#ifndef _AK_GRAPHICS_METAL_H_
#define _AK_GRAPHICS_METAL_H_
#include "graphics/graphics.h"

#include <array>
#include <gsl/gsl_assert>

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

   private:
    friend class CommandBufferMetal;

    GraphicsMetal(const GraphicsMetal&) = delete;
    GraphicsMetal& operator=(const GraphicsMetal&) = delete;
    GraphicsMetal(GraphicsMetal&&) = delete;
    GraphicsMetal& operator=(GraphicsMetal&&) = delete;


    //
    // Constants
    //

    //
    // Types
    //

    //
    // Data members
    //
};

/// @brief Creates a Metal Graphics device
ScopedGraphics create_graphics_metal();

}  // namespace ak

#endif  // _AK_GRAPHICS_METAL_H_

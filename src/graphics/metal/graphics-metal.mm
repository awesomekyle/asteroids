#include "graphics-metal.h"
#include "graphics/graphics.h"

#include <gsl/gsl>

#define UNUSED(v) ((void)(v))

namespace {

}  // anonymous namespace

namespace ak {

GraphicsMetal::GraphicsMetal()
{
}
GraphicsMetal::~GraphicsMetal()
{
}

Graphics::API GraphicsMetal::api_type() const
{
    return kMetal;
}

bool GraphicsMetal::create_swap_chain(void* /*window*/, void* /*application*/)
{
    return false;
}

bool GraphicsMetal::resize(int /*width*/, int /*height*/)
{
    return false;
}

bool GraphicsMetal::present()
{
    return false;
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

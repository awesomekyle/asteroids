#include "graphics/graphics.h"
#if defined(_WIN32)
#include "d3d12/graphics-d3d12.h"
#endif

namespace ak {

Graphics::~Graphics() = default;

ScopedGraphics create_graphics()
{
    return create_graphics_d3d12();
}

}  // namespace ak

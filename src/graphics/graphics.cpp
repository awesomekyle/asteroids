#include "graphics/graphics.h"
#if defined(_WIN32)
#include "graphics-d3d12.h"
#endif

namespace ak {

Graphics::~Graphics()
{
}

ScopedGraphics create_graphics()
{
    return create_graphics_d3d12();
}

}  // namespace ak

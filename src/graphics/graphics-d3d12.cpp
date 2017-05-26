#include "graphics\graphics.h"
#include "graphics-d3d12.h"

namespace ak {

class GraphicsD3D12 : public Graphics
{
  public:
    ~GraphicsD3D12()
    {

    }

    bool create_swap_chain(void* window, void* application) final {
        return false;
    }

    bool resize(int width, int height) final {
        return false;
    }

};


ScopedGraphics create_graphics_d3d12()
{
    return std::make_unique<GraphicsD3D12>();
}

} // namespace ak

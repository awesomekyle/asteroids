#include "graphics\graphics.h"
#include "graphics-d3d12.h"

namespace ak {

class GraphicsD3D12 : public Graphics
{
  public:
    ~GraphicsD3D12()
    {

    }

    virtual bool initialized() const
    {
        return true;
    }
};


ScopedGraphics create_graphics_d3d12()
{
    return std::make_unique<GraphicsD3D12>();
}

} // namespace ak

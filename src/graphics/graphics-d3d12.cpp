#include "graphics\graphics.h"
#include "graphics-d3d12.h"

namespace ak {

class GraphicsD3D12 : public Graphics
{
  public:
    void destroy() final {
        delete this;
    }

    bool initialized() const final
    {
        return true;
    }
};


ScopedGraphics create_graphics_d3d12()
{
    return std::unique_ptr<Graphics, GraphicsDeleter>(new GraphicsD3D12);
}

} // namespace ak

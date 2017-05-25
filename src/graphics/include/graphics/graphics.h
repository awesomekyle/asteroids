#ifndef _AK_GRAPHICS_H_
#define _AK_GRAPHICS_H_
#include <memory>

namespace ak {

class Graphics
{
  public:
    virtual void destroy() = 0;

    virtual bool initialized() const = 0;
};

struct GraphicsDeleter {
    void operator()(Graphics* g)
    {
        g->destroy();
    }
};

using ScopedGraphics = std::unique_ptr<Graphics, GraphicsDeleter>;

ScopedGraphics create_graphics();

} // ak

#endif //_AK_GRAPHICS_H_
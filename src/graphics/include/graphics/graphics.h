#ifndef _AK_GRAPHICS_H_
#define _AK_GRAPHICS_H_
#include <memory>

namespace ak {

class Graphics
{
  public:
    virtual ~Graphics();

    virtual bool initialized() const = 0;
};

using ScopedGraphics = std::unique_ptr<Graphics>;

ScopedGraphics create_graphics();

} // ak

#endif //_AK_GRAPHICS_H_
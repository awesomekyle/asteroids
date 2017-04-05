extern "C" {
#include "gfx/gfx.h"
} // extern "C"
#include <stdbool.h>
#include <stdlib.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#if defined(_DEBUG)
    #include <dxgidebug.h>
#endif

struct Gfx {
    bool initialized;
};

Gfx* gfxCreateD3D12(void)
{
    Gfx* const G = (Gfx*)calloc(1, sizeof(*G));
    G->initialized = true;
    return G;
}

void gfxDestroyD3D12(Gfx* G)
{
    free(G);
}

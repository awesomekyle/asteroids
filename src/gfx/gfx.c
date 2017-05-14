#include "config.h"
#include "gfx/gfx.h"
#include <stdbool.h>
#include <stdlib.h>
#if HAVE_D3D12
#   include "d3d12/gfx-d3d12.h"
#endif
#if HAVE_METAL
#   include "metal/gfx-metal.h"
#endif

Gfx* gfxCreate(void)
{
    Gfx* G = NULL;
    #if HAVE_D3D12
        G = gfxCreateD3D12();
    #endif
    #if HAVE_METAL
        G = gfxCreateMetal();
    #endif
    return G;
}

void gfxDestroy(Gfx* G)
{
    (void)G;
    #if HAVE_D3D12
        gfxDestroyD3D12(G);
    #endif
    #if HAVE_METAL
        gfxDestroyMetal(G);
    #endif
}

#include "gfx/gfx.h"
#include <stdbool.h>
#include <stdlib.h>

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

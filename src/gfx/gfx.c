#include "gfx/gfx.h"
#include <stdbool.h>
#include <stdlib.h>

struct Gfx {
    bool initialized;
};

Gfx* gfxCreate(void)
{
    Gfx* const G = (Gfx*)calloc(1, sizeof(*G));
    G->initialized = true;
    return G;
}

void gfxDestroy(Gfx* G)
{
    free(G);
}

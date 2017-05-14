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
#include "gfx-internal.h"

Gfx* gfxCreate(void)
{
    Gfx* G = NULL;
    #if HAVE_D3D12
        G = (Gfx*)gfxD3D12Create();
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
        //gfxD3D12Destroy(G);
        G->table->Destroy(G);
    #endif
    #if HAVE_METAL
        gfxDestroyMetal(G);
    #endif
}


bool gfxCreateSwapChain(Gfx* G, void* window)
{
    return G->table->CreateSwapChain(G, window);
}

bool gfxResize(Gfx* G, int width, int height)
{
    return G->table->Resize(G, width, height);
}

GfxRenderTarget gfxGetBackBuffer(Gfx* G)
{
    return G->table->GetBackBuffer(G);
}

bool gfxPresent(Gfx* G)
{
    return G->table->Present(G);
}

GfxCmdBuffer* gfxGetCommandBuffer(Gfx* G)
{
    return G->table->GetCommandBuffer(G);
}

int gfxNumAvailableCommandBuffers(Gfx* G)
{
    return G->table->NumAvailableCommandBuffers(G);
}

void gfxResetCommandBuffer(GfxCmdBuffer* B)
{
    B->table->ResetCommandBuffer(B);
}

bool gfxExecuteCommandBuffer(GfxCmdBuffer* B)
{
    return B->table->ExecuteCommandBuffer(B);
}

void gfxCmdBeginRenderPass(GfxCmdBuffer* B,
                           GfxRenderTarget renderTargetHandle,
                           GfxRenderPassAction loadAction,
                           float const clearColor[4])
{
    B->table->CmdBeginRenderPass(B, renderTargetHandle, loadAction, clearColor);
}
void gfxCmdEndRenderPass(GfxCmdBuffer* B)
{
    B->table->CmdEndRenderPass(B);
}

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
        G = gfxD3D12Create();
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
        gfxD3D12Destroy(G);
    #endif
    #if HAVE_METAL
        gfxDestroyMetal(G);
    #endif
}


bool gfxCreateSwapChain(Gfx* G, void* window)
{
    return gfxD3D12CreateSwapChain(G, window);
}

bool gfxResize(Gfx* G, int width, int height)
{
    return gfxD3D12Resize(G, width, height);
}

GfxRenderTarget gfxGetBackBuffer(Gfx* G)
{
    return gfxD3D12GetBackBuffer(G);
}

bool gfxPresent(Gfx* G)
{
    return gfxD3D12Present(G);
}

GfxCmdBuffer* gfxGetCommandBuffer(Gfx* G)
{
    return gfxD3D12GetCommandBuffer(G);
}

int gfxNumAvailableCommandBuffers(Gfx* G)
{
    return gfxD3D12NumAvailableCommandBuffers(G);
}

void gfxResetCommandBuffer(GfxCmdBuffer* B)
{
    gfxD3D12ResetCommandBuffer(B);
}

bool gfxExecuteCommandBuffer(GfxCmdBuffer* B)
{
    return gfxD3D12ExecuteCommandBuffer(B);
}

void gfxCmdBeginRenderPass(GfxCmdBuffer* B,
                           GfxRenderTarget renderTargetHandle,
                           GfxRenderPassAction loadAction,
                           float const clearColor[4])
{
    gfxD3D12CmdBeginRenderPass(B, renderTargetHandle, loadAction, clearColor);
}
void gfxCmdEndRenderPass(GfxCmdBuffer* B)
{
    gfxD3D12CmdEndRenderPass(B);
}

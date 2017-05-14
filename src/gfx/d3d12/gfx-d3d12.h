#pragma once
#include "gfx/gfx.h"

Gfx* gfxD3D12Create(void);
void gfxD3D12Destroy(Gfx* G);

bool gfxD3D12CreateSwapChain(Gfx* G, void* window);

bool gfxD3D12Resize(Gfx* G, int width, int height);

GfxRenderTarget gfxD3D12GetBackBuffer(Gfx* G);

bool gfxD3D12Present(Gfx* G);

GfxCmdBuffer* gfxD3D12GetCommandBuffer(Gfx* G);

int gfxD3D12NumAvailableCommandBuffers(Gfx* G);

void gfxD3D12ResetCommandBuffer(GfxCmdBuffer* B);

bool gfxD3D12ExecuteCommandBuffer(GfxCmdBuffer* B);

void gfxD3D12CmdBeginRenderPass(GfxCmdBuffer* B,
                                GfxRenderTarget renderTargetHandle,
                                GfxRenderPassAction loadAction,
                                float const clearColor[4]);

void gfxD3D12CmdEndRenderPass(GfxCmdBuffer* B);

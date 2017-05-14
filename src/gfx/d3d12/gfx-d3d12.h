#pragma once
#include "gfx/gfx.h"
#include "../gfx-internal.h"

typedef struct GfxD3D12 GfxD3D12;
typedef struct GfxD3D12CmdBuffer GfxD3D12CmdBuffer;

extern GfxTable const GfxD3D12Table;
extern GfxCmdBufferTable const GfxD3D12CmdBufferTable;

GfxD3D12* gfxD3D12Create(void);


GfxD3D12* gfxD3D12Create(void);
void gfxD3D12Destroy(GfxD3D12* G);

bool gfxD3D12CreateSwapChain(GfxD3D12* G, void* window);

bool gfxD3D12Resize(GfxD3D12* G, int width, int height);

GfxRenderTarget gfxD3D12GetBackBuffer(GfxD3D12* G);

bool gfxD3D12Present(GfxD3D12* G);

GfxD3D12CmdBuffer* gfxD3D12GetCommandBuffer(GfxD3D12* G);

int gfxD3D12NumAvailableCommandBuffers(GfxD3D12* G);

void gfxD3D12ResetCommandBuffer(GfxD3D12CmdBuffer* B);

bool gfxD3D12ExecuteCommandBuffer(GfxD3D12CmdBuffer* B);

void gfxD3D12CmdBeginRenderPass(GfxD3D12CmdBuffer* B,
                                GfxRenderTarget renderTargetHandle,
                                GfxRenderPassAction loadAction,
                                float const clearColor[4]);

void gfxD3D12CmdEndRenderPass(GfxD3D12CmdBuffer* B);

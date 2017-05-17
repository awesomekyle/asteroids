#pragma once
#include "gfx/gfx.h"
#include "../gfx-internal.h"

extern GfxTable const GfxMetalTable;
extern GfxCmdBufferTable const GfxMetalCmdBufferTable;

Gfx* gfxMetalCreate(void);

void gfxMetalDestroy(Gfx* G);

bool gfxMetalCreateSwapChain(Gfx* G, void* window, void* application);

bool gfxMetalResize(Gfx* G, int width, int height);

GfxRenderTarget gfxMetalGetBackBuffer(Gfx* G);

bool gfxMetalPresent(Gfx* G);

GfxCmdBuffer* gfxMetalGetCommandBuffer(Gfx* G);

int gfxMetalNumAvailableCommandBuffers(Gfx* G);

void gfxMetalResetCommandBuffer(GfxCmdBuffer* B);

bool gfxMetalExecuteCommandBuffer(GfxCmdBuffer* B);

void gfxMetalCmdBeginRenderPass(GfxCmdBuffer* B,
                           GfxRenderTarget renderTargetHandle,
                           GfxRenderPassAction loadAction,
                           float const clearColor[4]);

void gfxMetalCmdEndRenderPass(GfxCmdBuffer* B);

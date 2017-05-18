#pragma once
#include "gfx/gfx.h"
#include "../gfx-internal.h"

typedef struct Gfx Gfx;
typedef struct GfxCmdBuffer GfxCmdBuffer;

extern GfxTable const GfxVulkanTable;
extern GfxCmdBufferTable const GfxVulkanCmdBufferTable;

Gfx* gfxVulkanCreate(void);


Gfx* gfxVulkanCreate(void);
void gfxVulkanDestroy(Gfx* G);

bool gfxVulkanCreateSwapChain(Gfx* G, void* window, void* application);

bool gfxVulkanResize(Gfx* G, int width, int height);

GfxRenderTarget gfxVulkanGetBackBuffer(Gfx* G);

bool gfxVulkanPresent(Gfx* G);

GfxRenderState* gfxVulkanCreateRenderState(Gfx* G, GfxRenderStateDesc const* desc);

GfxCmdBuffer* gfxVulkanGetCommandBuffer(Gfx* G);

int gfxVulkanNumAvailableCommandBuffers(Gfx* G);

void gfxVulkanResetCommandBuffer(GfxCmdBuffer* B);

bool gfxVulkanExecuteCommandBuffer(GfxCmdBuffer* B);

void gfxVulkanCmdBeginRenderPass(GfxCmdBuffer* B,
                                 GfxRenderTarget renderTargetHandle,
                                 GfxRenderPassAction loadAction,
                                 float const clearColor[4]);

void gfxVulkanCmdEndRenderPass(GfxCmdBuffer* B);

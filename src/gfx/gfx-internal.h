#pragma once
#include "gfx/gfx.h"

size_t gfxFormatSize(GfxPixelFormat format);

typedef struct {
    GfxApi (*GetApi)(Gfx const* G);
    void (*Destroy)(Gfx* G);
    bool (*CreateSwapChain)(Gfx* G, void* window, void* application);
    bool (*Resize)(Gfx* G, int width, int height);
    GfxRenderTarget (*GetBackBuffer)(Gfx* G);
    bool (*Present)(Gfx* G);
    GfxRenderState* (*CreateRenderState)(Gfx* G, GfxRenderStateDesc const* desc);
    void (*DestroyRenderState)(Gfx* G, GfxRenderState* state);
    GfxCmdBuffer* (*GetCommandBuffer)(Gfx* G);
    int (*NumAvailableCommandBuffers)(Gfx* G);
} GfxTable;

typedef struct {
    void (*ResetCommandBuffer)(GfxCmdBuffer* B);
    bool (*ExecuteCommandBuffer)(GfxCmdBuffer* B);
    void (*CmdBeginRenderPass)(GfxCmdBuffer* B,
                               GfxRenderTarget renderTargetHandle,
                               GfxRenderPassAction loadAction,
                               float const clearColor[4]);
    void (*CmdEndRenderPass)(GfxCmdBuffer* B);
} GfxCmdBufferTable;

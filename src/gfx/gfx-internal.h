#pragma once
#include "gfx/gfx.h"

typedef struct {
    void (*Destroy)(Gfx* G);
    bool (*CreateSwapChain)(Gfx* G, void* window);
    bool (*Resize)(Gfx* G, int width, int height);
    GfxRenderTarget (*GetBackBuffer)(Gfx* G);
    bool (*Present)(Gfx* G);
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

struct Gfx
{
    GfxTable const* table;
};
struct GfxCmdBuffer
{
    GfxCmdBufferTable const* table;
};

#include "gfx/gfx.h"
#include "gfx-metal.h"
#include <assert.h>
#include <stddef.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

enum {
    kMaxCommandLists = 128,
};

struct Gfx
{
    id<MTLDevice>       device;
    id<MTLCommandQueue> renderQueue;

    NSWindow*       window;
    CAMetalLayer*   layer;
};

Gfx* gfxCreateMetal(void)
{
    Gfx* const G = calloc(1, sizeof(*G));
    G->device = MTLCreateSystemDefaultDevice();
    G->renderQueue = [G->device newCommandQueueWithMaxCommandBufferCount:kMaxCommandLists];
    return G;
}
void gfxDestroyMetal(Gfx* G)
{
    assert(G);
    [G->layer release];
    [G->renderQueue release];
    [G->device release];
    free(G);
}

bool gfxCreateSwapChain(Gfx* G, void* window)
{
    assert(G);
    assert(window);
    NSWindow* const cocoaWindow = window;
    G->window = cocoaWindow;
    G->layer = [CAMetalLayer layer];

    cocoaWindow.contentView.layer = G->layer;
    return true;
}

bool gfxResize(Gfx* G, int width, int height)
{
    assert(G);
    (void)width;(void)height;
    return false;
}

GfxRenderTarget gfxGetBackBuffer(Gfx* G)
{
    assert(G);
    return kGfxInvalidHandle;
}

bool gfxPresent(Gfx* G)
{
    assert(G);
    return false;
}

GfxCmdBuffer* gfxGetCommandBuffer(Gfx* G)
{
    assert(G);
    return NULL;
}

int gfxNumAvailableCommandBuffers(Gfx* G)
{
    assert(G);
    return 0;
}

void gfxResetCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
}

bool gfxExecuteCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
    return false;
}

void gfxCmdBeginRenderPass(GfxCmdBuffer* B,
                           GfxRenderTarget renderTargetHandle,
                           GfxRenderPassAction loadAction,
                           float const clearColor[4])
{
    assert(B);
    (void)renderTargetHandle;
    (void)loadAction;
    (void)clearColor;
}
void gfxCmdEndRenderPass(GfxCmdBuffer* B)
{
    assert(B);

}

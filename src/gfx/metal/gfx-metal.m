#include "gfx/gfx.h"
#include "gfx-metal.h"
#include <assert.h>
#include <stddef.h>
#include <stdatomic.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

enum {
    kMaxCommandLists = 128,
};

struct GfxCmdBuffer
{
    id<MTLCommandBuffer> buffer;
    Gfx*    G;
};

struct Gfx
{
    id<MTLDevice>       device;
    id<MTLCommandQueue> renderQueue;

    NSWindow*       window;
    CAMetalLayer*   layer;

    atomic_int availableCommandBuffers;
};

Gfx* gfxCreateMetal(void)
{
    Gfx* const G = calloc(1, sizeof(*G));
    G->device = MTLCreateSystemDefaultDevice();
    G->renderQueue = [G->device newCommandQueueWithMaxCommandBufferCount:kMaxCommandLists];
    G->renderQueue.label = @"Render Queue";
    G->availableCommandBuffers = kMaxCommandLists;
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
    G->layer.device = G->device;

    cocoaWindow.contentView.layer = G->layer;
    return true;
}

bool gfxResize(Gfx* G, int width, int height)
{
    assert(G);
    (void)width;(void)height; // TODO: Use these, don't just infer from window size
    if (G->layer == NULL) {
        return false;
    }

    NSRect const windowSize = [G->window convertRectToBacking:G->window.frame];
    G->layer.drawableSize = windowSize.size;

    return true;
}

GfxRenderTarget gfxGetBackBuffer(Gfx* G)
{
    assert(G);
    return (GfxRenderTarget)[G->layer nextDrawable];
}

bool gfxPresent(Gfx* G)
{
    assert(G);
    [[G->layer nextDrawable] present];
    return true;
}

GfxCmdBuffer* gfxGetCommandBuffer(Gfx* G)
{
    assert(G);
    if (G->availableCommandBuffers == 0) {
        return NULL;
    }
    id<MTLCommandBuffer> buffer = [[G->renderQueue commandBuffer] retain];
    if (buffer == nil) {
        return NULL;
    }
    atomic_fetch_add(&G->availableCommandBuffers, -1);
    GfxCmdBuffer* const cmdBuffer = calloc(1, sizeof(*cmdBuffer));
    cmdBuffer->buffer = buffer;
    cmdBuffer->G = G;
    return cmdBuffer;
}

int gfxNumAvailableCommandBuffers(Gfx* G)
{
    assert(G);
    return G->availableCommandBuffers;
}

void gfxResetCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
    atomic_fetch_add(&B->G->availableCommandBuffers, 1);
    [B->buffer release];
    free(B);
}

bool gfxExecuteCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
    [B->buffer commit];
    gfxResetCommandBuffer(B);
    return true;
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

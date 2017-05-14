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
    id<MTLRenderCommandEncoder> renderEncoder;
    Gfx*    G;
};

struct Gfx
{
    id<MTLDevice>       device;
    id<MTLCommandQueue> renderQueue;

    NSWindow*       window;
    CAMetalLayer*   layer;

    atomic_int availableCommandBuffers;

    id<CAMetalDrawable> currentDrawable;
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
    G->layer.opaque = true;
    G->layer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

    cocoaWindow.contentView.wantsLayer = true;
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
    if(G->currentDrawable == nil) {
        G->currentDrawable = [G->layer nextDrawable];
    }
    return (GfxRenderTarget)G->currentDrawable.texture;
}

bool gfxPresent(Gfx* G)
{
    assert(G);
    [G->currentDrawable present];
    G->currentDrawable = nil;
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
    MTLRenderPassDescriptor* const descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    assert(descriptor);
    if (renderTargetHandle != kGfxInvalidHandle) {
        MTLRenderPassColorAttachmentDescriptor* const attachment = descriptor.colorAttachments[0];
        attachment.texture = (id<MTLTexture>)renderTargetHandle;
        attachment.storeAction = MTLStoreActionStore;
        if (loadAction == kGfxRenderPassActionClear) {
            assert(clearColor);
            attachment.loadAction = MTLLoadActionClear;
            attachment.clearColor = MTLClearColorMake(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        }
    }
    id<MTLRenderCommandEncoder> encoder = [B->buffer renderCommandEncoderWithDescriptor:descriptor];
    B->renderEncoder = encoder;
}
void gfxCmdEndRenderPass(GfxCmdBuffer* B)
{
    assert(B);
    [B->renderEncoder endEncoding];
}
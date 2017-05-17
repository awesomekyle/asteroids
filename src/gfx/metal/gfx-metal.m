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
    GfxCmdBufferTable const* table;
    id<MTLCommandBuffer> buffer;
    id<MTLRenderCommandEncoder> renderEncoder;
    Gfx*    G;
};

struct Gfx
{
    GfxTable const* table;
    id<MTLDevice>       device;
    id<MTLCommandQueue> renderQueue;

    NSWindow*       window;
    CAMetalLayer*   layer;

    atomic_int availableCommandBuffers;

    id<CAMetalDrawable> currentDrawable;
};

Gfx* gfxMetalCreate(void)
{
    Gfx* const G = calloc(1, sizeof(*G));
    G->device = MTLCreateSystemDefaultDevice();
    G->renderQueue = [G->device newCommandQueueWithMaxCommandBufferCount:kMaxCommandLists];
    G->renderQueue.label = @"Render Queue";
    G->availableCommandBuffers = kMaxCommandLists;
    G->table = &GfxMetalTable;
    return G;
}
void gfxMetalDestroy(Gfx* G)
{
    assert(G);
    [G->layer release];
    [G->renderQueue release];
    [G->device release];
    free(G);
}

bool gfxMetalCreateSwapChain(Gfx* G, void* window, void* application)
{
    assert(G);
    assert(window);
    (void)application;
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

bool gfxMetalResize(Gfx* G, int width, int height)
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

GfxRenderTarget gfxMetalGetBackBuffer(Gfx* G)
{
    assert(G);
    if(G->currentDrawable == nil) {
        G->currentDrawable = [G->layer nextDrawable];
    }
    return (GfxRenderTarget)G->currentDrawable.texture;
}

bool gfxMetalPresent(Gfx* G)
{
    assert(G);
    [G->currentDrawable present];
    G->currentDrawable = nil;
    return true;
}

GfxCmdBuffer* gfxMetalGetCommandBuffer(Gfx* G)
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
    cmdBuffer->table = &GfxMetalCmdBufferTable;
    return cmdBuffer;
}

int gfxMetalNumAvailableCommandBuffers(Gfx* G)
{
    assert(G);
    return G->availableCommandBuffers;
}

void gfxMetalResetCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
    atomic_fetch_add(&B->G->availableCommandBuffers, 1);
    [B->buffer release];
    free(B);
}

bool gfxMetalExecuteCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
    [B->buffer commit];
    gfxMetalResetCommandBuffer(B);
    return true;
}

void gfxMetalCmdBeginRenderPass(GfxCmdBuffer* B,
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
void gfxMetalCmdEndRenderPass(GfxCmdBuffer* B)
{
    assert(B);
    [B->renderEncoder endEncoding];
}

GfxTable const GfxMetalTable = {
    gfxMetalDestroy,
    gfxMetalCreateSwapChain,
    gfxMetalResize,
    gfxMetalGetBackBuffer,
    gfxMetalPresent,
    gfxMetalGetCommandBuffer,
    gfxMetalNumAvailableCommandBuffers,
};
GfxCmdBufferTable const GfxMetalCmdBufferTable = {
    gfxMetalResetCommandBuffer,
    gfxMetalExecuteCommandBuffer,
    gfxMetalCmdBeginRenderPass,
    gfxMetalCmdEndRenderPass,
};

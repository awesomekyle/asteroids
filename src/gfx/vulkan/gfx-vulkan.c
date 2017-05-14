#include "gfx/gfx.h"
#include "gfx-vulkan.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>

#include "../gfx-internal.h"

struct GfxCmdBuffer {
    GfxCmdBufferTable const* table;
};

struct Gfx {
    GfxTable const* table;
};

//////
// Helper methods
//////

Gfx* gfxVulkanCreate(void)
{
    Gfx* const G = (Gfx*)calloc(1, sizeof(*G));

    assert(G && "Could not allocate space for a Vulkan Gfx device");

    G->table = &GfxVulkanTable;

    return G;
}

void gfxVulkanDestroy(Gfx* G)
{
    assert(G);
    free(G);
}

bool gfxVulkanCreateSwapChain(Gfx* G, void* window)
{
    (void)G;
    (void)window;
    return true;
}
bool gfxVulkanResize(Gfx* G, int width, int height)
{
    assert(G);
    (void)width;
    (void)height;
    return true;
}
GfxRenderTarget gfxVulkanGetBackBuffer(Gfx* G)
{
    assert(G);
    return kGfxInvalidHandle;
}
bool gfxVulkanPresent(Gfx* G)
{
    assert(G);
    return true;
}
GfxCmdBuffer* gfxVulkanGetCommandBuffer(Gfx* G)
{
    assert(G);
    return NULL;
}
int gfxVulkanNumAvailableCommandBuffers(Gfx* G)
{
    assert(G);
    return 0;
}
void gfxVulkanResetCommandBuffer(GfxCmdBuffer * B)
{
    assert(B);
}

bool gfxVulkanExecuteCommandBuffer(GfxCmdBuffer * B)
{
    assert(B);
    return false;
}

void gfxVulkanCmdBeginRenderPass(GfxCmdBuffer * B,
                                 GfxRenderTarget renderTargetHandle,
                                 GfxRenderPassAction loadAction,
                                 float const clearColor[4])
{
    assert(B);
    (void)renderTargetHandle;
    (void)loadAction;
    (void)clearColor;
}

void gfxVulkanCmdEndRenderPass(GfxCmdBuffer* B)
{
    assert(B);
}

GfxTable const GfxVulkanTable = {
    gfxVulkanDestroy,
    gfxVulkanCreateSwapChain,
    gfxVulkanResize,
    gfxVulkanGetBackBuffer,
    gfxVulkanPresent,
    gfxVulkanGetCommandBuffer,
    gfxVulkanNumAvailableCommandBuffers,
};
GfxCmdBufferTable const GfxVulkanCmdBufferTable = {
    gfxVulkanResetCommandBuffer,
    gfxVulkanExecuteCommandBuffer,
    gfxVulkanCmdBeginRenderPass,
    gfxVulkanCmdEndRenderPass,
};

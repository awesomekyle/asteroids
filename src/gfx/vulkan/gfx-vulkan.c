#include "gfx/gfx.h"
#include "gfx-vulkan.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(_WIN32)
#   define VK_USE_PLATFORM_WIN32_KHR 1
#else
#   error "Must specify a Vulkan platform"
#endif
#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>

#include "../gfx-internal.h"

#define ARRAY_COUNT(a) (sizeof(a)/sizeof(a[0]))
#define VK_SUCCEEDED(res) (res == VK_SUCCESS)

struct GfxCmdBuffer {
    GfxCmdBufferTable const* table;
};

struct Gfx {
    GfxTable const* table;

    // Extension info
    VkExtensionProperties   availableExtensions[32];
    uint32_t  numAvailableExtensions;

    VkInstance  instance;
};

//////
// Helper methods
//////
static VkResult _GetExtensions(Gfx* G)
{
    G->numAvailableExtensions = ARRAY_COUNT(G->availableExtensions);
    VkResult const result = vkEnumerateInstanceExtensionProperties(NULL,
                                                                   &G->numAvailableExtensions,
                                                                   G->availableExtensions);
    assert(VK_SUCCEEDED(result) && "Could not get extension info");
    return result;
}
static VkResult _CreateInstance(Gfx* G)
{
    // Check for required extensions
    char const* const desiredExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if defined(_DEBUG)
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
    };
    uint32_t const numDesiredExtensions = ARRAY_COUNT(desiredExtensions);

    char const* enabledExtensions[32] = { NULL };
    uint32_t numEnabledExtensions = 0;
    for (uint32_t ii = 0; ii < numDesiredExtensions; ++ii) {
        for (uint32_t jj = 0; jj < G->numAvailableExtensions; ++jj) {
            if (!strcmp(desiredExtensions[ii], G->availableExtensions[jj].extensionName)) {
                enabledExtensions[numEnabledExtensions++] = desiredExtensions[ii];
            }
        }
    }

    // Create instance
    char const* const validationLayers[] = {
        "VK_LAYER_LUNARG_standard_validation",
    };
    uint32_t const numValidationLayers =
#if defined(_DEBUG)
        ARRAY_COUNT(validationLayers);
#else
        0;
#endif
    VkApplicationInfo const applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "GfxVulkan",
        .applicationVersion = 0,
        .pEngineName = "GfxVulkan",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_MAKE_VERSION(1, 0, 0)
    };
    VkInstanceCreateInfo const createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = numValidationLayers,
        .ppEnabledLayerNames = validationLayers,
        .enabledExtensionCount = numEnabledExtensions,
        .ppEnabledExtensionNames = enabledExtensions,
    };
    VkResult const result = vkCreateInstance(&createInfo, NULL, &G->instance);
    assert(VK_SUCCEEDED(result) && "Could not create instance");

    return result;
}

Gfx* gfxVulkanCreate(void)
{
    Gfx* const G = (Gfx*)calloc(1, sizeof(*G));
    assert(G && "Could not allocate space for a Vulkan Gfx device");

    VkResult result = VK_SUCCESS;
    result = _GetExtensions(G);
    assert(VK_SUCCEEDED(result));
    result = _CreateInstance(G);
    assert(VK_SUCCEEDED(result));

    G->table = &GfxVulkanTable;
    return G;
}

void gfxVulkanDestroy(Gfx* G)
{
    assert(G);
    vkDestroyInstance(G->instance, NULL);
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

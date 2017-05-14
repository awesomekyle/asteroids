#include "gfx/gfx.h"
#include "gfx-vulkan.h"
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
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
#include "gfx-vulkan-helper.h"

enum {
    kMaxPhysicalDevices = 8,
};

struct GfxCmdBuffer {
    GfxCmdBufferTable const* table;
};

struct Gfx {
    GfxTable const* table;

    // Extension info
    VkExtensionProperties   availableExtensions[32];
    uint32_t  numAvailableExtensions;

    VkInstance          instance;
    VkPhysicalDevice    availablePhysicalDevices[kMaxPhysicalDevices];
    uint32_t            numPhysicalDevices;

    VkPhysicalDevice    physicalDevice;
    uint32_t            queueIndex;

    VkDevice    device;
    VkQueue     renderQueue;

#if defined(_DEBUG)
    VkDebugReportCallbackEXT debugCallback;
#endif
};

//////
// Helper methods
//////

#if defined(_DEBUG)
#define STRINGIZE_CASE(v) case v: return #v
static char const* _FlagString(VkDebugReportFlagsEXT flag)
{
    switch (flag) {
            STRINGIZE_CASE(VK_DEBUG_REPORT_INFORMATION_BIT_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_WARNING_BIT_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_ERROR_BIT_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_DEBUG_BIT_EXT);
        default:
            return NULL;
    }
}
static char const* _ObjectString(VkDebugReportObjectTypeEXT type)
{
    switch (type) {
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT);
            STRINGIZE_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT);
        default:
            return NULL;
    }
}
#undef STRINGIZE_CASE

static VkBool32 VKAPI_CALL _DebugCallback(VkDebugReportFlagsEXT flags,
                                          VkDebugReportObjectTypeEXT objectType,
                                          uint64_t object,
                                          size_t location,
                                          int32_t messageCode,
                                          const char* pLayerPrefix,
                                          const char* pMessage,
                                          void* pUserData)
{
    Gfx* const G = (Gfx*)(pUserData);
    char message[1024] = { '\0' };
    snprintf(message, sizeof(message), "Vulkan:\n\t%s\n\t%s\n\t%" PRIu64 "\n\t%" PRIu64 "\n\t%d\n\t%s\n\t%s\n\n",
             _FlagString(flags),
             _ObjectString(objectType),
             object,
             location,
             messageCode,
             pLayerPrefix,
             pMessage);
#ifdef _MSC_VER
    OutputDebugStringA(message);
#endif
    fprintf(stderr, "%s\n", message);
    (void)G;
    return VK_TRUE;
}
#endif

static VkResult _GetExtensions(Gfx* G)
{
    assert(G);
    G->numAvailableExtensions = ARRAY_COUNT(G->availableExtensions);
    VkResult const result = vkEnumerateInstanceExtensionProperties(NULL,
                                                                   &G->numAvailableExtensions,
                                                                   G->availableExtensions);
    assert(VK_SUCCEEDED(result) && "Could not get extension info");
    return result;
}
static VkResult _CreateInstance(Gfx* G)
{
    assert(G);
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
    VkResult result = vkCreateInstance(&createInfo, NULL, &G->instance);
    assert(VK_SUCCEEDED(result) && "Could not create instance");


#if defined(_DEBUG)
    PFN_vkCreateDebugReportCallbackEXT const pvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)(vkGetInstanceProcAddr(G->instance, "vkCreateDebugReportCallbackEXT"));
    VkDebugReportFlagsEXT const debugFlags = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                                             VK_DEBUG_REPORT_DEBUG_BIT_EXT |
                                             VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                             // VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                                             VK_DEBUG_REPORT_WARNING_BIT_EXT;
    VkDebugReportCallbackCreateInfoEXT const debugInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT, // sType
        .pNext = NULL, // pNext
        .flags = debugFlags,
        .pfnCallback = &_DebugCallback,// pfnCallback
        .pUserData = G,// pUserData
    };
    result = pvkCreateDebugReportCallbackEXT(G->instance,
                                             &debugInfo,
                                             NULL,
                                             &G->debugCallback);
    assert(VK_SUCCEEDED(result) && "Could not create debug features");
#endif

    return result;
}
static VkResult _GetPhysicalDevices(Gfx* const G)
{
    assert(G);
    G->numPhysicalDevices = ARRAY_COUNT(G->availablePhysicalDevices);
    VkResult result = vkEnumeratePhysicalDevices(G->instance, &G->numPhysicalDevices, NULL);
    assert(VK_SUCCEEDED(result));
    assert(G->numPhysicalDevices <= ARRAY_COUNT(G->availablePhysicalDevices));
    result = vkEnumeratePhysicalDevices(G->instance, &G->numPhysicalDevices, G->availablePhysicalDevices);
    assert(VK_SUCCEEDED(result) && "Could not get physical devices");
    return result;
}
static VkResult _CreateDevice(Gfx* const G)
{
    float const queuePriorities[] = { 1.0f };
    VkDeviceQueueCreateInfo const queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = G->queueIndex,
        .queueCount = 1,
        .pQueuePriorities = queuePriorities,
    };
    char const* const knownExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, // TODO: Check this is supported
    };
    VkDeviceCreateInfo const deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = knownExtensions,
        .pEnabledFeatures = 0,
    };
    VkResult const result = vkCreateDevice(G->physicalDevice,
                                           &deviceInfo,
                                           NULL,
                                           &G->device);
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
    result = _GetPhysicalDevices(G);
    assert(VK_SUCCEEDED(result));

    result = FindBestPhysicalDevice(G->availablePhysicalDevices,
                                    G->numPhysicalDevices,
                                    &G->physicalDevice,
                                    &G->queueIndex);
    assert(VK_SUCCEEDED(result));

    result = _CreateDevice(G);
    assert(VK_SUCCEEDED(result));

    vkGetDeviceQueue(G->device, G->queueIndex, 0, &G->renderQueue);

    G->table = &GfxVulkanTable;
    return G;
}

void gfxVulkanDestroy(Gfx* G)
{
    assert(G);
    vkDeviceWaitIdle(G->device);
    vkDestroyDevice(G->device, NULL);
#if defined(_DEBUG)
    PFN_vkDestroyDebugReportCallbackEXT const pvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)(vkGetInstanceProcAddr(G->instance, "vkDestroyDebugReportCallbackEXT"));
    if (pvkDestroyDebugReportCallbackEXT) {
        pvkDestroyDebugReportCallbackEXT(G->instance, G->debugCallback, NULL);
    }
#endif
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
void gfxVulkanResetCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
}

bool gfxVulkanExecuteCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
    return false;
}

void gfxVulkanCmdBeginRenderPass(GfxCmdBuffer* B,
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

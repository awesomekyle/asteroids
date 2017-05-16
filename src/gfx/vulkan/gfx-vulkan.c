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
    kMaxBackBuffers = 8,
    kMaxCommandBuffers = 128,
};

struct GfxCmdBuffer {
    GfxCmdBufferTable const* table;
    Gfx* G;
    VkCommandPool   pool;
    VkCommandBuffer buffer;
    VkFence         fence;
    bool open;
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

    // Command interface
    VkQueue     renderQueue;
    GfxCmdBuffer commandBuffers[kMaxCommandBuffers];
    uint_fast32_t currentCommandBuffer;

    // Surface & Swap chain
    VkSurfaceKHR    surface;
    VkSemaphore     swapChainSemaphore;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;

    VkSwapchainKHR swapChain;
    VkImage backBuffers[kMaxBackBuffers];
    VkImageView backBufferViews[kMaxBackBuffers];
    uint32_t numBackBuffers;
    uint32_t backBufferIndex;

    VkFramebuffer framebuffers[kMaxBackBuffers];

    // Render pass info
    VkRenderPass    renderPass;

#if defined(_DEBUG)
    VkDebugReportCallbackEXT debugCallback;
#endif
};

//////
// Helper methods
//////

static VkImageSubresourceRange const kFullSubresourceRange = {
    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .baseMipLevel = 0,
    .levelCount = 1,
    .baseArrayLayer = 0,
    .layerCount = 1
};

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
                                             //VK_DEBUG_REPORT_DEBUG_BIT_EXT |
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
static VkResult _CreateRenderPasses(Gfx* const G)
{
    VkAttachmentDescription const attachments[] = {
        {
            .flags = 0,
            .format = VK_FORMAT_B8G8R8A8_SRGB, // Wait until we get this from the surface?
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
    };
    uint32_t const numAttachments = ARRAY_COUNT(attachments);

    VkAttachmentReference const colorAttachmentReferences[] = {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };
    uint32_t const numColorReferences = ARRAY_COUNT(colorAttachmentReferences);

    VkSubpassDescription const subpasses[] = {
        {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = NULL,
            .colorAttachmentCount = numColorReferences,
            .pColorAttachments = colorAttachmentReferences,
            .pResolveAttachments = NULL,
            .pDepthStencilAttachment = NULL,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = NULL,
        },
    };
    uint32_t const numSubpasses = ARRAY_COUNT(subpasses);

    VkRenderPassCreateInfo const renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = numAttachments,
        .pAttachments = attachments,
        .subpassCount = numSubpasses,
        .pSubpasses = subpasses,
        .dependencyCount = 0,
        .pDependencies = NULL,
    };

    VkResult const result = vkCreateRenderPass(G->device, &renderPassInfo, NULL, &G->renderPass);
    assert(VK_SUCCEEDED(result) && "Could not create render pass");

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

    result = _CreateRenderPasses(G);
    assert(VK_SUCCEEDED(result));

    // Create command interfaces
    vkGetDeviceQueue(G->device, G->queueIndex, 0, &G->renderQueue);
    for (uint32_t ii = 0; ii < ARRAY_COUNT(G->commandBuffers); ++ii) {
        GfxCmdBuffer* const currentBuffer = &G->commandBuffers[ii];
        VkCommandPoolCreateInfo const poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = G->queueIndex,
        };
        result = vkCreateCommandPool(G->device, &poolInfo, NULL, &currentBuffer->pool);
        assert(VK_SUCCEEDED(result));
        VkCommandBufferAllocateInfo const bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = currentBuffer->pool,
            .level =  VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        result = vkAllocateCommandBuffers(G->device, &bufferInfo, &currentBuffer->buffer);
        assert(VK_SUCCEEDED(result));

        // Fence
        VkFenceCreateInfo const fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        result = vkCreateFence(G->device, &fenceInfo, NULL, &currentBuffer->fence);
        assert(VK_SUCCEEDED(result));

        currentBuffer->G = G;
        currentBuffer->table = &GfxVulkanCmdBufferTable;
    }



    G->backBufferIndex = UINT32_MAX;
    G->table = &GfxVulkanTable;
    return G;
}

void gfxVulkanDestroy(Gfx* G)
{
    assert(G);
    vkDeviceWaitIdle(G->device);
    vkDestroyRenderPass(G->device, G->renderPass, NULL);
    for (uint32_t ii = 0; ii < ARRAY_COUNT(G->commandBuffers); ++ii) {
        GfxCmdBuffer* const currentBuffer = &G->commandBuffers[ii];
        vkDestroyCommandPool(G->device, currentBuffer->pool, NULL);
        vkDestroyFence(G->device, currentBuffer->fence, NULL);
    }
    for (uint32_t ii = 0; ii < G->numBackBuffers; ++ii) {
        vkDestroyFramebuffer(G->device, G->framebuffers[ii], NULL);
        vkDestroyImageView(G->device, G->backBufferViews[ii], NULL);
    }
    vkDestroySwapchainKHR(G->device, G->swapChain, NULL);
    vkDestroySemaphore(G->device, G->swapChainSemaphore, NULL);
    vkDestroySurfaceKHR(G->instance, G->surface, NULL);
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

bool gfxVulkanCreateSwapChain(Gfx* G, void* window, void* application)
{
    assert(G);
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR const surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hinstance = application,
        .hwnd = window
    };

    VkResult result = vkCreateWin32SurfaceKHR(G->instance,
                                              &surfaceCreateInfo,
                                              NULL,
                                              &G->surface);
    assert(VK_SUCCEEDED(result) && "Could not create surface");

    // Check that the queue supports present
    VkBool32 presentSupported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(G->physicalDevice, G->queueIndex,
                                         G->surface, &presentSupported);
    assert(presentSupported && "Present not supported on selected queue");

    // create semaphore
    VkSemaphoreCreateInfo const semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };
    result = vkCreateSemaphore(G->device,
                               &semaphoreInfo,
                               NULL,
                               &G->swapChainSemaphore);
    assert(VK_SUCCEEDED(result) && "Could not create semaphore");

    // Get surface capabilities
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(G->physicalDevice, G->surface,
                                                       &G->surfaceCapabilities);
    assert(VK_SUCCEEDED(result) && "Could not get surface capabilities");

    // Get supported formats
    VkSurfaceFormatKHR surfaceFormats[32] = { 0 };
    uint32_t numFormats = 0;

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(G->physicalDevice, G->surface,
                                                  &numFormats, NULL);
    assert(VK_SUCCEEDED(result));
    assert(numFormats <= ARRAY_COUNT(surfaceFormats) && "not enough room for all formats");
    numFormats = minu32(numFormats, ARRAY_COUNT(surfaceFormats));
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(G->physicalDevice, G->surface,
                                                  &numFormats, surfaceFormats);
    assert(VK_SUCCEEDED(result));

    // Get supported present modes
    VkPresentModeKHR presentModes[32] = { 0 };
    uint32_t numPresentModes = 0;

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(G->physicalDevice, G->surface,
                                                       &numPresentModes, NULL);
    assert(VK_SUCCEEDED(result));
    assert(numPresentModes <= ARRAY_COUNT(presentModes) && "not enough room for all formats");
    numPresentModes = minu32(numPresentModes, ARRAY_COUNT(presentModes));
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(G->physicalDevice, G->surface,
                                                       &numPresentModes, presentModes);
    assert(VK_SUCCEEDED(result));

    // Select properties
    for (uint32_t ii = 0; ii < numFormats; ++ii) {
        if (surfaceFormats[ii].format == VK_FORMAT_B8G8R8A8_SRGB) {
            G->surfaceFormat = surfaceFormats[ii];
            break;
        }
    }
    assert(G->surfaceFormat.format != VK_FORMAT_UNDEFINED);
    assert(G->surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT && "Cannot clear surface");

    // Trigger creation
    gfxVulkanResize(G, 0, 0);

    return VK_SUCCEEDED(result);
#else
#warning "No swap chain implementation"
    return false
#endif
}
bool gfxVulkanResize(Gfx* G, int width, int height)
{
    assert(G);
    if (G->surface == VK_NULL_HANDLE) {
        return false;
    }
    vkDeviceWaitIdle(G->device);
    VkResult result = VK_SUCCESS;
    // Get surface capabilities
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(G->physicalDevice, G->surface,
                                                       &G->surfaceCapabilities);
    assert(VK_SUCCEEDED(result) && "Could not get surface capabilities");

    VkImageUsageFlags const imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // TODO: check support
    VkPresentModeKHR const presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // TODO: check support
    uint32_t const numImages = G->surfaceCapabilities.minImageCount + 1;

    // Create swap chain
    VkSwapchainCreateInfoKHR const swapChainInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = G->surface,
        .minImageCount = numImages,
        .imageFormat = G->surfaceFormat.format,
        .imageColorSpace = G->surfaceFormat.colorSpace,
        .imageExtent = G->surfaceCapabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = imageUsageFlags,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = G->surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = G->swapChain,
    };
    VkSwapchainKHR newSwapChain = VK_NULL_HANDLE;
    result = vkCreateSwapchainKHR(G->device, &swapChainInfo, NULL, &newSwapChain);
    assert(VK_SUCCEEDED(result) && "Could not create swap chain");
    if (G->swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(G->device, G->swapChain, NULL);
    }
    G->swapChain = newSwapChain;

    // Get images
    result = vkGetSwapchainImagesKHR(G->device, G->swapChain, &G->numBackBuffers, NULL);
    assert(VK_SUCCEEDED(result));
    G->numBackBuffers = minu32(G->numBackBuffers, ARRAY_COUNT(G->backBuffers));
    result = vkGetSwapchainImagesKHR(G->device, G->swapChain, &G->numBackBuffers, G->backBuffers);
    assert(VK_SUCCEEDED(result));

    // Create image views & framebuffers
    for (uint32_t ii = 0; ii < G->numBackBuffers; ++ii) {
        // Image view
        VkImageViewCreateInfo const imageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = G->backBuffers[ii],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = G->surfaceFormat.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = kFullSubresourceRange,
        };
        result = vkCreateImageView(G->device, &imageViewInfo, NULL, &G->backBufferViews[ii]);
        assert(VK_SUCCEEDED(result) && "Could not create image view");

        // Framebuffer
        VkFramebufferCreateInfo const framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = G->renderPass,
            .attachmentCount = 1,
            .pAttachments = &G->backBufferViews[ii],
            .width = G->surfaceCapabilities.currentExtent.width,
            .height = G->surfaceCapabilities.currentExtent.height,
            .layers = 1,
        };
        result = vkCreateFramebuffer(G->device, &framebufferInfo, NULL, &G->framebuffers[ii]);
        assert(VK_SUCCEEDED(result) && "Could not create framebuffer");
    }


    (void)width;
    (void)height;
    return true;
}
GfxRenderTarget gfxVulkanGetBackBuffer(Gfx* G)
{
    assert(G);
    if (G->backBufferIndex != UINT32_MAX) {
        return G->backBufferIndex;
    }
    VkResult const result = vkAcquireNextImageKHR(G->device, G->swapChain, UINT64_MAX,
                                                  G->swapChainSemaphore, VK_NULL_HANDLE,
                                                  &G->backBufferIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        gfxResize(G, 0, 0);
        return gfxVulkanGetBackBuffer(G);
    }
    assert(VK_SUCCEEDED(result) && "Could not get swap chain image index");

    return G->backBufferIndex;
}
bool gfxVulkanPresent(Gfx* G)
{
    assert(G);
    uint32_t const currIndex = (uint32_t)gfxGetBackBuffer(G);

    // Present
    VkPresentInfoKHR const presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &G->swapChainSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &G->swapChain,
        .pImageIndices = &currIndex,
        .pResults = NULL,
    };
    VkResult const result = vkQueuePresentKHR(G->renderQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        gfxResize(G, 0, 0);
    }
    G->backBufferIndex = UINT32_MAX;
    if (result != VK_ERROR_VALIDATION_FAILED_EXT) {
        assert(VK_SUCCEEDED(result) && "Could not get swap chain image index");
    }

    return true;
}
GfxCmdBuffer* gfxVulkanGetCommandBuffer(Gfx* G)
{
    assert(G);
    uint_fast32_t const currIndex = G->currentCommandBuffer++ % kMaxCommandBuffers;
    GfxCmdBuffer* const buffer = &G->commandBuffers[currIndex];
    if (vkGetFenceStatus(G->device, buffer->fence) != VK_SUCCESS || buffer->open != false) {
        /// @TODO: This case needs to be handled
        return NULL;
    }
    gfxVulkanResetCommandBuffer(buffer);
    buffer->open = true;
    return buffer;
}
int gfxVulkanNumAvailableCommandBuffers(Gfx* G)
{
    assert(G);
    int availableCommandBuffers = 0;
    for (uint32_t ii = 0; ii < ARRAY_COUNT(G->commandBuffers); ++ii) {
        if (G->commandBuffers[ii].open == false) {
            availableCommandBuffers++;
        }
    }
    return availableCommandBuffers;
}
void gfxVulkanResetCommandBuffer(GfxCmdBuffer* B)
{
    VkResult result = vkResetFences(B->G->device, 1, &B->fence);
    assert(VK_SUCCEEDED(result) && "Could not reset fence");
    result = vkResetCommandPool(B->G->device, B->pool, 0);
    assert(VK_SUCCEEDED(result) && "Could not reset pool");
    result = vkResetCommandBuffer(B->buffer, 0);
    assert(VK_SUCCEEDED(result) && "Could not reset buffer");
    VkCommandBufferBeginInfo const beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL,
    };
    result = vkBeginCommandBuffer(B->buffer, &beginInfo);
    assert(VK_SUCCEEDED(result) && "Could not begin buffer");

    B->open = false;
}

bool gfxVulkanExecuteCommandBuffer(GfxCmdBuffer* B)
{
    assert(B);
    VkResult result = vkEndCommandBuffer(B->buffer);
    assert(VK_SUCCEEDED(result) && "Could not end command buffer");
    VkSubmitInfo const submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = 0,
        .commandBufferCount = 1,
        .pCommandBuffers = &B->buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL,
    };
    result = vkQueueSubmit(B->G->renderQueue, 1, &submitInfo, B->fence);
    assert(VK_SUCCEEDED(result) && "Could not submit command buffer");
    B->open = false;
    return true;
}

void gfxVulkanCmdBeginRenderPass(GfxCmdBuffer* B,
                                 GfxRenderTarget renderTargetHandle,
                                 GfxRenderPassAction loadAction,
                                 float const clearColor[4])
{
    assert(B);
    if (renderTargetHandle == kGfxInvalidHandle) {
        renderTargetHandle = B->G->backBufferIndex;
    }
    (void)loadAction;
    VkClearValue clearValues[] = {
        {
            .depthStencil = {
                .depth = 0.0f,
                .stencil = 0x0,
            },
        }
    };
    clearValues[0].color = *(VkClearColorValue*)clearColor;
    VkRenderPassBeginInfo const renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = B->G->renderPass,
        .framebuffer = B->G->framebuffers[renderTargetHandle],
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = B->G->surfaceCapabilities.currentExtent,
        },
        .clearValueCount = ARRAY_COUNT(clearValues),
        .pClearValues = clearValues,
    };
    vkCmdBeginRenderPass(B->buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void gfxVulkanCmdEndRenderPass(GfxCmdBuffer* B)
{
    assert(B);
    vkCmdEndRenderPass(B->buffer);
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

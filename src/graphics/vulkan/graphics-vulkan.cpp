#include "graphics-vulkan.h"
#include "graphics/graphics.h"

#include <inttypes.h>
#include <gsl/gsl>

#include "vulkan-debug.h"

#define UNUSED(v) ((void)(v))

namespace {

template<typename T, uint32_t kSize>
constexpr uint32_t array_length(T (&)[kSize])
{
    return kSize;
}
#define VK_SUCCEEDED(res) (res == VK_SUCCESS)

///
/// Constants
///
constexpr char const* kDesiredExtensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if defined(_DEBUG)
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
};
constexpr size_t kNumDesiredExtensions = array_length(kDesiredExtensions);

constexpr char const* kValidationLayers[] = {
    "VK_LAYER_LUNARG_standard_validation",
};
constexpr size_t kNumValidationLayers =
#if defined(_DEBUG)
    array_length(kValidationLayers);
#else
    0;
#endif

constexpr VkImageSubresourceRange kFullSubresourceRange = {
    VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
    0,                          // baseMipLevel
    1,                          // levelCount
    0,                          // baseArrayLayer
    1,                          // layerCount
};

}  // anonymous namespace

namespace ak {

GraphicsVulkan::GraphicsVulkan()
{
    auto const library = LoadLibraryW(L"vulkan-1.dll");
    Ensures(library);
    vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        GetProcAddress(library, "vkGetInstanceProcAddr"));
    Ensures(vkGetInstanceProcAddr);

// Load global functions
#define VK_GLOBAL_FUNCTION(fn) fn = reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr(nullptr, #fn));
#include "vulkan-global-method-list.inl"
#undef VK_GLOBAL_FUNCTION

    get_extensions();
    create_instance();
    create_debug_callback();
    get_physical_devices();
    select_physical_device();
    create_device();
    create_render_passes();
}

GraphicsVulkan::~GraphicsVulkan()
{
    for (auto const& back_buffer : _back_buffer_views) {
        vkDestroyImageView(_device, back_buffer, _vk_allocator);
    }
    for (auto const& framebuffer : _framebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, _vk_allocator);
    }
    vkDestroyRenderPass(_device, _render_pass, _vk_allocator);
    vkDestroySwapchainKHR(_device, _swap_chain, _vk_allocator);
    vkDestroySemaphore(_device, _swap_chain_semaphore, _vk_allocator);
    vkDestroySurfaceKHR(_instance, _surface, _vk_allocator);
    vkDestroyDevice(_device, _vk_allocator);
    vkDestroyDebugReportCallbackEXT(_instance, _debug_report, _vk_allocator);
    vkDestroyInstance(_instance, _vk_allocator);
}

Graphics::API GraphicsVulkan::api_type() const
{
    return kVulkan;
}

bool GraphicsVulkan::create_swap_chain(void* window, void* application)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    auto hwnd = static_cast<HWND>(window);
    auto hinstance = static_cast<HINSTANCE>(application);
    VkWin32SurfaceCreateInfoKHR const surface_create_info = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,  // sType
        nullptr,                                          // pNext
        0,                                                // flags
        hinstance,                                        // hinstance
        hwnd,                                             // hwnd
    };

    VkResult result = vkCreateWin32SurfaceKHR(_instance, &surface_create_info, nullptr, &_surface);
    assert(VK_SUCCEEDED(result) && "Could not create surface");

    // Check that the queue supports present
    VkBool32 present_supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, _queue_index, _surface,
                                         &present_supported);
    assert(present_supported && "Present not supported on selected queue");

    // create semaphore
    VkSemaphoreCreateInfo const semaphore_info = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,  // sType
        nullptr,                                  // pNext
        0                                         // flags
    };
    result = vkCreateSemaphore(_device, &semaphore_info, nullptr, &_swap_chain_semaphore);
    assert(VK_SUCCEEDED(result) && "Could not create semaphore");

    // Get surface capabilities
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface,
                                                       &_surface_capabilities);
    assert(VK_SUCCEEDED(result) && "Could not get surface capabilities");

    // Get supported formats
    uint32_t num_formats = 0;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    do {
        result =
            vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &num_formats, nullptr);
        if (result == VK_SUCCESS && num_formats) {
            surface_formats.resize(num_formats);
            result = vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &num_formats,
                                                          surface_formats.data());
            assert(VK_SUCCEEDED(result));
        }
    } while (result == VK_INCOMPLETE);

    // Get supported present modes
    uint32_t num_present_modes = 0;
    std::vector<VkPresentModeKHR> present_modes;
    do {
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface,
                                                           &num_present_modes, nullptr);
        if (result == VK_SUCCESS && num_present_modes) {
            present_modes.resize(num_present_modes);
            result =
                vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface,
                                                          &num_present_modes, present_modes.data());
            assert(VK_SUCCEEDED(result));
        }
    } while (result == VK_INCOMPLETE);

    // select present mode
    constexpr VkPresentModeKHR desired_modes[] = {
        // TODO(kw): Only use IMMEDIATE for profiling
        VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR,
    };
    for (auto desired : desired_modes) {
        for (auto available : present_modes) {
            if (available == desired) {
                _present_mode = desired;
                break;
            }
        }
    }

    // select surface format
    for (auto const& format : surface_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {  // TODO(kw): support other formats
            _surface_format = format;
            break;
        }
    }

    assert(_surface_format.format != VK_FORMAT_UNDEFINED);
    assert(_surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT &&
           "Cannot clear surface");
    return true;
#else
#warning "No swap chain implementation"
    return false;
#endif
}

bool GraphicsVulkan::resize(int /*width*/, int /*height*/)
{
    Expects(_surface);

    if (_surface == VK_NULL_HANDLE) {
        return false;
    }
    vkDeviceWaitIdle(_device);
    VkResult result = VK_SUCCESS;

    // Get surface capabilities
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface,
                                                       &_surface_capabilities);
    assert(VK_SUCCEEDED(result) && "Could not get surface capabilities");

    VkImageUsageFlags const imageUsageFlags =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;                               // TODO: check support
    VkPresentModeKHR const presentMode = VK_PRESENT_MODE_MAILBOX_KHR;  // TODO: check support
    uint32_t const numImages = _surface_capabilities.minImageCount + 1;

    // Create swap chain
    VkSwapchainCreateInfoKHR const swapChainInfo = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        _surface,                                     // surface
        numImages,                                    // minImageCount
        _surface_format.format,                       // imageFormat
        _surface_format.colorSpace,                   // imageColorSpace
        _surface_capabilities.currentExtent,          // imageExtent
        1,                                            // imageArrayLayers
        imageUsageFlags,                              // imageUsage
        VK_SHARING_MODE_EXCLUSIVE,                    // imageSharingMode
        0,                                            // queueFamilyIndexCount
        nullptr,                                      // pQueueFamilyIndices
        _surface_capabilities.currentTransform,       // preTransform
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,            // compositeAlpha
        presentMode,                                  // presentMode
        VK_TRUE,                                      // clipped
        _swap_chain,                                  // oldSwapchain
    };
    VkSwapchainKHR newSwapChain = VK_NULL_HANDLE;
    result = vkCreateSwapchainKHR(_device, &swapChainInfo, nullptr, &newSwapChain);
    assert(VK_SUCCEEDED(result) && "Could not create swap chain");
    if (_swap_chain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_device, _swap_chain, nullptr);
    }
    _swap_chain = newSwapChain;

    // Get images
    result = vkGetSwapchainImagesKHR(_device, _swap_chain, &_num_back_buffers, nullptr);
    assert(VK_SUCCEEDED(result));
    _num_back_buffers = std::min(_num_back_buffers, array_length(_back_buffers));
    result = vkGetSwapchainImagesKHR(_device, _swap_chain, &_num_back_buffers, _back_buffers);
    assert(VK_SUCCEEDED(result));

    // Create image views & framebuffers
    for (uint32_t ii = 0; ii < _num_back_buffers; ++ii) {
        // Clear originals
        vkDestroyFramebuffer(_device, _framebuffers[ii], nullptr);
        vkDestroyImageView(_device, _back_buffer_views[ii], nullptr);
        // Image view
        VkImageViewCreateInfo const imageViewInfo = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
            nullptr,                                   // pNext
            0,                                         // flags
            _back_buffers[ii],                         // image
            VK_IMAGE_VIEW_TYPE_2D,                     // viewType
            _surface_format.format,                    // format
            // components
            {
                VK_COMPONENT_SWIZZLE_IDENTITY,  // r
                VK_COMPONENT_SWIZZLE_IDENTITY,  // g
                VK_COMPONENT_SWIZZLE_IDENTITY,  // b
                VK_COMPONENT_SWIZZLE_IDENTITY,  // a
            },
            kFullSubresourceRange,  // subresourceRange
        };
        result = vkCreateImageView(_device, &imageViewInfo, nullptr, &_back_buffer_views[ii]);
        assert(VK_SUCCEEDED(result) && "Could not create image view");

        // Framebuffer
        VkFramebufferCreateInfo const framebufferInfo = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,   // sType
            nullptr,                                     // pNext
            0,                                           // flags
            _render_pass,                                // renderPass
            1,                                           // attachmentCount
            &_back_buffer_views[ii],                     // pAttachments
            _surface_capabilities.currentExtent.width,   // width
            _surface_capabilities.currentExtent.height,  // height
            1,                                           // layers
        };
        result = vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_framebuffers[ii]);
        assert(VK_SUCCEEDED(result) && "Could not create framebuffer");
    }
    return true;
}

bool GraphicsVulkan::present()
{
    return false;
}

CommandBuffer* GraphicsVulkan::command_buffer()
{
    return nullptr;
}
int GraphicsVulkan::num_available_command_buffers()
{
    return 0;
}
bool GraphicsVulkan::execute(CommandBuffer* /*command_buffer*/)
{
    return false;
}

void GraphicsVulkan::get_extensions()
{
    uint32_t num_extensions = 0;
    VkResult result = VK_SUCCESS;
    do {
        result = vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);
        if (result == VK_SUCCESS && num_extensions) {
            _available_extensions.resize(num_extensions);
            result = vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions,
                                                            _available_extensions.data());
            assert(VK_SUCCEEDED(result));
        }
    } while (result == VK_INCOMPLETE);
}

void GraphicsVulkan::create_instance()
{
    // Check for available extensions
    std::vector<char const*> enabled_extensions;
    for (auto const& available_ext : _available_extensions) {
        for (auto* const desired_ext : kDesiredExtensions) {
            if (strncmp(available_ext.extensionName, desired_ext, 256) == 0) {
                enabled_extensions.push_back(available_ext.extensionName);
            }
        }
    }

    VkApplicationInfo const application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,  // sType
        nullptr,                             // pNext
        "Asteroids",                         // pApplicationName
        0,                                   // applicationVersion
        "Vulkan Graphics",                   // pEngineName
        VK_MAKE_VERSION(0, 1, 0),            // engineVersion
        VK_MAKE_VERSION(1, 0, 0),            // apiVersion
    };
    VkInstanceCreateInfo const create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,            // sType
        nullptr,                                           // pNext
        0,                                                 // flags
        &application_info,                                 // pApplicationInfo
        kNumValidationLayers,                              // enabledLayerCount
        kValidationLayers,                                 // ppEnabledLayerNames
        static_cast<uint32_t>(enabled_extensions.size()),  // enabledExtensionCount
        enabled_extensions.data(),                         // ppEnabledExtensionNames
    };
    auto const result = vkCreateInstance(&create_info, _vk_allocator, &_instance);
    assert(VK_SUCCEEDED(result) && "Could not create instance");
    Ensures(_instance);

// Load instance methods
#define VK_INSTANCE_FUNCTION(fn) \
    fn = reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr(_instance, #fn));
#include "vulkan-instance-method-list.inl"
#undef VK_INSTANCE_FUNCTION
}

void GraphicsVulkan::create_debug_callback()
{
    Expects(_instance);
#if _DEBUG
    VkDebugReportFlagsEXT const debug_flags = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                                              // VK_DEBUG_REPORT_DEBUG_BIT_EXT |
                                              VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                              // VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                                              VK_DEBUG_REPORT_WARNING_BIT_EXT;
    VkDebugReportCallbackCreateInfoEXT const debug_info = {
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,  // sType
        nullptr,                                         // pNext
        debug_flags,                                     // flags
        &debug_callback,                                 // pfnCallback
        this                                             // pUserData
    };
    auto const result =
        vkCreateDebugReportCallbackEXT(_instance, &debug_info, _vk_allocator, &_debug_report);
    assert(VK_SUCCEEDED(result) && "Could not create debug features");
#endif
}

void GraphicsVulkan::get_physical_devices()
{
    uint32_t num_devices = 0;
    VkResult result = VK_SUCCESS;
    do {
        result = vkEnumeratePhysicalDevices(_instance, &num_devices, nullptr);
        if (result == VK_SUCCESS && num_devices) {
            _all_physical_devices.resize(num_devices);
            result =
                vkEnumeratePhysicalDevices(_instance, &num_devices, _all_physical_devices.data());
            assert(VK_SUCCEEDED(result));
        }
    } while (result == VK_INCOMPLETE);
}

void GraphicsVulkan::select_physical_device()
{
    VkPhysicalDevice best_physical_device = VK_NULL_HANDLE;
    for (auto const& device : _all_physical_devices) {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            best_physical_device = device;
            break;
        }
    }
    assert(best_physical_device);

    // TODO(kw): Support multiple queue types
    uint32_t num_queues = 0;
    std::vector<VkQueueFamilyProperties> queue_properties;
    vkGetPhysicalDeviceQueueFamilyProperties(best_physical_device, &num_queues, nullptr);
    if (num_queues) {
        queue_properties.resize(num_queues);
        vkGetPhysicalDeviceQueueFamilyProperties(best_physical_device, &num_queues,
                                                 queue_properties.data());
    }
    for (uint32_t ii = 0; ii < static_cast<uint32_t>(queue_properties.size()); ++ii) {
        if (queue_properties[ii].queueCount >= 1 &&
            queue_properties[ii].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            _queue_index = ii;
            break;
        }
    }
    assert(_queue_index != UINT32_MAX);

    _physical_device = best_physical_device;
}

void GraphicsVulkan::create_device()
{
    constexpr float queuePriorities[] = {1.0f};
    VkDeviceQueueCreateInfo const queue_info = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,  // sType
        nullptr,                                     // pNext
        0,                                           // flags
        _queue_index,                                // queueFamilyIndex
        1,                                           // queueCount
        queuePriorities,                             // pQueuePriorities
    };
    constexpr char const* known_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,  // TODO: Check this is supported
    };
    constexpr uint32_t const num_known_extensions = array_length(known_extensions);
    VkDeviceCreateInfo const device_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        1,                                     // queueCreateInfoCount
        &queue_info,                           // pQueueCreateInfos
        0,                                     // enabledLayerCount
        nullptr,                               // ppEnabledLayerNames
        num_known_extensions,                  // enabledExtensionCount
        known_extensions,                      // ppEnabledExtensionNames
        0,                                     // pEnabledFeatures
    };
    VkResult const result = vkCreateDevice(_physical_device, &device_info, nullptr, &_device);
    assert(VK_SUCCEEDED(result) && "Could not create device");

// Load device methods
#define VK_DEVICE_FUNCTION(fn) \
    fn = reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr(_instance, #fn));
#include "vulkan-device-method-list.inl"
#undef VK_DEVICE_FUNCTION
}

void GraphicsVulkan::create_render_passes()
{
    constexpr VkAttachmentDescription attachments[] = {
        {
            0,                                 // flags
            VK_FORMAT_B8G8R8A8_SRGB,           // format // Wait until we get this from the surface?
            VK_SAMPLE_COUNT_1_BIT,             // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
            VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencilStoreOp
            VK_IMAGE_LAYOUT_UNDEFINED,         // initialLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,   // finalLayout
        },
    };
    constexpr uint32_t const num_attachments = array_length(attachments);

    constexpr VkAttachmentReference const color_attachment_references[] = {
        {
            0,                                         // attachment
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // layout
        },
    };
    constexpr uint32_t const num_color_references = array_length(color_attachment_references);

    constexpr VkSubpassDescription const subpasses[] = {
        {
            0,                                // flags
            VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
            0,                                // inputAttachmentCount
            nullptr,                          // pInputAttachments
            num_color_references,             // colorAttachmentCount
            color_attachment_references,      // pColorAttachments
            nullptr,                          // pResolveAttachments
            nullptr,                          // pDepthStencilAttachment
            0,                                // preserveAttachmentCount
            nullptr,                          // pPreserveAttachments
        },
    };
    constexpr uint32_t const num_subpasses = array_length(subpasses);

    constexpr VkRenderPassCreateInfo const render_pass_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        num_attachments,                            // attachmentCount
        attachments,                                // pAttachments
        num_subpasses,                              // subpassCount
        subpasses,                                  // pSubpasses
        0,                                          // dependencyCount
        nullptr,                                    // pDependencies
    };

    VkResult const result = vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass);
    assert(VK_SUCCEEDED(result) && "Could not create render pass");
}

ScopedGraphics create_graphics_vulkan()
{
    return std::make_unique<GraphicsVulkan>();
}

}  // namespace ak

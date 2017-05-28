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
}

GraphicsVulkan::~GraphicsVulkan()
{
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
    return false;
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

ScopedGraphics create_graphics_vulkan()
{
    return std::make_unique<GraphicsVulkan>();
}

}  // namespace ak

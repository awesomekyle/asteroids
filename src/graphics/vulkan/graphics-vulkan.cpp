#include "graphics-vulkan.h"
#include "graphics/graphics.h"

#include <inttypes.h>
#include <gsl/gsl>

#include "vulkan-debug.h"

#define UNUSED(v) ((void)(v))

namespace {

template<typename T, size_t kSize>
constexpr size_t array_length(T (&)[kSize])
{
    return kSize;
}

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
    get_extensions();
    create_instance();
    create_debug_callback();
    get_physical_devices();
    select_physical_device();
    create_device();
}

GraphicsVulkan::~GraphicsVulkan()
{
#if defined(_DEBUG)
    auto const pvkDestroyDebugReportCallbackEXT =
        reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT"));
    pvkDestroyDebugReportCallbackEXT(_instance, _debug_report, NULL);
#endif
    _instance.destroy();
}

Graphics::API GraphicsVulkan::api_type() const
{
    return kVulkan;
}

bool GraphicsVulkan::create_swap_chain(void* /*window*/, void* /*application*/)
{
    return false;
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
    auto extension_result = vk::enumerateInstanceExtensionProperties();
    if (extension_result.result != vk::Result::eSuccess) {
        return;
    }
    _available_extensions = extension_result.value;
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

    vk::ApplicationInfo const application_info("Vulkan Graphics", VK_MAKE_VERSION(0, 0, 1),
                                               "Vulkan Graphics Engine", VK_MAKE_VERSION(0, 0, 1),
                                               VK_API_VERSION_1_0);
    vk::InstanceCreateInfo const instance_info(vk::InstanceCreateFlags::Flags(), &application_info,
                                               kNumValidationLayers, kValidationLayers,
                                               static_cast<uint32_t>(enabled_extensions.size()),
                                               enabled_extensions.data());

    auto const instance_result = vk::createInstance(instance_info);
    if (instance_result.result != vk::Result::eSuccess) {
        return;
    }

    _instance = instance_result.value;
}

void GraphicsVulkan::create_debug_callback()
{
    Expects(_instance);
#if _DEBUG
    auto const pvkCreateDebugReportCallbackEXT =
        reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT"));
    vk::DebugReportFlagsEXT const debug_flags =
        vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eError |
        // vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eInformation |
        vk::DebugReportFlagBitsEXT::eWarning;
    vk::DebugReportCallbackCreateInfoEXT const debug_info(debug_flags, debug_callback, this);

    auto const debug_result =
        pvkCreateDebugReportCallbackEXT(_instance,
                                        reinterpret_cast<VkDebugReportCallbackCreateInfoEXT const*>(
                                            &debug_info),
                                        nullptr,
                                        reinterpret_cast<VkDebugReportCallbackEXT*>(
                                            &_debug_report));
    assert(debug_result == VK_SUCCESS);
#endif
}

void GraphicsVulkan::get_physical_devices()
{
    auto const devices_result = _instance.enumeratePhysicalDevices();
    if (devices_result.result != vk::Result::eSuccess) {
        return;
    }
    _all_physical_devices = devices_result.value;
}

void GraphicsVulkan::select_physical_device()
{
    vk::PhysicalDevice best_physical_device;
    for (auto const& device : _all_physical_devices) {
        auto const properties = device.getProperties();
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            best_physical_device = device;
            break;
        }
    }

    assert(best_physical_device);

    // TODO(kw): Support multiple queue types
    auto const queue_properties = best_physical_device.getQueueFamilyProperties();
    for (uint32_t ii = 0; ii < static_cast<uint32_t>(queue_properties.size()); ++ii) {
        if (queue_properties[ii].queueCount >= 1 &&
            queue_properties[ii].queueFlags & vk::QueueFlagBits::eGraphics) {
            _queue_index = ii;
            break;
        }
    }
    assert(_queue_index != UINT32_MAX);
}

void GraphicsVulkan::create_device()
{
    constexpr float queue_priorities[] = {1.0f};
    vk::DeviceQueueCreateInfo const queue_info(vk::DeviceQueueCreateFlags::Flags(), _queue_index, 1,
                                               queue_priorities);

    constexpr char const* known_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    vk::DeviceCreateInfo const device_info(vk::DeviceCreateFlags::Flags(), 1, &queue_info, 0,
                                           nullptr, 1, known_extensions);

    auto const device_result = _physical_device.createDevice(device_info);
    if (device_result.result != vk::Result::eSuccess) {
        return;
    }
    _device = device_result.value;
}

ScopedGraphics create_graphics_vulkan()
{
    return std::make_unique<GraphicsVulkan>();
}

}  // namespace ak

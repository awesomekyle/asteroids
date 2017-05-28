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

ScopedGraphics create_graphics_vulkan()
{
    return std::make_unique<GraphicsVulkan>();
}

}  // namespace ak

#include "graphics-vulkan.h"
#include "graphics/graphics.h"

#include <inttypes.h>
#include <gsl/gsl>

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

#if defined(_DEBUG)
#define STRINGIZE_CASE(v) \
    case v:               \
        return #v
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

static VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags,
                                          VkDebugReportObjectTypeEXT objectType, uint64_t object,
                                          size_t location, int32_t messageCode,
                                          const char* pLayerPrefix, const char* pMessage,
                                          void* /*pUserData*/)
{
    char message[1024] = {'\0'};
    snprintf(message, sizeof(message),
             "Vulkan:\n\t%s\n\t%s\n\t%" PRIu64 "\n\t%" PRIu64 "\n\t%d\n\t%s\n\t%s\n\n",
             _FlagString(flags), _ObjectString(objectType), object, location, messageCode,
             pLayerPrefix, pMessage);
#ifdef _MSC_VER
    OutputDebugStringA(message);
#endif
    fprintf(stderr, "%s\n", message);
    return VK_TRUE;
}
#endif

}  // anonymous namespace

namespace ak {

GraphicsVulkan::GraphicsVulkan()
{
    (void)&debug_callback;
}
GraphicsVulkan::~GraphicsVulkan()
{
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

void GraphicsVulkan::create_instance()
{
}

ScopedGraphics create_graphics_vulkan()
{
    return std::make_unique<GraphicsVulkan>();
}

}  // namespace ak

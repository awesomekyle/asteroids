#ifndef _VULKAN_DEBUG_H_
#define _VULKAN_DEBUG_H_

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace ak {

#if defined(_DEBUG)
#define STRINGIZE_CASE(v) \
    case v:               \
        return #v

constexpr char const* debug_flag_string(VkDebugReportFlagsEXT flag)
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
constexpr char const* debug_object_type_string(VkDebugReportObjectTypeEXT type)
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

inline VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags,
                                          VkDebugReportObjectTypeEXT objectType, uint64_t object,
                                          size_t location, int32_t messageCode,
                                          const char* pLayerPrefix, const char* pMessage,
                                          void* /*pUserData*/)
{
    char message[1024] = {'\0'};
    snprintf(message, sizeof(message),
             "Vulkan:\n\t%s\n\t%s\n\t%" PRIu64 "\n\t%" PRIu64 "\n\t%d\n\t%s\n\t%s\n\n",
             debug_flag_string(flags), debug_object_type_string(objectType), object, location,
             messageCode, pLayerPrefix, pMessage);
#ifdef _MSC_VER
    OutputDebugStringA(message);
#endif
    fprintf(stderr, "%s\n", message);
    return VK_TRUE;
}
#endif
}

#endif  // _VULKAN_DEBUG_H_
#ifndef _AK_GRAPHICS_VULKAN_H_
#define _AK_GRAPHICS_VULKAN_H_
#include "graphics/graphics.h"

#include <array>
#include <vector>
#include <atomic>
#include <gsl/gsl_assert>

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR 1
#else
#error "Must specify a Vulkan platform"
#endif
#define VK_NO_PROTOTYPES
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>

#include "command-buffer-vulkan.h"

namespace ak {

class GraphicsVulkan : public Graphics
{
   public:
    GraphicsVulkan();
    ~GraphicsVulkan() final;

    API api_type() const final;
    bool create_swap_chain(void* window, void*) final;
    bool resize(int, int) final;
    bool present() final;
    CommandBuffer* command_buffer() final;
    int num_available_command_buffers() final;
    bool execute(CommandBuffer* command_buffer) final;

   private:
    friend class CommandBufferVulkan;

    GraphicsVulkan(const GraphicsVulkan&) = delete;
    GraphicsVulkan& operator=(const GraphicsVulkan&) = delete;
    GraphicsVulkan(GraphicsVulkan&&) = delete;
    GraphicsVulkan& operator=(GraphicsVulkan&&) = delete;

    void get_extensions();
    void create_instance();
    void create_debug_callback();
    void get_physical_devices();
    void select_physical_device();
    void create_device();

    //
    // constants
    //
    VkAllocationCallbacks const* const _vk_allocator = nullptr;  // TODO(kw): change if needed

    //
    // Vulkan function pointers
    //
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;

#define DECLARE_VK_FUNCTION(fn) PFN_##fn fn = nullptr;
#define VK_GLOBAL_FUNCTION(fn) DECLARE_VK_FUNCTION(fn)
#define VK_INSTANCE_FUNCTION(fn) DECLARE_VK_FUNCTION(fn)
#define VK_DEVICE_FUNCTION(fn) DECLARE_VK_FUNCTION(fn)
#include "vulkan-global-method-list.inl"
#include "vulkan-instance-method-list.inl"
#include "vulkan-device-method-list.inl"
#undef VK_GLOBAL_FUNCTION
#undef VK_INSTANCE_FUNCTION
#undef VK_DEVICE_FUNCTION

    //
    // data members
    //
    std::vector<VkExtensionProperties> _available_extensions;

    // TODO(kw): Add smart pointers for RAII
    VkInstance _instance = VK_NULL_HANDLE;

    std::vector<VkPhysicalDevice> _all_physical_devices;

    VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
    uint32_t _queue_index = UINT32_MAX;

    VkDevice _device = VK_NULL_HANDLE;

#if defined(_DEBUG)
    VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
#endif
};

/// @brief Creates a Vulkan Graphics device
ScopedGraphics create_graphics_vulkan();

}  // namespace ak

#endif  // _AK_GRAPHICS_VULKAN_H_
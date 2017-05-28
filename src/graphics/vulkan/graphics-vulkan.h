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
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vk_platform.h>
#pragma warning(push)
#pragma warning(disable : 4100)  // unreferenced parameter
#include <vulkan/vulkan.hpp>
#pragma warning(pop)

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
    // data members
    //
    std::vector<vk::ExtensionProperties> _available_extensions;

    // TODO(kw): Enable extensions to enable vulkan smart pointers
    vk::Instance _instance;

    std::vector<vk::PhysicalDevice> _all_physical_devices;

    vk::PhysicalDevice _physical_device;
    uint32_t _queue_index = UINT32_MAX;

    vk::Device _device;

#if defined(_DEBUG)
    vk::DebugReportCallbackEXT _debug_report;
#endif
};

/// @brief Creates a Vulkan Graphics device
ScopedGraphics create_graphics_vulkan();

}  // namespace ak

#endif  // _AK_GRAPHICS_VULKAN_H_

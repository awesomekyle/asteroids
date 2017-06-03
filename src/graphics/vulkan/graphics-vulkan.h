#ifndef _AK_GRAPHICS_VULKAN_H_
#define _AK_GRAPHICS_VULKAN_H_
#include "graphics/graphics.h"

#include <array>
#include <vector>
#include <atomic>
#include <gsl/gsl_assert>

#define VK_NO_PROTOTYPES
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>

#include "command-buffer-vulkan.h"

#define VK_SUCCEEDED(res) (res == VK_SUCCESS)

template<typename T, uint32_t kSize>
constexpr uint32_t array_length(T (&)[kSize])
{
    return kSize;
}

namespace ak {

class RenderStateVulkan : public RenderState
{
   public:
    ~RenderStateVulkan();

    class GraphicsVulkan* _graphics = nullptr;
    VkShaderModule _vs_module = VK_NULL_HANDLE;
    VkShaderModule _ps_module = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkPipeline _pso = VK_NULL_HANDLE;
};

class VertexBufferVulkan : public VertexBuffer
{
   public:
    ~VertexBufferVulkan();

    class GraphicsVulkan* _graphics = nullptr;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkBuffer _buffer = VK_NULL_HANDLE;
};

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
    std::unique_ptr<RenderState> create_render_state(RenderStateDesc const& desc) final;
    std::unique_ptr<VertexBuffer> create_vertex_buffer(uint32_t size, void const* data) final;

   private:
    friend class CommandBufferVulkan;
    friend class RenderStateVulkan;
    friend class VertexBufferVulkan;

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
    void create_render_passes();
    void create_command_buffers();
    uint32_t get_memory_type_index(VkMemoryRequirements const& requirements,
                                   VkMemoryPropertyFlags const property_flags);

    uint32_t get_back_buffer();

    //
    // constants
    //
    static constexpr uint32_t kMaxBackBuffers = 8;
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

    // Surface
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkSemaphore _swap_chain_semaphore = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR _surface_capabilities = {};
    VkPresentModeKHR _present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkSurfaceFormatKHR _surface_format = {
        VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };

    // Swap chain
    VkSwapchainKHR _swap_chain = VK_NULL_HANDLE;
    VkImage _back_buffers[kMaxBackBuffers] = {};
    VkImageView _back_buffer_views[kMaxBackBuffers] = {};
    uint32_t _num_back_buffers = 0;
    uint32_t _back_buffer_index = UINT32_MAX;
    VkFramebuffer _framebuffers[kMaxBackBuffers] = {};

    // Render pass info
    VkRenderPass _render_pass = VK_NULL_HANDLE;

    // execution
    VkQueue _render_queue = VK_NULL_HANDLE;
    std::array<CommandBufferVulkan, kMaxCommandBuffers> _command_buffers;
    std::atomic<uint32_t> _current_command_buffer = {};

#if defined(_DEBUG)
    VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
#endif
};

/// @brief Creates a Vulkan Graphics device
ScopedGraphics create_graphics_vulkan();

}  // namespace ak

#endif  // _AK_GRAPHICS_VULKAN_H_

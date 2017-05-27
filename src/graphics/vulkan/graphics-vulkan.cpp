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

}  // anonymous namespace

namespace ak {

GraphicsVulkan::GraphicsVulkan()
{
    auto const vulkan_lib = LoadLibraryW(L"vulkan-1.dll");
    Ensures(vulkan_lib);
    vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        GetProcAddress(vulkan_lib, "vkGetInstanceProcAddr"));
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

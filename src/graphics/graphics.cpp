#include "graphics/graphics.h"
#if defined(_WIN32)
#include "d3d12/graphics-d3d12.h"
#include "vulkan/graphics-vulkan.h"
#endif

namespace {

constexpr ak::Graphics::API platform_default_api()
{
#if defined(_WIN32)
    return ak::Graphics::kD3D12;
#elif defined(__APPLE__)
    return ak::Graphics::kMetal;
#else
#error Must specify default API
    return ak::Graphics::kUnknown;
#endif
}

}  // anonymous namespace

namespace ak {

Graphics::~Graphics() = default;

ScopedGraphics create_graphics(Graphics::API api)
{
    if (api == Graphics::kDefault) {
        api = platform_default_api();
    }
    switch (api) {
#if defined(_WIN32)
        case ak::Graphics::kD3D12:
            return create_graphics_d3d12();
        case ak::Graphics::kVulkan:
            return create_graphics_vulkan();
#endif  // _WIN32
#if defined(__APPLE__)
        case ak::Graphics::kMetal:
            break;
#endif  // __APPLE__
        case ak::Graphics::kDefault:
        case ak::Graphics::kUnknown:
        default:
            break;
    }
    return nullptr;
}

}  // namespace ak

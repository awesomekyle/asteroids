#pragma once
#if defined(_WIN32)
#   define VK_USE_PLATFORM_WIN32_KHR 1
#else
#   error "Must specify a Vulkan platform"
#endif
#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>

// TODO: Add this helper. It's a pain because it needs so much information from
// the parent: instance, physical device, device, queue family, etc

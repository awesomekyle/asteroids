#pragma once
#include "gfx/gfx.h"
#include <vulkan/vulkan.h>

#define ARRAY_COUNT(a) (sizeof(a)/sizeof(a[0]))
#define VK_SUCCEEDED(res) (res == VK_SUCCESS)

inline uint32_t maxu32(uint32_t const a, uint32_t const b)
{
    if (a > b) {
        return a;
    }
    return b;
}
inline uint32_t minu32(uint32_t const a, uint32_t const b)
{
    if (a < b) {
        return a;
    }
    return b;
}

/// @brief Find's a physical device supporting the specified properties
/// @note By default, looks for the first GPU that supports a graphics queue
VkResult FindBestPhysicalDevice(VkPhysicalDevice const* availableDevices,
                                uint32_t numAvailableDevices,
                                VkPhysicalDevice* outDevice,
                                uint32_t* outQueueIndex);

VkFormat VkFormatFromGfxFormat(GfxPixelFormat format);

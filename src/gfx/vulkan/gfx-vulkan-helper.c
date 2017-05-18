#include "gfx-vulkan-helper.h"
#include <assert.h>

enum {
    kMaxQueues = 64,
};

static uint32_t _FindBestQueue(VkQueueFamilyProperties const* const queueProperties,
                               uint32_t const numQueues,
                               uint32_t const minRequiredQueues,
                               VkQueueFlags const requiredFlags)
{
    for (uint32_t ii = 0; ii < numQueues; ii++) {
        if (queueProperties[ii].queueCount >= minRequiredQueues &&
                queueProperties[ii].queueFlags & requiredFlags) {
            return ii;
        }
    }
    return UINT32_MAX;
}

VkResult FindBestPhysicalDevice(VkPhysicalDevice const* availableDevices,
                                uint32_t numAvailableDevices,
                                VkPhysicalDevice* outDevice,
                                uint32_t* outQueueIndex)
{
    VkPhysicalDevice bestPhysicalDevice = VK_NULL_HANDLE;

    // Find the first discrete card
    for (uint32_t ii = 0; ii < numAvailableDevices; ++ii) {
        VkPhysicalDeviceProperties deviceProperties = { 0 };
        vkGetPhysicalDeviceProperties(availableDevices[ii], &deviceProperties);
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            bestPhysicalDevice = availableDevices[ii];
            break;
        }
    }

    if (bestPhysicalDevice == VK_NULL_HANDLE) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    // Find the correct queue
    VkQueueFamilyProperties queueProperties[kMaxQueues] = { 0 };
    uint32_t numAvailableQueues = 0;

    // Get count
    vkGetPhysicalDeviceQueueFamilyProperties(bestPhysicalDevice,
                                             &numAvailableQueues,
                                             NULL);
    assert(numAvailableQueues <= ARRAY_COUNT(queueProperties));

    // Get properties
    numAvailableQueues = minu32(numAvailableQueues, ARRAY_COUNT(queueProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(bestPhysicalDevice,
                                             &numAvailableQueues,
                                             queueProperties);

    uint32_t const queueIndex = _FindBestQueue(queueProperties, numAvailableQueues,
                                               1, VK_QUEUE_GRAPHICS_BIT);

    if (queueIndex == UINT32_MAX) {
        assert(queueIndex != UINT32_MAX && "Could not find queue with correct properties");
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    if (outDevice) {
        *outDevice = bestPhysicalDevice;
    }
    if (outQueueIndex) {
        *outQueueIndex = queueIndex;
    }

    return VK_SUCCESS;
}

VkFormat VkFormatFromGfxFormat(GfxPixelFormat format)
{
    switch (format) {
        case kUnknown:
            return VK_FORMAT_UNDEFINED;
        // r8
        case kR8Uint:
            return VK_FORMAT_R8_UINT;
        case kR8Sint:
            return VK_FORMAT_R8_SINT;
        case kR8SNorm:
            return VK_FORMAT_R8_SNORM;
        case kR8UNorm:
            return VK_FORMAT_R8_UNORM;
        // rg8
        case kRG8Uint:
            return VK_FORMAT_R8G8_UINT;
        case kRG8Sint:
            return VK_FORMAT_R8G8_SINT;
        case kRG8SNorm:
            return VK_FORMAT_R8G8_SNORM;
        case kRG8UNorm:
            return VK_FORMAT_R8G8_UNORM;
        // r8g8b8
        case kRGBX8Uint:
            return VK_FORMAT_R8G8B8A8_UINT;
        case kRGBX8Sint:
            return VK_FORMAT_R8G8B8A8_SINT;
        case kRGBX8SNorm:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case kRGBX8UNorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case kRGBX8UNorm_sRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        // r8g8b8a8
        case kRGBA8Uint:
            return VK_FORMAT_R8G8B8A8_UINT;
        case kRGBA8Sint:
            return VK_FORMAT_R8G8B8A8_SINT;
        case kRGBA8SNorm:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case kRGBA8UNorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case kRGBA8UNorm_sRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        // b8g8r8a8
        case kBGRA8UNorm:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case kBGRA8UNorm_sRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;
        // r16
        case kR16Uint:
            return VK_FORMAT_R16_UINT;
        case kR16Sint:
            return VK_FORMAT_R16_SINT;
        case kR16Float:
            return VK_FORMAT_R16_SFLOAT;
        case kR16SNorm:
            return VK_FORMAT_R16_SNORM;
        case kR16UNorm:
            return VK_FORMAT_R16_UNORM;
        // r16g16
        case kRG16Uint:
            return VK_FORMAT_R16G16_UINT;
        case kRG16Sint:
            return VK_FORMAT_R16G16_SINT;
        case kRG16Float:
            return VK_FORMAT_R16G16_SFLOAT;
        case kRG16SNorm:
            return VK_FORMAT_R16G16_SNORM;
        case kRG16UNorm:
            return VK_FORMAT_R16G16_UNORM;
        // r16g16b16a16
        case kRGBA16Uint:
            return VK_FORMAT_R16G16B16A16_UINT;
        case kRGBA16Sint:
            return VK_FORMAT_R16G16B16A16_SINT;
        case kRGBA16Float:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case kRGBA16SNorm:
            return VK_FORMAT_R16G16B16A16_SNORM;
        case kRGBA16UNorm:
            return VK_FORMAT_R16G16B16A16_UNORM;
        // r32
        case kR32Uint:
            return VK_FORMAT_R32_UINT;
        case kR32Sint:
            return VK_FORMAT_R32_SINT;
        case kR32Float:
            return VK_FORMAT_R32_SFLOAT;
        // r32g32
        case kRG32Uint:
            return VK_FORMAT_R32G32_UINT;
        case kRG32Sint:
            return VK_FORMAT_R32G32_SINT;
        case kRG32Float:
            return VK_FORMAT_R32G32_SFLOAT;
        // r32g32b32x32
        case kRGBX32Uint:
            return VK_FORMAT_R32G32B32_UINT;
        case kRGBX32Sint:
            return VK_FORMAT_R32G32B32_SINT;
        case kRGBX32Float:
            return VK_FORMAT_R32G32B32_SFLOAT;
        // r32g32b32a32
        case kRGBA32Uint:
            return VK_FORMAT_R32G32B32A32_UINT;
        case kRGBA32Sint:
            return VK_FORMAT_R32G32B32A32_SINT;
        case kRGBA32Float:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        // rbg10a2
        case kRGB10A2Uint:
            return VK_FORMAT_A2R10G10B10_UINT_PACK32;
        case kRGB10A2UNorm:
            return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        // r11g11b10
        case kRG11B10Float:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        // rgb9e5
        case kRGB9E5Float:
            return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
        // bgr5a1
        case kBGR5A1Unorm:
            return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
        // b5g6r5
        case kB5G6R5Unorm:
            return VK_FORMAT_B5G6R5_UNORM_PACK16;
        // b4g4r4a4
        case kBGRA4Unorm:
            return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
        // bc1-7
        case kBC1Unorm:
            return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case kBC1Unorm_SRGB:
            return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case kBC2Unorm:
            return VK_FORMAT_BC2_UNORM_BLOCK;
        case kBC2Unorm_SRGB:
            return VK_FORMAT_BC2_SRGB_BLOCK;
        case kBC3Unorm:
            return VK_FORMAT_BC3_UNORM_BLOCK;
        case kBC3Unorm_SRGB:
            return VK_FORMAT_BC3_SRGB_BLOCK;
        case kBC4Unorm:
            return VK_FORMAT_BC4_UNORM_BLOCK;
        case kBC4Snorm:
            return VK_FORMAT_BC4_SNORM_BLOCK;
        case kBC5Unorm:
            return VK_FORMAT_BC5_UNORM_BLOCK;
        case kBC5Snorm:
            return VK_FORMAT_BC5_SNORM_BLOCK;
        case kBC6HUfloat:
            return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case kBC6HSfloat:
            return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        case kBC7Unorm:
            return VK_FORMAT_BC7_UNORM_BLOCK;
        case kBC7Unorm_SRGB:
            return VK_FORMAT_BC7_SRGB_BLOCK;
        // d24s8
        case kD24UnormS8Uint:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        // d32
        case kD32Float:
            return VK_FORMAT_D32_SFLOAT;
        // d32s8
        case kD32FloatS8Uint:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
        default:
            break;
    }
    return VK_FORMAT_UNDEFINED;
}

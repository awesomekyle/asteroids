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

#include <vulkan/vulkan.h>

extern PFN_vkVoidFunction get_real_proc_addr(VkInstance instance, const char* pName);

VkResult call_real_vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties)
{
    auto real_fn = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        get_real_proc_addr(nullptr, "vkEnumerateDeviceExtensionProperties"));
    if (!real_fn) return VK_ERROR_INITIALIZATION_FAILED;
    return real_fn(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

void call_real_vkGetPhysicalDeviceFeatures2(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures2* pFeatures)
{
    auto real_fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        get_real_proc_addr(nullptr, "vkGetPhysicalDeviceFeatures2"));
    if (!real_fn) {
        real_fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
            get_real_proc_addr(nullptr, "vkGetPhysicalDeviceFeatures2KHR"));
    }
    if (real_fn) real_fn(physicalDevice, pFeatures);
}

VkResult call_real_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    auto real_fn = reinterpret_cast<PFN_vkCreateDevice>(
        get_real_proc_addr(nullptr, "vkCreateDevice"));
    if (!real_fn) return VK_ERROR_INITIALIZATION_FAILED;
    return real_fn(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

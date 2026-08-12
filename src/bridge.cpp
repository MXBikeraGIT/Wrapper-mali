#include <vulkan/vulkan.h>
#include <cstring>

// Forward declarations for functions defined in logic.cpp / output.cpp
extern "C" {
    void init_android_vulkan();
    PFN_vkVoidFunction get_real_proc_addr(VkInstance instance, const char* pName);
    VkResult logic_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
    VkResult logic_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);
    VkResult logic_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
    void logic_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures);
    VkResult logic_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    init_android_vulkan();

    if (strcmp(pName, "vkCreateShaderModule") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateShaderModule);
    if (strcmp(pName, "vkCreateInstance") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateInstance);
    if (strcmp(pName, "vkEnumerateDeviceExtensionProperties") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkEnumerateDeviceExtensionProperties);
    if (strcmp(pName, "vkGetPhysicalDeviceFeatures2") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkGetPhysicalDeviceFeatures2);
    if (strcmp(pName, "vkCreateDevice") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateDevice);

    return get_real_proc_addr(instance, pName);
}

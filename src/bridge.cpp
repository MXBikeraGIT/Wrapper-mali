// External feature logic from logic.cpp
extern VkResult logic_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
extern void logic_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures);
extern VkResult logic_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);

extern "C" {

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    init_android_vulkan();

    if (!pName) return nullptr;

    if (strcmp(pName, "vkCreateShaderModule") == 0) return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateShaderModule);
    if (strcmp(pName, "vkCreateInstance") == 0) return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateInstance);
    if (strcmp(pName, "vkEnumerateDeviceExtensionProperties") == 0) return reinterpret_cast<PFN_vkVoidFunction>(logic_vkEnumerateDeviceExtensionProperties);
    if (strcmp(pName, "vkGetPhysicalDeviceFeatures2") == 0 || strcmp(pName, "vkGetPhysicalDeviceFeatures2KHR") == 0) return reinterpret_cast<PFN_vkVoidFunction>(logic_vkGetPhysicalDeviceFeatures2);
    if (strcmp(pName, "vkCreateDevice") == 0) return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateDevice);

    return get_real_proc_addr(instance, pName);
}

}

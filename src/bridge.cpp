#include <vulkan/vulkan.h>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "WrapperBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" {
    void init_android_vulkan();
    PFN_vkVoidFunction get_real_proc_addr(VkInstance instance, const char* pName);

    VkResult logic_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
    VkResult logic_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);
    VkResult logic_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
    void logic_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures);
    VkResult logic_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
}

extern "C" {

// 1. ICD Interface Negotiation (Required by Vulkan Loader to query devices properly)
VKAPI_ATTR VkResult VKAPI_CALL vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* pSupportedVersion) {
    if (!pSupportedVersion) return VK_ERROR_INITIALIZATION_FAILED;
    if (*pSupportedVersion > 5) {
        *pSupportedVersion = 5;
    }
    return VK_SUCCESS;
}

// 2. Main Function Interceptor
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    init_android_vulkan();

    if (!pName) return nullptr;

    if (strcmp(pName, "vkCreateShaderModule") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateShaderModule);
    if (strcmp(pName, "vkCreateInstance") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateInstance);
    if (strcmp(pName, "vkEnumerateDeviceExtensionProperties") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkEnumerateDeviceExtensionProperties);
    if (strcmp(pName, "vkGetPhysicalDeviceFeatures2") == 0 || strcmp(pName, "vkGetPhysicalDeviceFeatures2KHR") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkGetPhysicalDeviceFeatures2);
    if (strcmp(pName, "vkCreateDevice") == 0) 
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateDevice);

    return get_real_proc_addr(instance, pName);
}

// 3. ICD Entrypoint Alias
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return vkGetInstanceProcAddr(instance, pName);
}

} // extern "C"

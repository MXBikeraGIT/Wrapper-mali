#include "bridge.h"
#include <cstring>

// External driver passthrough from output.cpp
extern void init_android_vulkan();
extern PFN_vkVoidFunction get_real_proc_addr(VkInstance instance, const char* pName);

// External feature logic & intercepted calls implemented in logic.cpp
extern VkResult logic_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule);

extern VkResult logic_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance);

extern "C" {

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    init_android_vulkan();

    if (!pName) return nullptr;

    // Route modified Vulkan calls directly to logic.cpp
    if (strcmp(pName, "vkCreateShaderModule") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateShaderModule);
    }
    if (strcmp(pName, "vkCreateInstance") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(logic_vkCreateInstance);
    }

    // Direct passthrough to the target hardware driver
    return get_real_proc_addr(instance, pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return vkGetInstanceProcAddr(instance, pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    return vkGetInstanceProcAddr(nullptr, pName);
}

} // extern "C"

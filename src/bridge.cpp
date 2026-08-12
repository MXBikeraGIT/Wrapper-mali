#include "dispatch.h"
#include <cstring>
#include <android/log.h>

#define LOG_TAG "WrapperBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" PFN_vkGetInstanceProcAddr get_global_vkGetInstanceProcAddr();

// Declare logic hooks defined in logic.cpp
extern "C" {
    VkResult logic_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
    void logic_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator);
    VkResult logic_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
    void logic_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator);
    VkResult logic_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);
}

#define CHECK_HOOK(func_name, logic_handler) \
    if (strcmp(pName, func_name) == 0) return reinterpret_cast<PFN_vkVoidFunction>(logic_handler);

extern "C" {

VKAPI_ATTR VkResult VKAPI_CALL vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* pSupportedVersion) {
    if (!pSupportedVersion) return VK_ERROR_INITIALIZATION_FAILED;
    if (*pSupportedVersion > 5) *pSupportedVersion = 5;
    return VK_SUCCESS;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    if (!pName) return nullptr;

    // 1. Check Hook List
    CHECK_HOOK("vkCreateInstance", logic_vkCreateInstance);
    CHECK_HOOK("vkDestroyInstance", logic_vkDestroyInstance);
    CHECK_HOOK("vkCreateDevice", logic_vkCreateDevice);
    CHECK_HOOK("vkDestroyDevice", logic_vkDestroyDevice);
    CHECK_HOOK("vkCreateShaderModule", logic_vkCreateShaderModule);

    // 2. Fallback to Instance Dispatch Table
    if (instance != VK_NULL_HANDLE) {
        InstanceDispatchTable* dt = get_instance_dispatch(instance);
        if (dt && dt->vkGetInstanceProcAddr) {
            return dt->vkGetInstanceProcAddr(dt->real_instance, pName);
        }
    }

    // 3. Fallback to Global Driver Entry Point
    PFN_vkGetInstanceProcAddr global_gpa = get_global_vkGetInstanceProcAddr();
    if (global_gpa) {
        return global_gpa(instance, pName);
    }

    return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    if (!pName) return nullptr;

    // 1. Check Device-Level Hook List
    CHECK_HOOK("vkCreateShaderModule", logic_vkCreateShaderModule);
    CHECK_HOOK("vkDestroyDevice", logic_vkDestroyDevice);

    // 2. Fallback to Device Dispatch Table
    if (device != VK_NULL_HANDLE) {
        DeviceDispatchTable* dt = get_device_dispatch(device);
        if (dt && dt->vkGetDeviceProcAddr) {
            return dt->vkGetDeviceProcAddr(dt->real_device, pName);
        }
    }

    return vkGetInstanceProcAddr(VK_NULL_HANDLE, pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return vkGetInstanceProcAddr(instance, pName);
}

} // extern "C"

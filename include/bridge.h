#ifndef BRIDGE_H
#define BRIDGE_H

#include <vulkan/vulkan.h>
#include <mutex>

// Thread-safe mutex shared across wrapper components
extern std::mutex g_wrapper_log_mutex;

#ifdef __cplusplus
extern "C" {
#endif

// SPIR-V transformation handler implemented in logic.cpp
VkResult logic_process_spirv(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

// Primary Vulkan Hook Entry Points
VKAPI_ATTR VkResult VKAPI_CALL wrapper_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL wrapper_vkGetDeviceProcAddr(
    VkDevice device,
    const char* pName
);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL wrapper_vkGetInstanceProcAddr(
    VkInstance instance,
    const char* pName
);

// Real hardware driver symbol procurement
PFN_vkVoidFunction get_real_instance_proc(VkInstance instance, const char* name);
PFN_vkVoidFunction get_real_device_proc(VkDevice device, const char* name);
bool init_real_driver();

#ifdef __cplusplus
}
#endif

// C++ Template Wrappers for Dynamic Symbol Casting
#ifdef __cplusplus
template<typename T>
inline T get_real_device_proc(VkDevice device, const char* name) {
    return reinterpret_cast<T>(get_real_device_proc(device, name));
}

template<typename T>
inline T get_real_instance_proc(VkInstance instance, const char* name) {
    return reinterpret_cast<T>(get_real_instance_proc(instance, name));
}
#endif

#endif // BRIDGE_H

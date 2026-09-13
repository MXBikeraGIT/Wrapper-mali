#ifndef BRIDGE_H
#define BRIDGE_H

#include <vulkan/vulkan.h>
#include <mutex>

// Global mutex shared across wrapper modules for safe thread logging
extern std::mutex g_wrapper_log_mutex;

#ifdef __cplusplus
extern "C" {
#endif

// Logic processor entry point implemented in logic.cpp
VkResult logic_process_spirv(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

// Bridge Hook Entry Point
VKAPI_ATTR VkResult VKAPI_CALL wrapper_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

#ifdef __cplusplus
}
#endif

// --- Real Driver Proc Procurement ---
bool init_real_driver();
PFN_vkVoidFunction get_real_instance_proc(VkInstance instance, const char* name);
PFN_vkVoidFunction get_real_device_proc(VkDevice device, const char* name);

template<typename T>
T get_real_device_proc(VkDevice device, const char* name) {
    return reinterpret_cast<T>(get_real_device_proc(device, name));
}

template<typename T>
T get_real_instance_proc(VkInstance instance, const char* name) {
    return reinterpret_cast<T>(get_real_instance_proc(instance, name));
}

#endif // BRIDGE_H

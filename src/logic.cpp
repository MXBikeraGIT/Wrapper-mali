#include "dispatch.h"
#include <vector>
#include <android/log.h>

#define LOG_TAG "WrapperLogic"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" PFN_vkGetInstanceProcAddr get_global_vkGetInstanceProcAddr();

extern "C" VkResult logic_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance)
{
    PFN_vkGetInstanceProcAddr global_gpa = get_global_vkGetInstanceProcAddr();
    if (!global_gpa) return VK_ERROR_INITIALIZATION_FAILED;

    auto real_vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
        global_gpa(VK_NULL_HANDLE, "vkCreateInstance")
    );

    VkInstance real_inst = VK_NULL_HANDLE;
    VkResult res = real_vkCreateInstance(pCreateInfo, pAllocator, &real_inst);

    if (res == VK_SUCCESS && real_inst != VK_NULL_HANDLE) {
        *pInstance = real_inst; // Register real handle into dispatch table
        register_instance(real_inst, real_inst, global_gpa);
        LOGI("Created and registered VkInstance: %p", real_inst);
    }

    return res;
}

extern "C" void logic_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
    InstanceDispatchTable* dt = get_instance_dispatch(instance);
    if (dt && dt->vkDestroyInstance) {
        dt->vkDestroyInstance(dt->real_instance, pAllocator);
        unregister_instance(instance);
        LOGI("Destroyed VkInstance: %p", instance);
    }
}

extern "C" VkResult logic_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    // Retrieve parent instance dispatch table
    InstanceDispatchTable* dt = get_instance_dispatch(nullptr); 
    PFN_vkCreateDevice real_fn = dt ? dt->vkCreateDevice : nullptr;

    if (!real_fn) {
        PFN_vkGetInstanceProcAddr global_gpa = get_global_vkGetInstanceProcAddr();
        real_fn = reinterpret_cast<PFN_vkCreateDevice>(global_gpa(VK_NULL_HANDLE, "vkCreateDevice"));
    }

    VkDevice real_dev = VK_NULL_HANDLE;
    VkResult res = real_fn(physicalDevice, pCreateInfo, pAllocator, &real_dev);

    if (res == VK_SUCCESS && real_dev != VK_NULL_HANDLE) {
        *pDevice = real_dev;

        PFN_vkGetInstanceProcAddr global_gpa = get_global_vkGetInstanceProcAddr();
        auto real_gda = reinterpret_cast<PFN_vkGetDeviceProcAddr>(global_gpa(VK_NULL_HANDLE, "vkGetDeviceProcAddr"));

        register_device(real_dev, real_dev, real_gda);
        LOGI("Created and registered VkDevice: %p", real_dev);
    }

    return res;
}

extern "C" void logic_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable* dt = get_device_dispatch(device);
    if (dt && dt->vkDestroyDevice) {
        dt->vkDestroyDevice(dt->real_device, pAllocator);
        unregister_device(device);
        LOGI("Destroyed VkDevice: %p", device);
    }
}

// SPIR-V Shader Module Interceptor Hook
extern "C" VkResult logic_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule)
{
    DeviceDispatchTable* dt = get_device_dispatch(device);
    if (!dt || !dt->vkCreateShaderModule) {
        LOGE("Failed to find dispatch table for VkDevice: %p", device);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (!pCreateInfo || !pCreateInfo->pCode || pCreateInfo->codeSize == 0) {
        return dt->vkCreateShaderModule(dt->real_device, pCreateInfo, pAllocator, pShaderModule);
    }

    // 1. Copy SPIR-V bytecode stream into mutable buffer
    size_t word_count = pCreateInfo->codeSize / sizeof(uint32_t);
    std::vector<uint32_t> spirv_words(pCreateInfo->pCode, pCreateInfo->pCode + word_count);

    // 2. Perform Mali SPIR-V Patching Pass
    bool patched = false;
    for (size_t i = 5; i < spirv_words.size(); ) {
        uint32_t inst = spirv_words[i];
        uint16_t opcode = inst & 0xFFFF;
        uint16_t length = (inst >> 16) & 0xFFFF;

        if (length == 0 || (i + length) > spirv_words.size()) break;

        // Strip OpSpecConstantComposite (51) & OpSpecConstantOp (52)
        if (opcode == 51 || opcode == 52) {
            for (size_t k = 0; k < length; ++k) {
                spirv_words[i + k] = (1 << 16) | 0; // Replace with OpNop
            }
            patched = true;
        }
        i += length;
    }

    if (patched) {
        LOGI("Patched SPIR-V shader module for Mali compatibility.");
    }

    // 3. Forward patched SPIR-V buffer to real driver
    VkShaderModuleCreateInfo modified_info = *pCreateInfo;
    modified_info.pCode = spirv_words.data();
    modified_info.codeSize = spirv_words.size() * sizeof(uint32_t);

    return dt->vkCreateShaderModule(dt->real_device, &modified_info, pAllocator, pShaderModule);
}

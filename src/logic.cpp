#include "dispatch.h"
#include <vector>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "WrapperLogic"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" PFN_vkGetInstanceProcAddr get_global_vkGetInstanceProcAddr();

// Hook 1: Enumerate Instance Extensions
extern "C" VkResult logic_vkEnumerateInstanceExtensionProperties(
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties)
{
    PFN_vkGetInstanceProcAddr global_gpa = get_global_vkGetInstanceProcAddr();
    if (!global_gpa) return VK_ERROR_INITIALIZATION_FAILED;

    auto real_fn = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
        global_gpa(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties")
    );
    if (!real_fn) return VK_ERROR_INITIALIZATION_FAILED;

    return real_fn(pLayerName, pPropertyCount, pProperties);
}

// Hook 2: Enumerate Instance Layers
extern "C" VkResult logic_vkEnumerateInstanceLayerProperties(
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties)
{
    PFN_vkGetInstanceProcAddr global_gpa = get_global_vkGetInstanceProcAddr();
    if (!global_gpa) return VK_ERROR_INITIALIZATION_FAILED;

    auto real_fn = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
        global_gpa(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties")
    );
    if (!real_fn) return VK_ERROR_INITIALIZATION_FAILED;

    return real_fn(pPropertyCount, pProperties);
}

// Hook 3: Instance Creation with Extension Sanitization
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
    auto real_vkEnumerateExt = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
        global_gpa(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties")
    );

    // 1. Fetch real host driver supported extensions
    uint32_t host_ext_count = 0;
    if (real_vkEnumerateExt) {
        real_vkEnumerateExt(nullptr, &host_ext_count, nullptr);
    }
    std::vector<VkExtensionProperties> host_exts(host_ext_count);
    if (host_ext_count > 0) {
        real_vkEnumerateExt(nullptr, &host_ext_count, host_exts.data());
    }

    // 2. Filter requested extensions to keep only host-supported ones
    std::vector<const char*> filtered_extensions;
    if (pCreateInfo && pCreateInfo->enabledExtensionCount > 0) {
        for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
            const char* req_ext = pCreateInfo->ppEnabledExtensionNames[i];
            bool supported = false;

            for (const auto& host_ext : host_exts) {
                if (strcmp(req_ext, host_ext.extensionName) == 0) {
                    supported = true;
                    break;
                }
            }

            if (supported) {
                filtered_extensions.push_back(req_ext);
            } else {
                LOGW("Stripping unsupported instance extension: %s", req_ext);
            }
        }
    }

    // 3. Construct sanitized create info payload
    VkInstanceCreateInfo modified_info = *pCreateInfo;
    modified_info.enabledExtensionCount = static_cast<uint32_t>(filtered_extensions.size());
    modified_info.ppEnabledExtensionNames = filtered_extensions.data();

    VkInstance real_inst = VK_NULL_HANDLE;
    VkResult res = real_vkCreateInstance(&modified_info, pAllocator, &real_inst);

    if (res == VK_SUCCESS && real_inst != VK_NULL_HANDLE) {
        *pInstance = real_inst;
        register_instance(real_inst, real_inst, global_gpa);
        LOGI("Successfully created and registered VkInstance: %p", real_inst);
    } else {
        LOGE("real_vkCreateInstance failed with VkResult: %d", res);
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

// Hook 4: Device Creation with Extension Sanitization
extern "C" VkResult logic_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    PFN_vkGetInstanceProcAddr global_gpa = get_global_vkGetInstanceProcAddr();
    InstanceDispatchTable* dt = get_instance_dispatch(nullptr);

    auto real_vkEnumerateDevExt = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        global_gpa(VK_NULL_HANDLE, "vkEnumerateDeviceExtensionProperties")
    );

    // Fetch device-supported extensions
    uint32_t host_ext_count = 0;
    if (real_vkEnumerateDevExt) {
        real_vkEnumerateDevExt(physicalDevice, nullptr, &host_ext_count, nullptr);
    }
    std::vector<VkExtensionProperties> host_exts(host_ext_count);
    if (host_ext_count > 0) {
        real_vkEnumerateDevExt(physicalDevice, nullptr, &host_ext_count, host_exts.data());
    }

    // Filter requested device extensions
    std::vector<const char*> filtered_extensions;
    if (pCreateInfo && pCreateInfo->enabledExtensionCount > 0) {
        for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
            const char* req_ext = pCreateInfo->ppEnabledExtensionNames[i];
            bool supported = false;

            for (const auto& host_ext : host_exts) {
                if (strcmp(req_ext, host_ext.extensionName) == 0) {
                    supported = true;
                    break;
                }
            }

            if (supported) {
                filtered_extensions.push_back(req_ext);
            } else {
                LOGW("Stripping unsupported device extension: %s", req_ext);
            }
        }
    }

    VkDeviceCreateInfo modified_info = *pCreateInfo;
    modified_info.enabledExtensionCount = static_cast<uint32_t>(filtered_extensions.size());
    modified_info.ppEnabledExtensionNames = filtered_extensions.data();

    PFN_vkCreateDevice real_fn = (dt && dt->vkCreateDevice) ? dt->vkCreateDevice :
        reinterpret_cast<PFN_vkCreateDevice>(global_gpa(VK_NULL_HANDLE, "vkCreateDevice"));

    VkDevice real_dev = VK_NULL_HANDLE;
    VkResult res = real_fn(physicalDevice, &modified_info, pAllocator, &real_dev);

    if (res == VK_SUCCESS && real_dev != VK_NULL_HANDLE) {
        *pDevice = real_dev;
        auto real_gda = reinterpret_cast<PFN_vkGetDeviceProcAddr>(global_gpa(VK_NULL_HANDLE, "vkGetDeviceProcAddr"));
        register_device(real_dev, real_dev, real_gda);
        LOGI("Successfully created and registered VkDevice: %p", real_dev);
    } else {
        LOGE("real_vkCreateDevice failed with VkResult: %d", res);
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

// Hook 5: SPIR-V Interceptor Hook
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

    size_t word_count = pCreateInfo->codeSize / sizeof(uint32_t);
    std::vector<uint32_t> spirv_words(pCreateInfo->pCode, pCreateInfo->pCode + word_count);

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

    VkShaderModuleCreateInfo modified_info = *pCreateInfo;
    modified_info.pCode = spirv_words.data();
    modified_info.codeSize = spirv_words.size() * sizeof(uint32_t);

    return dt->vkCreateShaderModule(dt->real_device, &modified_info, pAllocator, pShaderModule);
}

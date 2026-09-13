#include <vulkan/vulkan.h>
#include <android/log.h>
#include <cstdio>
#include <mutex>
#include "bridge.h"

// Global mutex shared across wrapper modules to prevent log conflicts
std::mutex g_wrapper_log_mutex;

// Function prototype implemented in logic.cpp
extern "C" VkResult logic_process_spirv(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

// Bridge Hook Entry Point
extern "C" VKAPI_ATTR VkResult VKAPI_CALL wrapper_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    if (pCreateInfo && pCreateInfo->pCode && pCreateInfo->codeSize > 0) {
        // Safe, single log entry per shader module (no 3-second thread block)
        std::lock_guard<std::mutex> lock(g_wrapper_log_mutex);
        __android_log_print(ANDROID_LOG_INFO, "Winlator-Wrapper", 
                            "Intercepted vkCreateShaderModule (Size: %zu bytes)", pCreateInfo->codeSize);
        fprintf(stderr, "[Winlator-Wrapper] Intercepted vkCreateShaderModule (Size: %zu bytes)\n", pCreateInfo->codeSize);
        fflush(stderr);
    }

    // Forward execution directly to logic stage (logic.cpp) for spirv-tools transformation
    return logic_process_spirv(device, pCreateInfo, pAllocator, pShaderModule);
}

// Fallback alias for bridge procurement
extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    return wrapper_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

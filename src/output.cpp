#define _GNU_SOURCE
#include <vulkan/vulkan.h>
#include <android/log.h>
#include <cstdio>
#include <mutex>
#include "bridge.h"

// Output Stage Dispatch Handler
extern "C" VkResult output_send_to_driver(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    // Single non-blocking log entry for output dispatch
    {
        std::lock_guard<std::mutex> lock(g_wrapper_log_mutex);
        __android_log_print(ANDROID_LOG_INFO, "Winlator-Output", 
                            "Forwarding shader module to underlying Mali driver...");
        fprintf(stderr, "[Winlator-Output] Forwarding shader module to underlying Mali driver...\n");
        fflush(stderr);
    }

    // Resolve real vkCreateShaderModule function pointer from real driver proc procurement
    static PFN_vkCreateShaderModule real_vkCreateShaderModule = nullptr;
    if (!real_vkCreateShaderModule) {
        real_vkCreateShaderModule = get_real_device_proc<PFN_vkCreateShaderModule>(device, "vkCreateShaderModule");
    }

    if (real_vkCreateShaderModule) {
        return real_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    }

    __android_log_print(ANDROID_LOG_ERROR, "Winlator-Output", "Failed to resolve real vkCreateShaderModule symbol!");
    return VK_ERROR_INITIALIZATION_FAILED;
}

// Driver dispatch bridge export
extern "C" VkResult dispatch_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    return output_send_to_driver(device, pCreateInfo, pAllocator, pShaderModule);
}

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <vulkan/vulkan.h>
#include <android/log.h>
#include <mutex>
#include "output.h"
#include "bridge.h"

extern "C" VkResult output_send_to_driver(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    {
        std::lock_guard<std::mutex> lock(g_wrapper_log_mutex);
        __android_log_print(ANDROID_LOG_INFO, "Winlator-Output", 
                            "Dispatching shader module to native Mali GPU driver");
    }

    static PFN_vkCreateShaderModule real_vkCreateShaderModule = nullptr;
    if (!real_vkCreateShaderModule) {
        real_vkCreateShaderModule = get_real_device_proc<PFN_vkCreateShaderModule>(device, "vkCreateShaderModule");
    }

    if (real_vkCreateShaderModule) {
        return real_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    }

    __android_log_print(ANDROID_LOG_ERROR, "Winlator-Output", 
                        "Fatal: Could not resolve real driver vkCreateShaderModule symbol!");
    return VK_ERROR_INITIALIZATION_FAILED;
}

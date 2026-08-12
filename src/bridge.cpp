#include <vulkan/vulkan.h>
#include <android/log.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>

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
        // Lock mutex to block logic/output until 3-second bridge spam finishes
        {
            std::lock_guard<std::mutex> lock(g_wrapper_log_mutex);
            auto start_time = std::chrono::steady_clock::now();

            while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(3)) {
                __android_log_print(ANDROID_LOG_INFO, "Winlator", "got code");
                fprintf(stderr, "[Winlator] got code\n");
                fflush(stderr);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    }

    // Forward execution to logic stage
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

#define _GNU_SOURCE
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <android/log.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <mutex>

extern std::mutex g_wrapper_log_mutex;

// Output Stage Dispatch Handler
extern "C" VkResult output_send_to_driver(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    // Wait until logic log finishes before spamming output logs
    {
        std::lock_guard<std::mutex> lock(g_wrapper_log_mutex);
        auto start_time = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(3)) {
            __android_log_print(ANDROID_LOG_INFO, "Winlator", "done");
            fprintf(stderr, "[Winlator] done\n");
            fflush(stderr);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // Dynamically fetch target Vulkan driver symbol from libvulkan.so
    typedef VkResult (VKAPI_PTR *PFN_vkCreateShaderModule)(
        VkDevice, const VkShaderModuleCreateInfo*, const VkAllocationCallbacks*, VkShaderModule*
    );

    static PFN_vkCreateShaderModule real_vkCreateShaderModule = nullptr;
    if (!real_vkCreateShaderModule) {
        real_vkCreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(
            dlsym(RTLD_NEXT, "vkCreateShaderModule")
        );
    }

    if (real_vkCreateShaderModule) {
        return real_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    }

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

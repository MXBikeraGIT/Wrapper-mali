#define _GNU_SOURCE
#include <vulkan/vulkan.h>
#include <android/log.h>
#include <dlfcn.h>
#include <cstdio>
#include <mutex>
#include "bridge.h"

// Global mutex shared across wrapper modules to prevent log conflicts
std::mutex g_wrapper_log_mutex;

// Handle to system Vulkan driver library
static void* g_vulkan_lib_handle = nullptr;
static PFN_vkGetInstanceProcAddr g_real_vkGetInstanceProcAddr = nullptr;
static PFN_vkGetDeviceProcAddr g_real_vkGetDeviceProcAddr = nullptr;

// Initialize real system Vulkan driver procurement
extern "C" bool init_real_driver() {
    if (g_vulkan_lib_handle) return true;

    // Load standard Android Vulkan library
    g_vulkan_lib_handle = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_vulkan_lib_handle) {
        __android_log_print(ANDROID_LOG_ERROR, "Winlator-Bridge", "Failed to dlopen libvulkan.so: %s", dlerror());
        return false;
    }

    g_real_vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        dlsym(g_vulkan_lib_handle, "vkGetInstanceProcAddr")
    );

    g_real_vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
        dlsym(g_vulkan_lib_handle, "vkGetDeviceProcAddr")
    );

    return (g_real_vkGetInstanceProcAddr != nullptr);
}

// Global symbol resolver for device functions
extern "C" PFN_vkVoidFunction get_real_device_proc(VkDevice device, const char* name) {
    if (!init_real_driver()) return nullptr;

    if (g_real_vkGetDeviceProcAddr && device) {
        PFN_vkVoidFunction proc = g_real_vkGetDeviceProcAddr(device, name);
        if (proc) return proc;
    }

    // Fallback to dlsym lookup directly on real driver library
    return reinterpret_cast<PFN_vkVoidFunction>(dlsym(g_vulkan_lib_handle, name));
}

// Global symbol resolver for instance functions
extern "C" PFN_vkVoidFunction get_real_instance_proc(VkInstance instance, const char* name) {
    if (!init_real_driver()) return nullptr;

    if (g_real_vkGetInstanceProcAddr) {
        PFN_vkVoidFunction proc = g_real_vkGetInstanceProcAddr(instance, name);
        if (proc) return proc;
    }

    return reinterpret_cast<PFN_vkVoidFunction>(dlsym(g_vulkan_lib_handle, name));
}

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
        std::lock_guard<std::mutex> lock(g_wrapper_log_mutex);
        __android_log_print(ANDROID_LOG_INFO, "Winlator-Wrapper", 
                            "Intercepted vkCreateShaderModule (Size: %zu bytes)", pCreateInfo->codeSize);
        fprintf(stderr, "[Winlator-Wrapper] Intercepted vkCreateShaderModule (Size: %zu bytes)\n", pCreateInfo->codeSize);
        fflush(stderr);
    }

    // Forward execution directly to logic stage for spirv-tools transformation
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

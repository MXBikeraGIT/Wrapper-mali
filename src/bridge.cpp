#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <vulkan/vulkan.h>
#include <android/log.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <mutex>
#include "bridge.h"

std::mutex g_wrapper_log_mutex;

static void* g_vulkan_lib_handle = nullptr;
static PFN_vkGetInstanceProcAddr g_real_vkGetInstanceProcAddr = nullptr;
static PFN_vkGetDeviceProcAddr g_real_vkGetDeviceProcAddr = nullptr;

extern "C" bool init_real_driver() {
    if (g_vulkan_lib_handle) return true;

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

extern "C" PFN_vkVoidFunction get_real_device_proc(VkDevice device, const char* name) {
    if (!init_real_driver()) return nullptr;

    if (g_real_vkGetDeviceProcAddr && device) {
        PFN_vkVoidFunction proc = g_real_vkGetDeviceProcAddr(device, name);
        if (proc) return proc;
    }

    return reinterpret_cast<PFN_vkVoidFunction>(dlsym(g_vulkan_lib_handle, name));
}

extern "C" PFN_vkVoidFunction get_real_instance_proc(VkInstance instance, const char* name) {
    if (!init_real_driver()) return nullptr;

    if (g_real_vkGetInstanceProcAddr) {
        PFN_vkVoidFunction proc = g_real_vkGetInstanceProcAddr(instance, name);
        if (proc) return proc;
    }

    return reinterpret_cast<PFN_vkVoidFunction>(dlsym(g_vulkan_lib_handle, name));
}

// Global Interception Hooks for GetProcAddr
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL wrapper_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    if (pName && strcmp(pName, "vkCreateShaderModule") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(wrapper_vkCreateShaderModule);
    }
    return get_real_device_proc(device, pName);
}

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL wrapper_vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    if (pName && strcmp(pName, "vkCreateShaderModule") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(wrapper_vkCreateShaderModule);
    }
    if (pName && strcmp(pName, "vkGetDeviceProcAddr") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(wrapper_vkGetDeviceProcAddr);
    }
    return get_real_instance_proc(instance, pName);
}

// Shader Hook Bridge Entry Point
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
    }
    return logic_process_spirv(device, pCreateInfo, pAllocator, pShaderModule);
}

// Export standard C symbols for dynamic lookup
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    return wrapper_vkGetDeviceProcAddr(device, pName);
}

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return wrapper_vkGetInstanceProcAddr(instance, pName);
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    return wrapper_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

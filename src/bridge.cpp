#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <iostream>
#include <unordered_map>
#include <mutex>

#include "bridge.h"
#include "output.h"

typedef PFN_vkVoidFunction (VKAPI_PTR *PFN_vkGetInstanceProcAddr)(VkInstance instance, const char* pName);
typedef PFN_vkVoidFunction (VKAPI_PTR *PFN_vkGetDeviceProcAddr)(VkDevice device, const char* pName);

static void* g_real_vulkan_handle = nullptr;
static PFN_vkGetInstanceProcAddr g_real_vkGetInstanceProcAddr = nullptr;

static std::mutex g_mutex;

extern "C" VKAPI_ATTR VkResult VKAPI_CALL wrapper_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule);

bool init_real_driver() {
    if (g_real_vulkan_handle) return true;

    g_real_vulkan_handle = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!g_real_vulkan_handle) {
        g_real_vulkan_handle = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    }
    if (!g_real_vulkan_handle) {
        log_error("Failed to load real libvulkan.so!");
        return false;
    }

    g_real_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(g_real_vulkan_handle, "vkGetInstanceProcAddr");
    return g_real_vkGetInstanceProcAddr != nullptr;
}

PFN_vkVoidFunction get_real_instance_proc(VkInstance instance, const char* name) {
    if (!init_real_driver()) return nullptr;
    return g_real_vkGetInstanceProcAddr(instance, name);
}

PFN_vkVoidFunction get_real_device_proc(VkDevice device, const char* name) {
    if (!init_real_driver()) return nullptr;
    return g_real_vkGetInstanceProcAddr(NULL, name);
}

// Exported loader entrypoint
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance instance, 
    const char* pName) 
{
    if (!init_real_driver()) return nullptr;

    std::string name(pName);

    if (name == "vkGetInstanceProcAddr") return (PFN_vkVoidFunction)vkGetInstanceProcAddr;
    if (name == "vkCreateShaderModule") return (PFN_vkVoidFunction)wrapper_vkCreateShaderModule;

    return g_real_vkGetInstanceProcAddr(instance, pName);
}

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
    VkDevice device, 
    const char* pName) 
{
    std::string name(pName);

    if (name == "vkGetDeviceProcAddr") return (PFN_vkVoidFunction)vkGetDeviceProcAddr;
    if (name == "vkCreateShaderModule") return (PFN_vkVoidFunction)wrapper_vkCreateShaderModule;

    PFN_vkGetDeviceProcAddr real_gdpa = get_real_instance_proc<PFN_vkGetDeviceProcAddr>(NULL, "vkGetDeviceProcAddr");
    if (real_gdpa) {
        return real_gdpa(device, pName);
    }

    return nullptr;
}

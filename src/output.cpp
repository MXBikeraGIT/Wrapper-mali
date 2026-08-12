#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "AndroidOutput"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static void* g_real_libvulkan = nullptr;
static PFN_vkGetInstanceProcAddr g_real_vkGetInstanceProcAddr = nullptr;
static PFN_vkCreateShaderModule g_real_vkCreateShaderModule = nullptr;
static PFN_vkCreateInstance g_real_vkCreateInstance = nullptr;

void init_android_vulkan() {
    if (g_real_libvulkan) return;

    // Load native Android Bionic Vulkan driver directly from system
    g_real_libvulkan = dlopen("/system/lib64/libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_real_libvulkan) {
        g_real_libvulkan = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    }

    if (!g_real_libvulkan) {
        LOGE("FATAL: Could not load Android system libvulkan.so!");
        return;
    }

    g_real_vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        dlsym(g_real_libvulkan, "vkGetInstanceProcAddr")
    );

    LOGI("Successfully loaded system libvulkan.so dispatch table!");
}

PFN_vkVoidFunction get_real_proc_addr(VkInstance instance, const char* pName) {
    if (!g_real_vkGetInstanceProcAddr) {
        init_android_vulkan();
    }
    return g_real_vkGetInstanceProcAddr(instance, pName);
}

VkResult call_real_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule) 
{
    if (!g_real_vkCreateShaderModule) {
        g_real_vkCreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(
            g_real_vkGetInstanceProcAddr(nullptr, "vkCreateShaderModule")
        );
    }
    return g_real_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

VkResult call_real_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) 
{
    if (!g_real_vkCreateInstance) {
        g_real_vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
            g_real_vkGetInstanceProcAddr(nullptr, "vkCreateInstance")
        );
    }
    return g_real_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

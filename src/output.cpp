#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "WrapperOutput"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static void* g_real_vulkan_handle = nullptr;
static PFN_vkGetInstanceProcAddr g_real_vkGetInstanceProcAddr = nullptr;

extern "C" void init_android_vulkan() {
    if (g_real_vulkan_handle) return;

    // Scan standard Android system and vendor driver paths
    const char* candidate_paths[] = {
        "/system/lib64/libvulkan.so",
        "/vendor/lib64/hw/vulkan.mali.so",
        "/vendor/lib64/egl/vulkan.mali.so",
        "/system/lib/libvulkan.so",
        "libvulkan.so.1",
        nullptr
    };

    for (int i = 0; candidate_paths[i] != nullptr; ++i) {
        g_real_vulkan_handle = dlopen(candidate_paths[i], RTLD_NOW | RTLD_LOCAL);
        if (g_real_vulkan_handle) {
            LOGI("Successfully loaded real driver: %s", candidate_paths[i]);
            break;
        }
    }

    if (!g_real_vulkan_handle) {
        LOGE("Failed to open real Vulkan driver handle!");
        return;
    }

    g_real_vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        dlsym(g_real_vulkan_handle, "vkGetInstanceProcAddr")
    );
}

extern "C" PFN_vkVoidFunction get_real_proc_addr(VkInstance instance, const char* pName) {
    if (!g_real_vulkan_handle) {
        init_android_vulkan();
    }

    if (g_real_vkGetInstanceProcAddr) {
        PFN_vkVoidFunction proc = g_real_vkGetInstanceProcAddr(instance, pName);
        if (proc) return proc;
    }

    if (g_real_vulkan_handle) {
        return reinterpret_cast<PFN_vkVoidFunction>(dlsym(g_real_vulkan_handle, pName));
    }

    return nullptr;
}

extern "C" {

VkResult call_real_vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties)
{
    auto real_fn = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        get_real_proc_addr(nullptr, "vkEnumerateDeviceExtensionProperties"));
    if (!real_fn) return VK_ERROR_INITIALIZATION_FAILED;
    return real_fn(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

void call_real_vkGetPhysicalDeviceFeatures2(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures2* pFeatures)
{
    auto real_fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        get_real_proc_addr(nullptr, "vkGetPhysicalDeviceFeatures2"));
    if (!real_fn) {
        real_fn = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
            get_real_proc_addr(nullptr, "vkGetPhysicalDeviceFeatures2KHR"));
    }
    if (real_fn) real_fn(physicalDevice, pFeatures);
}

VkResult call_real_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    auto real_fn = reinterpret_cast<PFN_vkCreateDevice>(
        get_real_proc_addr(nullptr, "vkCreateDevice"));
    if (!real_fn) return VK_ERROR_INITIALIZATION_FAILED;
    return real_fn(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

VkResult call_real_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule)
{
    auto real_fn = reinterpret_cast<PFN_vkCreateShaderModule>(
        get_real_proc_addr(nullptr, "vkCreateShaderModule"));
    if (!real_fn) return VK_ERROR_INITIALIZATION_FAILED;
    return real_fn(device, pCreateInfo, pAllocator, pShaderModule);
}

VkResult call_real_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance)
{
    auto real_fn = reinterpret_cast<PFN_vkCreateInstance>(
        get_real_proc_addr(nullptr, "vkCreateInstance"));
    if (!real_fn) return VK_ERROR_INITIALIZATION_FAILED;
    return real_fn(pCreateInfo, pAllocator, pInstance);
}

} // extern "C"

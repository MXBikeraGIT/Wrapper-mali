#include "dispatch.h"
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "WrapperOutput"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static void* g_real_vulkan_handle = nullptr;
static PFN_vkGetInstanceProcAddr g_real_vkGetInstanceProcAddr = nullptr;

static std::unordered_map<VkInstance, InstanceDispatchTable> g_instance_map;
static std::mutex g_instance_mutex;

static std::unordered_map<VkDevice, DeviceDispatchTable> g_device_map;
static std::mutex g_device_mutex;

extern "C" void init_android_vulkan() {
    if (g_real_vulkan_handle && g_real_vkGetInstanceProcAddr) return;

    LOGI("Loading system Vulkan library...");
    const char* paths[] = {
        "/system/lib64/libvulkan.so",
        "/vendor/lib64/hw/vulkan.mali.so",
        "/apex/com.android.runtime/lib64/bionic/libvulkan.so",
        "libvulkan.so.1",
        "libvulkan.so",
        nullptr
    };

    for (int i = 0; paths[i] != nullptr; ++i) {
        g_real_vulkan_handle = dlopen(paths[i], RTLD_NOW | RTLD_GLOBAL);
        if (g_real_vulkan_handle) {
            LOGI("Loaded driver at %s", paths[i]);
            break;
        }
    }

    if (g_real_vulkan_handle) {
        g_real_vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            dlsym(g_real_vulkan_handle, "vkGetInstanceProcAddr")
        );
    }
}

extern "C" PFN_vkGetInstanceProcAddr get_global_vkGetInstanceProcAddr() {
    init_android_vulkan();
    return g_real_vkGetInstanceProcAddr;
}

void register_instance(VkInstance wrapper_inst, VkInstance real_inst, PFN_vkGetInstanceProcAddr gpa) {
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    InstanceDispatchTable table{};
    table.real_instance = real_inst;
    table.vkGetInstanceProcAddr = gpa;

    // Resolve instance-level function pointers
    table.vkDestroyInstance = (PFN_vkDestroyInstance)gpa(real_inst, "vkDestroyInstance");
    table.vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)gpa(real_inst, "vkEnumeratePhysicalDevices");
    table.vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)gpa(real_inst, "vkGetPhysicalDeviceProperties");
    table.vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)gpa(real_inst, "vkGetPhysicalDeviceProperties2");
    table.vkCreateDevice = (PFN_vkCreateDevice)gpa(real_inst, "vkCreateDevice");

    g_instance_map[wrapper_inst] = table;
}

void unregister_instance(VkInstance wrapper_inst) {
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    g_instance_map.erase(wrapper_inst);
}

InstanceDispatchTable* get_instance_dispatch(VkInstance inst) {
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    auto it = g_instance_map.find(inst);
    return (it != g_instance_map.end()) ? &it->second : nullptr;
}

void register_device(VkDevice wrapper_dev, VkDevice real_dev, PFN_vkGetDeviceProcAddr gda) {
    std::lock_guard<std::mutex> lock(g_device_mutex);
    DeviceDispatchTable table{};
    table.real_device = real_dev;
    table.vkGetDeviceProcAddr = gda;

    table.vkDestroyDevice = (PFN_vkDestroyDevice)gda(real_dev, "vkDestroyDevice");
    table.vkCreateShaderModule = (PFN_vkCreateShaderModule)gda(real_dev, "vkCreateShaderModule");

    g_device_map[wrapper_dev] = table;
}

void unregister_device(VkDevice wrapper_dev) {
    std::lock_guard<std::mutex> lock(g_device_mutex);
    g_device_map.erase(wrapper_dev);
}

DeviceDispatchTable* get_device_dispatch(VkDevice dev) {
    std::lock_guard<std::mutex> lock(g_device_mutex);
    auto it = g_device_map.find(dev);
    return (it != g_device_map.end()) ? &it->second : nullptr;
}

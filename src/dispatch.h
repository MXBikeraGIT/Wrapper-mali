#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <mutex>

// Function pointers for real instance-level calls
struct InstanceDispatchTable {
    VkInstance real_instance = VK_NULL_HANDLE;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    PFN_vkDestroyInstance vkDestroyInstance = nullptr;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = nullptr;
    PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = nullptr;
    PFN_vkCreateDevice vkCreateDevice = nullptr;
};

// Function pointers for real device-level calls
struct DeviceDispatchTable {
    VkDevice real_device = VK_NULL_HANDLE;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
    PFN_vkDestroyDevice vkDestroyDevice = nullptr;
    PFN_vkCreateShaderModule vkCreateShaderModule = nullptr;
};

// Global thread-safe registry declarations
void register_instance(VkInstance wrapper_inst, VkInstance real_inst, PFN_vkGetInstanceProcAddr gpa);
void unregister_instance(VkInstance wrapper_inst);
InstanceDispatchTable* get_instance_dispatch(VkInstance inst);

void register_device(VkDevice wrapper_dev, VkDevice real_dev, PFN_vkGetDeviceProcAddr gda);
void unregister_device(VkDevice wrapper_dev);
DeviceDispatchTable* get_device_dispatch(VkDevice dev);

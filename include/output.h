#pragma once

#include <vulkan/vulkan.h>

// Direct dispatch calls to Android system GPU driver (/system/lib64/libvulkan.so)
void init_android_vulkan();
PFN_vkVoidFunction get_real_proc_addr(VkInstance instance, const char* pName);

VkResult call_real_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule);

VkResult call_real_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance);

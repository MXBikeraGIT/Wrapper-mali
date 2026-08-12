#ifndef BRIDGE_H
#define BRIDGE_H

#include <vulkan/vulkan.h>

bool init_real_driver();
PFN_vkVoidFunction get_real_instance_proc(VkInstance instance, const char* name);
PFN_vkVoidFunction get_real_device_proc(VkDevice device, const char* name);

template<typename T>
T get_real_device_proc(VkDevice device, const char* name) {
    return reinterpret_cast<T>(get_real_device_proc(device, name));
}

template<typename T>
T get_real_instance_proc(VkInstance instance, const char* name) {
    return reinterpret_cast<T>(get_real_instance_proc(instance, name));
}

#endif

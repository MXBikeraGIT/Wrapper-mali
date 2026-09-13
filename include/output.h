#ifndef OUTPUT_H
#define OUTPUT_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forwards transformed payload down to underlying Mali driver
VkResult output_send_to_driver(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

#ifdef __cplusplus
}
#endif

#endif // OUTPUT_H

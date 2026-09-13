#ifndef LOGIC_H
#define LOGIC_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sanitizes SPIR-V bytecode and hands it off to output stage
VkResult logic_process_spirv(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

#ifdef __cplusplus
}
#endif

#endif // LOGIC_H

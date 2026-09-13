#ifndef OUTPUT_H
#define OUTPUT_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Output Stage Dispatch Handler (forwarding transformed SPIR-V to real driver)
VkResult output_send_to_driver(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

// Driver dispatch bridge export alias
VkResult dispatch_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

#ifdef __cplusplus
}
#endif

// --- Debugging & File I/O Utilities ---
void log_info(const std::string& message);
void log_error(const std::string& message);
void dump_spirv_file(const std::string& filename, const std::vector<uint32_t>& code);

#endif // OUTPUT_H

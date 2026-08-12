#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstring>

// Standard SPIR-V Header Magic & Opcodes
constexpr uint32_t SPIRV_MAGIC_NUMBER = 0x07230203;

constexpr uint16_t OP_NOP        = 0;
constexpr uint16_t OP_CAPABILITY = 17;

// Capabilities problematic on Mali GPUs
constexpr uint32_t SPV_CAP_FLOAT64 = 12;
constexpr uint32_t SPV_CAP_INT64   = 11;

// Core SPIR-V Rewriter Function
std::vector<uint32_t> rewrite_spirv_with_mesa(
    const uint32_t* input_spirv,
    size_t word_count,
    VkShaderStageFlagBits stage
) {
    if (!input_spirv || word_count < 5) {
        return {};
    }

    if (input_spirv[0] != SPIRV_MAGIC_NUMBER) {
        return std::vector<uint32_t>(input_spirv, input_spirv + word_count);
    }

    std::vector<uint32_t> spirv(input_spirv, input_spirv + word_count);
    size_t idx = 5; // Skip standard 5-word header

    while (idx < spirv.size()) {
        uint32_t instruction = spirv[idx];
        uint16_t opcode = static_cast<uint16_t>(instruction & 0xFFFF);
        uint16_t inst_word_count = static_cast<uint16_t>(instruction >> 16);

        if (inst_word_count == 0 || (idx + inst_word_count) > spirv.size()) {
            break;
        }

        switch (opcode) {
            case OP_CAPABILITY: {
                if (inst_word_count >= 2) {
                    uint32_t capability = spirv[idx + 1];
                    // Strip unsupported Float64 / Int64 capabilities for Mali drivers
                    if (capability == SPV_CAP_FLOAT64 || capability == SPV_CAP_INT64) {
                        for (size_t i = 0; i < inst_word_count; ++i) {
                            spirv[idx + i] = (1 << 16) | OP_NOP;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }

        idx += inst_word_count;
    }

    return spirv;
}

// Driver dispatch interface (implemented in bridge.cpp / output.cpp)
extern "C" VkResult dispatch_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

// Wrapper Hook Function referenced by bridge.cpp
extern "C" VKAPI_ATTR VkResult VKAPI_CALL wrapper_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    if (!pCreateInfo || !pCreateInfo->pCode || pCreateInfo->codeSize == 0) {
        return dispatch_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    }

    // 1. Convert byte size to word count
    size_t word_count = pCreateInfo->codeSize / sizeof(uint32_t);

    // 2. Perform Mali SPIR-V rewriting
    std::vector<uint32_t> rewritten_spirv = rewrite_spirv_with_mesa(
        pCreateInfo->pCode,
        word_count,
        VK_SHADER_STAGE_ALL
    );

    // 3. Patch creation info struct if rewriting produced modified bytecode
    VkShaderModuleCreateInfo modified_info = *pCreateInfo;
    if (!rewritten_spirv.empty()) {
        modified_info.pCode = rewritten_spirv.data();
        modified_info.codeSize = rewritten_spirv.size() * sizeof(uint32_t);
    }

    // 4. Pass down modified SPIR-V to driver dispatch
    return dispatch_vkCreateShaderModule(device, &modified_info, pAllocator, pShaderModule);
}

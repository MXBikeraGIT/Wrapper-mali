#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <cstddef>

// Standard SPIR-V Constants
constexpr uint32_t SPIRV_MAGIC_NUMBER = 0x07230203;

// Opcodes
constexpr uint16_t OP_NOP              = 0;
constexpr uint16_t OP_CAPABILITY       = 17;
constexpr uint16_t OP_DECORATE         = 71;
constexpr uint16_t OP_MEMBER_DECORATE  = 72;

// Capabilities problematic on Mali GPUs
constexpr uint32_t SPV_CAP_FLOAT64     = 12;
constexpr uint32_t SPV_CAP_INT64       = 11;
constexpr uint32_t SPV_CAP_INT16       = 22;

// Decorations
constexpr uint32_t SPV_DEC_RELAXED_PRECISION = 0;

std::vector<uint32_t> rewrite_spirv_with_mesa(
    const uint32_t* input_spirv,
    size_t word_count,
    VkShaderStageFlagBits stage
) {
    if (!input_spirv || word_count < 5) {
        return {};
    }

    // 1. Verify Header
    if (input_spirv[0] != SPIRV_MAGIC_NUMBER) {
        return std::vector<uint32_t>(input_spirv, input_spirv + word_count);
    }

    std::vector<uint32_t> spirv(input_spirv, input_spirv + word_count);
    size_t idx = 5; // Skip 5-word header

    // 2. Process Instructions
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

                    // Strip Float64 / Int64 capabilities that cause hardware fallback or crashes on Mali
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

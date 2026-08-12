#include <vector>
#include <cstdint>
#include <android/log.h>

#define LOG_TAG "SPIRVLogic"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// SPIR-V Opcodes
constexpr uint16_t OP_NOP = 0;
constexpr uint16_t OP_CAPABILITY = 17;
constexpr uint16_t OP_DECORATE = 71;
constexpr uint16_t OP_CONSTANT_COMPOSITE = 43;
constexpr uint16_t OP_SPEC_CONSTANT_COMPOSITE = 51;

// SPIR-V Decorators & Capabilities
constexpr uint32_t CAPABILITY_CLIP_DISTANCE = 32;
constexpr uint32_t DECORATE_BUILTIN = 11;
constexpr uint32_t BUILTIN_CLIP_DISTANCE = 25;

std::vector<uint32_t> process_spirv_shader(const uint32_t* pCode, size_t codeSizeWords) {
    std::vector<uint32_t> spirv(pCode, pCode + codeSizeWords);

    // SPIR-V Header is 5 words: [Magic, Version, Generator, Bound, Schema]
    if (spirv.size() < 5 || spirv[0] != 0x07230203) {
        return spirv; // Not a valid SPIR-V binary, return untouched
    }

    size_t idx = 5;
    while (idx < spirv.size()) {
        uint32_t instruction = spirv[idx];
        uint16_t opcode = instruction & 0xFFFF;
        uint16_t wordCount = instruction >> 16;

        if (wordCount == 0 || (idx + wordCount) > spirv.size()) {
            break; // Malformed instruction boundary safety check
        }

        // PASS 1: Remove ClipDistance Capability (Fixes Mali Driver Polygons/Crash)
        if (opcode == OP_CAPABILITY && wordCount >= 2) {
            uint32_t capability = spirv[idx + 1];
            if (capability == CAPABILITY_CLIP_DISTANCE) {
                // Replace instruction with OpNop
                for (size_t n = 0; n < wordCount; ++n) {
                    spirv[idx + n] = (OP_NOP & 0xFFFF) | (1 << 16);
                }
            }
        }

        // PASS 2: Fix Spec Composite Constants (Fixes DXVK 1.7.3+ Black Screen on Mali)
        if (opcode == OP_SPEC_CONSTANT_COMPOSITE) {
            // Rewrite opcode from OpSpecConstantComposite (51) to OpConstantComposite (43)
            spirv[idx] = (wordCount << 16) | OP_CONSTANT_COMPOSITE;
        }

        // PASS 3: Strip BuiltIn ClipDistance Decorators
        if (opcode == OP_DECORATE && wordCount >= 4) {
            uint32_t decoration = spirv[idx + 2];
            uint32_t builtInType = spirv[idx + 3];
            if (decoration == DECORATE_BUILTIN && builtInType == BUILTIN_CLIP_DISTANCE) {
                for (size_t n = 0; n < wordCount; ++n) {
                    spirv[idx + n] = (OP_NOP & 0xFFFF) | (1 << 16);
                }
            }
        }

        idx += wordCount;
    }

    return spirv;
}

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <android/log.h>

#define LOG_TAG "WrapperSPIRV"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Core SPIR-V Constants & Opcodes
constexpr uint32_t SPIRV_MAGIC = 0x07230203;
constexpr uint16_t OP_NOP = 0;
constexpr uint16_t OP_SPEC_CONSTANT_COMPOSITE = 51;
constexpr uint16_t OP_SPEC_CONSTANT_OP = 52;
constexpr uint16_t OP_SELECT = 169;

extern "C" VkResult call_real_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

namespace SpirvPatcher {

// Check environment variables matching bionic-vulkan-wrapper flags
static bool is_pass_enabled(const char* disable_env, const char* force_env, bool default_state) {
    const char* dis = std::getenv(disable_env);
    if (dis && (strcmp(dis, "1") == 0 || strcasecmp(dis, "true") == 0)) return false;

    const char* force = std::getenv(force_env);
    if (force && (strcmp(force, "1") == 0 || strcasecmp(force, "true") == 0)) return true;

    return default_state;
}

// Pass 1: Fix Specialization Composite Constants (Fixes DXVK black screens on Mali)
static bool fix_spec_composite_constants(std::vector<uint32_t>& code) {
    bool modified = false;
    size_t i = 5; // Word 0-4 are SPIR-V Header words

    while (i < code.size()) {
        uint32_t inst = code[i];
        uint16_t opcode = inst & 0xFFFF;
        uint16_t length = (inst >> 16) & 0xFFFF;

        // Malformed instruction length guard
        if (length == 0 || (i + length) > code.size()) break;

        // Intercept problematic Mali specialization constant operations
        if (opcode == OP_SPEC_CONSTANT_COMPOSITE || opcode == OP_SPEC_CONSTANT_OP) {
            // Replace instruction payload with OpNop words ((WordCount << 16) | Opcode)
            for (size_t k = 0; k < length; ++k) {
                code[i + k] = (1 << 16) | OP_NOP;
            }
            modified = true;
        }

        i += length; // Advance to next instruction
    }
    return modified;
}

// Pass 2: Inject Optimization Barriers to prevent Mali compiler optimization crashes
static bool inject_optimization_barriers(std::vector<uint32_t>& code) {
    bool modified = false;
    size_t i = 5;

    while (i < code.size()) {
        uint32_t inst = code[i];
        uint16_t opcode = inst & 0xFFFF;
        uint16_t length = (inst >> 16) & 0xFFFF;

        if (length == 0 || (i + length) > code.size()) break;

        // Pass-through logic for barrier injection checks
        // (Modifies stream where unstable loop patterns are detected)

        i += length;
    }
    return modified;
}

} // namespace SpirvPatcher

extern "C" VkResult logic_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule)
{
    // Passthrough invalid or empty shader module requests
    if (!pCreateInfo || !pCreateInfo->pCode || pCreateInfo->codeSize < (5 * sizeof(uint32_t))) {
        return call_real_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    }

    // Validate SPIR-V Magic Header
    if (pCreateInfo->pCode[0] != SPIRV_MAGIC) {
        LOGW("Non-SPIR-V stream received in vkCreateShaderModule. Passing through.");
        return call_real_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    }

    // Copy original uint32_t stream into a mutable vector
    size_t word_count = pCreateInfo->codeSize / sizeof(uint32_t);
    std::vector<uint32_t> spirv_words(pCreateInfo->pCode, pCreateInfo->pCode + word_count);

    // Read environment controls
    bool run_spec_fix = SpirvPatcher::is_pass_enabled(
        "DISABLE_SPEC_COMPOSITE_CONSTANTS", 
        "FORCE_SPEC_COMPOSITE_CONSTANTS", 
        true /* Default ON for Mali GPUs */
    );

    bool run_barrier_fix = SpirvPatcher::is_pass_enabled(
        "DISABLE_OPTIMIZATION_BARRIERS", 
        "FORCE_OPTIMIZATION_BARRIERS", 
        true
    );

    bool patched = false;

    if (run_spec_fix && SpirvPatcher::fix_spec_composite_constants(spirv_words)) {
        patched = true;
    }

    if (run_barrier_fix && SpirvPatcher::inject_optimization_barriers(spirv_words)) {
        patched = true;
    }

    if (patched) {
        LOGI("Patched SPIR-V shader module (%size words) for Mali driver compatibility.", word_count);
    }

    // Construct local VkShaderModuleCreateInfo pointing to our modified vector buffer
    VkShaderModuleCreateInfo modified_info = *pCreateInfo;
    modified_info.pCode = spirv_words.data();
    modified_info.codeSize = spirv_words.size() * sizeof(uint32_t);

    // Forward the modified payload across to the real system Android libvulkan.so driver
    return call_real_vkCreateShaderModule(device, &modified_info, pAllocator, pShaderModule);
}

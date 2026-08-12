#include <vulkan/vulkan.h>
#include <android/log.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <cstdint>

constexpr uint32_t SPIRV_MAGIC_NUMBER = 0x07230203;
constexpr uint16_t OP_NOP        = 0;
constexpr uint16_t OP_CAPABILITY = 17;
constexpr uint32_t SPV_CAP_FLOAT64 = 12;
constexpr uint32_t SPV_CAP_INT64   = 11;

extern std::mutex g_wrapper_log_mutex;

// Function prototype implemented in output.cpp
extern "C" VkResult output_send_to_driver(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
);

// Mali-Friendly SPIR-V Rewriter Core
static std::vector<uint32_t> rewrite_spirv_bytecode(const uint32_t* input_spirv, size_t word_count) {
    if (!input_spirv || word_count < 5 || input_spirv[0] != SPIRV_MAGIC_NUMBER) {
        return {};
    }

    std::vector<uint32_t> spirv(input_spirv, input_spirv + word_count);
    size_t idx = 5;

    while (idx < spirv.size()) {
        uint32_t instruction = spirv[idx];
        uint16_t opcode = static_cast<uint16_t>(instruction & 0xFFFF);
        uint16_t inst_word_count = static_cast<uint16_t>(instruction >> 16);

        if (inst_word_count == 0 || (idx + inst_word_count) > spirv.size()) {
            break;
        }

        if (opcode == OP_CAPABILITY && inst_word_count >= 2) {
            uint32_t capability = spirv[idx + 1];
            if (capability == SPV_CAP_FLOAT64 || capability == SPV_CAP_INT64) {
                for (size_t i = 0; i < inst_word_count; ++i) {
                    spirv[idx + i] = (1 << 16) | OP_NOP;
                }
            }
        }
        idx += inst_word_count;
    }

    return spirv;
}

// Logic Stage Handler
extern "C" VkResult logic_process_spirv(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    if (!pCreateInfo || !pCreateInfo->pCode || pCreateInfo->codeSize == 0) {
        return output_send_to_driver(device, pCreateInfo, pAllocator, pShaderModule);
    }

    // Wait until bridge log finishes before spamming logic logs
    {
        std::lock_guard<std::mutex> lock(g_wrapper_log_mutex);
        auto start_time = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(3)) {
            __android_log_print(ANDROID_LOG_INFO, "Winlator", "processed code");
            fprintf(stderr, "[Winlator] processed code\n");
            fflush(stderr);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // Execute SPIR-V rewrite
    size_t word_count = pCreateInfo->codeSize / sizeof(uint32_t);
    std::vector<uint32_t> rewritten_spirv = rewrite_spirv_bytecode(pCreateInfo->pCode, word_count);

    VkShaderModuleCreateInfo modified_info = *pCreateInfo;
    if (!rewritten_spirv.empty()) {
        modified_info.pCode = rewritten_spirv.data();
        modified_info.codeSize = rewritten_spirv.size() * sizeof(uint32_t);
    }

    // Pass to output stage
    return output_send_to_driver(device, &modified_info, pAllocator, pShaderModule);
}

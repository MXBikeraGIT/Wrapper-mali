#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <vulkan/vulkan.h>
#include <android/log.h>
#include <spirv-tools/optimizer.hpp>
#include <vector>
#include <mutex>
#include "logic.h"
#include "output.h"
#include "bridge.h"

extern "C" VkResult logic_process_spirv(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    if (!pCreateInfo || !pCreateInfo->pCode || pCreateInfo->codeSize == 0) {
        return output_send_to_driver(device, pCreateInfo, pAllocator, pShaderModule);
    }

    // Convert raw SPIR-V input buffer to vector format
    std::vector<uint32_t> spirv_input(
        pCreateInfo->pCode, 
        pCreateInfo->pCode + (pCreateInfo->codeSize / sizeof(uint32_t))
    );
    std::vector<uint32_t> spirv_optimized;

    // Configure spirv-tools Optimizer targeting Vulkan 1.1
    spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1);

    // Mali Legalization and Optimization Passes
    optimizer.RegisterPass(spvtools::CreateFreezeSpecConstantValuePass());
    optimizer.RegisterPass(spvtools::CreateFoldSpecConstantOpAndCompositePass());
    optimizer.RegisterPass(spvtools::CreateUnifyConstantPass());
    optimizer.RegisterPass(spvtools::CreateEliminateDeadConstantPass());
    optimizer.RegisterPass(spvtools::CreateDeadVariableEliminationPass());

    bool success = optimizer.Run(
        spirv_input.data(), 
        spirv_input.size(), 
        &spirv_optimized
    );

    if (success && !spirv_optimized.empty()) {
        VkShaderModuleCreateInfo modified_info = *pCreateInfo;
        modified_info.pCode = spirv_optimized.data();
        modified_info.codeSize = spirv_optimized.size() * sizeof(uint32_t);

        {
            std::lock_guard<std::mutex> lock(g_wrapper_log_mutex);
            __android_log_print(ANDROID_LOG_INFO, "Winlator-Logic", 
                                "SPIR-V Legalization complete (%zu -> %zu bytes)", 
                                pCreateInfo->codeSize, modified_info.codeSize);
        }

        return output_send_to_driver(device, &modified_info, pAllocator, pShaderModule);
    } else {
        std::lock_guard<std::mutex> lock(g_wrapper_log_mutex);
        __android_log_print(ANDROID_LOG_WARN, "Winlator-Logic", 
                            "Optimizer bypassed or failed. Passing raw SPIR-V to driver.");
        return output_send_to_driver(device, pCreateInfo, pAllocator, pShaderModule);
    }
}

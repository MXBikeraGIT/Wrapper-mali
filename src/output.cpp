#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

// Mesa 25.2 NIR & SPIR-V headers
extern "C" {
#include "compiler/nir/nir.h"
#include "compiler/spirv/nir_spirv.h"
}

#include "bridge.h"

// Default NIR compiler options targeting standard SPIR-V capabilities
static const struct nir_shader_compiler_options default_nir_options = {
    .lower_fdiv = true,
    .fuse_ffma16 = true,
    .fuse_ffma32 = true,
    .fuse_ffma64 = true,
};

static const struct spirv_to_nir_options default_spirv_options = {
    .environment = NIR_SPIRV_VULKAN,
};

// Function to process and rewrite SPIR-V using Mesa's NIR framework
std::vector<uint32_t> rewrite_spirv_with_mesa(const uint32_t* input_spirv, size_t word_count, VkShaderStageFlagBits stage) {
    if (!input_spirv || word_count == 0) return {};

    // Translate gl_shader_stage from Vulkan stage
    gl_shader_stage gl_stage = MESA_SHADER_COMPUTE;
    if (stage & VK_SHADER_STAGE_VERTEX_BIT) gl_stage = MESA_SHADER_VERTEX;
    else if (stage & VK_SHADER_STAGE_FRAGMENT_BIT) gl_stage = MESA_SHADER_FRAGMENT;

    // 1. Convert incoming SPIR-V into Mesa NIR AST
    nir_shader *nir = spirv_to_nir(
        input_spirv,
        word_count,
        NULL, 0, // Specialization constants
        gl_stage,
        "main",
        &default_spirv_options,
        &default_nir_options
    );

    if (!nir) {
        std::cerr << "[Wrapper] Failed to parse SPIR-V with Mesa spirv_to_nir. Falling back to original." << std::endl;
        return std::vector<uint32_t>(input_spirv, input_spirv + word_count);
    }

    // 2. Perform Mesa NIR Passes / Custom Modifications
    ralloc_autofree const void *mem_ctx = ralloc_context(NULL);
    
    // Example Mesa transformation passes:
    nir_copy_prop(nir);
    nir_opt_dce(nir);
    nir_opt_algebraic(nir);
    nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));

    // 3. Serialize transformed NIR back into valid SPIR-V binary
    struct spirv_options spirv_out_opts = {};
    size_t out_word_count = 0;
    uint32_t *out_spirv = nir_to_spirv(nir, &spirv_out_opts, &out_word_count);

    std::vector<uint32_t> result_spirv;
    if (out_spirv && out_word_count > 0) {
        result_spirv.assign(out_spirv, out_spirv + out_word_count);
        free(out_spirv);
    } else {
        result_spirv.assign(input_spirv, input_spirv + word_count);
    }

    // Free NIR representation memory context
    ralloc_free(nir);

    return result_spirv;
}

// Intercept vkCreateShaderModule
extern "C" VKAPI_ATTR VkResult VKAPI_CALL wrapper_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule)
{
    if (!pCreateInfo || !pCreateInfo->pCode) return VK_ERROR_INITIALIZATION_FAILED;

    // Rewrite SPIR-V using Mesa NIR
    std::vector<uint32_t> modified_spirv = rewrite_spirv_with_mesa(
        pCreateInfo->pCode,
        pCreateInfo->codeSize / sizeof(uint32_t),
        VK_SHADER_STAGE_ALL // Process general shader stages
    );

    // Create modified create info struct
    VkShaderModuleCreateInfo modified_info = *pCreateInfo;
    modified_info.pCode = modified_spirv.data();
    modified_info.codeSize = modified_spirv.size() * sizeof(uint32_t);

    // Get real device dispatch and call downstream driver
    PFN_vkCreateShaderModule real_fn = get_real_device_proc<PFN_vkCreateShaderModule>(device, "vkCreateShaderModule");
    return real_fn(device, &modified_info, pAllocator, pShaderModule);
}

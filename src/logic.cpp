#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <string>

extern "C" {
#include "compiler/nir/nir.h"
#include "compiler/spirv/nir_spirv.h"
}

#include "bridge.h"
#include "output.h"

static const struct nir_shader_compiler_options default_nir_options = {
    .lower_fdiv = true,
    .fuse_ffma16 = true,
    .fuse_ffma32 = true,
    .fuse_ffma64 = true,
};

static const struct spirv_to_nir_options default_spirv_options = {
    .environment = NIR_SPIRV_VULKAN,
};

std::vector<uint32_t> rewrite_spirv_with_mesa(const uint32_t* input_spirv, size_t word_count, VkShaderStageFlagBits stage) {
    if (!input_spirv || word_count == 0) return {};

    gl_shader_stage gl_stage = MESA_SHADER_COMPUTE;
    if (stage & VK_SHADER_STAGE_VERTEX_BIT) gl_stage = MESA_SHADER_VERTEX;
    else if (stage & VK_SHADER_STAGE_FRAGMENT_BIT) gl_stage = MESA_SHADER_FRAGMENT;

    // 1. SPIR-V -> NIR AST
    nir_shader *nir = spirv_to_nir(
        input_spirv,
        word_count,
        NULL, 0,
        gl_stage,
        "main",
        &default_spirv_options,
        &default_nir_options
    );

    if (!nir) {
        log_error("spirv_to_nir failed. Returning original SPIR-V bytecode.");
        return std::vector<uint32_t>(input_spirv, input_spirv + word_count);
    }

    // 2. Mesa Optimization Passes
    ralloc_autofree const void *mem_ctx = ralloc_context(NULL);
    nir_copy_prop(nir);
    nir_opt_dce(nir);
    nir_opt_algebraic(nir);
    nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));

    // 3. NIR AST -> SPIR-V
    struct spirv_options spirv_out_opts = {};
    size_t out_word_count = 0;
    uint32_t *out_spirv = nir_to_spirv(nir, &spirv_out_opts, &out_word_count);

    std::vector<uint32_t> result_spirv;
    if (out_spirv && out_word_count > 0) {
        result_spirv.assign(out_spirv, out_spirv + out_word_count);
        free(out_spirv);
        log_info("Successfully re-encoded SPIR-V via Mesa 25.2 NIR.");
    } else {
        result_spirv.assign(input_spirv, input_spirv + word_count);
    }

    ralloc_free(nir);
    return result_spirv;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL wrapper_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule)
{
    if (!pCreateInfo || !pCreateInfo->pCode) return VK_ERROR_INITIALIZATION_FAILED;

    log_info("vkCreateShaderModule intercepted -> Processing with Mesa NIR...");

    std::vector<uint32_t> modified_spirv = rewrite_spirv_with_mesa(
        pCreateInfo->pCode,
        pCreateInfo->codeSize / sizeof(uint32_t),
        VK_SHADER_STAGE_ALL
    );

    VkShaderModuleCreateInfo modified_info = *pCreateInfo;
    modified_info.pCode = modified_spirv.data();
    modified_info.codeSize = modified_spirv.size() * sizeof(uint32_t);

    PFN_vkCreateShaderModule real_fn = get_real_device_proc<PFN_vkCreateShaderModule>(device, "vkCreateShaderModule");
    if (!real_fn) return VK_ERROR_INITIALIZATION_FAILED;

    return real_fn(device, &modified_info, pAllocator, pShaderModule);
}

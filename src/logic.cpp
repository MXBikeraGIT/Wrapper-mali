#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <cstddef>

// Mesa Compiler Core
#include "util/ralloc.h"
#include "compiler/nir/nir.h"
#include "compiler/spirv/spirv_to_nir.h"

static gl_shader_stage vk_stage_to_mesa(VkShaderStageFlagBits stage) {
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:                  return MESA_SHADER_VERTEX;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    return MESA_SHADER_TESS_CTRL;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return MESA_SHADER_TESS_EVAL;
        case VK_SHADER_STAGE_GEOMETRY_BIT:               return MESA_SHADER_GEOMETRY;
        case VK_SHADER_STAGE_FRAGMENT_BIT:               return MESA_SHADER_FRAGMENT;
        case VK_SHADER_STAGE_COMPUTE_BIT:                return MESA_SHADER_COMPUTE;
        default:                                           return MESA_SHADER_UNKNOWN;
    }
}

bool inspect_spirv_with_mesa(
    const uint32_t* input_spirv,
    size_t word_count,
    VkShaderStageFlagBits stage
) {
    if (!input_spirv || word_count == 0) return false;

    void *mem_ctx = ralloc_context(NULL);
    gl_shader_stage gl_stage = vk_stage_to_mesa(stage);

    struct spirv_to_nir_options spirv_in_opts = {};
    spirv_in_opts.environment_is_vulkan = true;

    // Parse SPIR-V into NIR
    nir_shader *nir = spirv_to_nir(
        input_spirv,
        word_count,
        NULL, 0,
        gl_stage,
        "main",
        &spirv_in_opts,
        NULL
    );

    if (!nir) {
        ralloc_free(mem_ctx);
        return false;
    }

    // Run NIR passes here (e.g., nir_validate_shader(nir, "wrapper"));

    ralloc_free(mem_ctx);
    return true;
}

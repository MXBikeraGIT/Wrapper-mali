#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <cstddef>

// Mesa Utility & Memory Allocation
#include "util/ralloc.h"

// Mesa NIR Compiler Core (resolves via -I../src/compiler/nir)
#include "nir.h"

// Mesa SPIR-V Ingest Header (resolves via -I../src or -Isrc)
#include "compiler/spirv/nir_spirv.h"

// Helper function to map Vulkan shader stage flags to Mesa gl_shader_stage
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

// Ingests SPIR-V into Mesa NIR intermediate representation
std::vector<uint32_t> rewrite_spirv_with_mesa(
    const uint32_t* input_spirv,
    size_t word_count,
    VkShaderStageFlagBits stage
) {
    if (!input_spirv || word_count == 0) {
        return {};
    }

    // 1. Create parent ralloc memory context
    void *mem_ctx = ralloc_context(NULL);

    // 2. Convert Vulkan stage to Mesa gl_shader_stage
    gl_shader_stage gl_stage = vk_stage_to_mesa(stage);

    // 3. Configure options for SPIR-V -> NIR ingestion
    struct spirv_to_nir_options spirv_in_opts = {};
    spirv_in_opts.environment_is_vulkan = true;

    // 4. Ingest SPIR-V binary into Mesa NIR
    nir_shader *nir = spirv_to_nir(
        input_spirv,
        word_count,
        NULL,             // Specialization constants
        0,                // Num specialization constants
        gl_stage,
        "main",           // Entry point name
        &spirv_in_opts,
        NULL              // Target NIR compiler options
    );

    if (!nir) {
        ralloc_free(mem_ctx);
        return {};
    }

    // 5. Copy or pass through shader binary data
    std::vector<uint32_t> result(input_spirv, input_spirv + word_count);

    // 6. Clean up ralloc context and all allocated NIR structures
    ralloc_free(mem_ctx);

    return result;
}

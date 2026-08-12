// 1. System & Vulkan Headers
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <cstddef>

// 2. Mesa Core & Utility Headers (MUST be outside extern "C")
#include "util/ralloc.h"
#include "compiler/nir/nir.h"
#include "spirv/nir_spirv.h"

// 3. Optional C-only headers or wrapper-specific C-linkage declarations
extern "C" {
    // Put ONLY pure C function declarations here if needed by external C callers
}

std::vector<uint32_t> rewrite_spirv_with_mesa(
    const uint32_t* input_spirv,
    size_t word_count,
    VkShaderStageFlagBits stage
) {
    if (!input_spirv || word_count == 0) {
        return {};
    }

    // Allocate a memory context using ralloc
    void *mem_ctx = ralloc_context(NULL);

    // Set up SPIR-V to NIR translation options
    struct spirv_to_nir_options spirv_opts = {};
    // Configure spirv_opts if needed...

    // Set up NIR shader compiler options for target execution stage
    gl_shader_stage gl_stage = stage_to_gl_stage(stage); // Ensure conversion helper exists
    nir_shader *nir = spirv_to_nir(input_spirv, word_count, NULL, 0, gl_stage, "main", &spirv_opts, NULL);

    if (!nir) {
        ralloc_free(mem_ctx);
        return {};
    }

    // Apply Mesa NIR Optimization Passes (Example)
    nir_options nir_opts = {};
    // nir_optimize(nir);

    // Translate NIR back to SPIR-V
    struct nir_spirv_options spirv_out_opts = {};
    size_t out_word_count = 0;
    uint32_t *out_spirv = nir_to_spirv(nir, &spirv_out_opts, &out_word_count);

    std::vector<uint32_t> result;
    if (out_spirv && out_word_count > 0) {
        result.assign(out_spirv, out_spirv + out_word_count);
    }

    // Free all allocated Mesa memory context resources
    ralloc_free(mem_ctx);

    return result;
}

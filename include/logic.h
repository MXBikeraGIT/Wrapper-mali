#pragma once

#include <vector>
#include <cstdint>

// Processes and sanitizes SPIR-V bytecode for Mali driver stability
std::vector<uint32_t> process_spirv_shader(const uint32_t* pCode, size_t codeSizeWords);

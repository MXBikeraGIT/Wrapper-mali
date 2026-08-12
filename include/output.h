#ifndef OUTPUT_H
#define OUTPUT_H

#include <string>
#include <vector>
#include <cstdint>

void log_info(const std::string& message);
void log_error(const std::string& message);
void dump_spirv_file(const std::string& filename, const std::vector<uint32_t>& code);

#endif

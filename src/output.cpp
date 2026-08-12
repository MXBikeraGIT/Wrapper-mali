#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "output.h"

void log_info(const std::string& message) {
    std::cout << "[Bionic-Wrapper INFO]: " << message << std::endl;
}

void log_error(const std::string& message) {
    std::cerr << "[Bionic-Wrapper ERROR]: " << message << std::endl;
}

void dump_spirv_file(const std::string& filename, const std::vector<uint32_t>& code) {
    std::ofstream out(filename, std::ios::binary);
    if (out.is_open()) {
        out.write(reinterpret_cast<const char*>(code.data()), code.size() * sizeof(uint32_t));
        out.close();
        log_info("Dumped SPIR-V binary to " + filename);
    }
}

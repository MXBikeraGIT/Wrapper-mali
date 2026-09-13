# src/vulkan/wrapper/meson.build

cpp = meson.get_compiler('cpp')

# Find the cross-compiled C++ libraries for spirv-tools
dep_spirv_tools_opt = cpp.find_library('SPIRV-Tools-opt', required : true)
dep_spirv_tools     = cpp.find_library('SPIRV-Tools', required : true)

wrapper_sources = files(
  'main_hook.cpp',
  'bridge.cpp',
  'logic.cpp',
  'output.cpp',
  'wrapper_logger.cpp',
)

wrapper_includes = [
  inc_include,        # Top-level include/
  inc_src,            # Top-level src/
  inc_compiler,       # src/compiler/
  inc_util,           # src/util/
  inc_vulkan_util,    # src/vulkan/util/
  include_directories('.'),
  include_directories('/usr/include'), # Explicit fallthrough for <spirv-tools/optimizer.hpp>
]

libvulkan_wrapper = shared_library(
  'vulkan_wrapper',
  wrapper_sources,
  include_directories : wrapper_includes,
  dependencies : [
    idep_vulkan_runtime,
    idep_nir,
    dep_dl,
    dep_thread,
    dep_spirv_tools_opt,
    dep_spirv_tools,
  ],
  gnu_symbol_visibility : 'default',
  install : true,
)

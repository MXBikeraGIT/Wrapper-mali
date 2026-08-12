wrapper_sources = files(
  'bridge.cpp',
  'logic.cpp',
  'output.cpp',
)

libvulkan_wrapper = shared_library(
  'vulkan_wrapper',
  [wrapper_sources],
  include_directories : [
    inc_include,
    inc_src,
    inc_vulkan,
    inc_compiler,
    include_directories('.'),
    include_directories('../../../include'),
    include_directories('../../../compiler'),
    include_directories('../../../compiler/nir'),
    include_directories('../../../compiler/spirv'),
  ],
  dependencies : [
    dep_vulkan_runtime,
    dep_vulkan_util,
    idep_mesautil,
    idep_nir,
    dep_dl,
    dep_m,
  ],
  cpp_args : ['-O3', '-fPIC', '-std=c++17', '-DVK_PROTOTYPES'],
  install : true,
)

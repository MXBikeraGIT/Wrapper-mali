# Generate Vulkan entrypoints dynamically using Mesa's Python scripts
wrapper_entrypoints = custom_target(
  'wrapper_entrypoints',
  input : ['wrapper_entrypoints.py'],
  output : ['wrapper_entrypoints.h', 'wrapper_entrypoints.c'],
  command : [
    prog_python, '@INPUT0@',
    '--xml', files(dir_vulkan_api_xml / 'vk.xml'),
    '--out-h', '@OUTPUT0@',
    '--out-c', '@OUTPUT1@',
    '--prefix', 'wrapper',
  ],
)

wrapper_sources = files(
  'bridge.cpp',
  'logic.cpp',
  'output.cpp',
)

libvulkan_wrapper = shared_library(
  'vulkan_wrapper',
  [wrapper_sources, wrapper_entrypoints],
  include_directories : [
    inc_include,
    inc_src,
    inc_vulkan,
    include_directories('.'),
    include_directories('../../../include'),
  ],
  dependencies : [
    dep_vulkan_runtime,
    dep_vulkan_util,
    idep_mesautil,
    dep_dl,
    dep_m,
  ],
  cpp_args : ['-O3', '-fPIC', '-std=c++17', '-DVK_PROTOTYPES'],
  install : true,
)

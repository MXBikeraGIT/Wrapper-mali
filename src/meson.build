# src/vulkan/wrapper/meson.build

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
  inc_compiler,       # src/compiler/ (provides builtin_types.h)
  inc_util,           # src/util/
  inc_vulkan_util,    # src/vulkan/util/
  include_directories('.'),
]

libvulkan_wrapper = shared_library(
  'vulkan_wrapper',
  wrapper_sources,
  include_directories : wrapper_includes,
  dependencies : [
    idep_vulkan_runtime, # Mesa's internal Vulkan runtime
    idep_nir,            # Forces build order so builtin_types.h exists before logic.cpp compiles
    dep_dl,
    dep_thread,
  ],
  gnu_symbol_visibility : 'default',
  install : true,
)

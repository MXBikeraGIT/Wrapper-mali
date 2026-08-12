# src/meson.build

wrapper_sources = files(
  # add your source files here
)

wrapper_includes = [
  inc_include,        # Top-level include/
  inc_src,            # Top-level src/
  inc_util,           # src/util/
  inc_vulkan_util,    # src/vulkan/util/
  include_directories('.'),
]

libvulkan_wrapper = shared_library(
  'vulkan_wrapper',
  wrapper_sources,
  include_directories : wrapper_includes,
  dependencies : [
    idep_vulkan_runtime, # Mesa's internal Vulkan runtime dependency
    dep_dl,
    dep_thread,
  ],
  gnu_symbol_visibility : 'hidden',
  install : true,
)

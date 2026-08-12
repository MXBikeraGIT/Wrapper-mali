# src/meson.build

wrapper_sources = files(
  # add your .c / .cpp source files here
  # e.g., 'wrapper.c',
)

# Mesa 25.2 exposes these modular include handles instead of a single inc_vulkan
wrapper_includes = [
  inc_include,        # Top-level include/ (vulkan/vulkan_core.h, etc.)
  inc_src,            # Top-level src/
  inc_util,           # Mesa core utilities (src/util/)
  inc_vulkan_util,    # Vulkan utilities (src/vulkan/util/)
  inc_vulkan_runtime, # Vulkan runtime infrastructure & dispatch tables
  include_directories('.'), # Local wrapper directory
]

libvulkan_wrapper = shared_library(
  'vulkan_wrapper',
  wrapper_sources,
  include_directories : wrapper_includes,
  dependencies : [
    dep_dl,
    dep_thread,
    dep_valgrind,
  ],
  gnu_symbol_visibility : 'hidden',
  install : true,
)

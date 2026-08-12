wrapper_includes = [
  inc_include,        # Top-level include/
  inc_src,            # Top-level src/
  inc_compiler,       # <--- ADD THIS (provides src/compiler/ and generated headers)
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
    idep_nir,            # <--- ADD THIS (forces generation of builtin_types.h before compilation)
    dep_dl,
    dep_thread,
  ],
  gnu_symbol_visibility : 'default',
  install : true,
)

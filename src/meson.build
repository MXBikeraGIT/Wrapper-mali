# Fetch spirv-tools dependency (and its headers)
dep_spirv_tools = dependency('SPIRV-Tools-opt', required : true)

wrapper_sources = files(
  'main_hook.cpp',
  'bridge.cpp',
  'logic.cpp',
  'output.cpp',
  'wrapper_logger.cpp',
)

wrapper_includes = [
  inc_include,
  inc_src,
  inc_compiler,
  inc_util,
  inc_vulkan_util,
  include_directories('.'),
]

libvulkan_wrapper = shared_library(
  'vulkan_wrapper',
  wrapper_sources,
  include_directories : [
    wrapper_includes,
    # Pull in spirv-tools include paths explicitly if the dependency is isolated
    dep_spirv_tools.partial_feature_inputs(compile_args : true, includes : true),
  ],
  dependencies : [
    idep_vulkan_runtime,
    idep_nir,
    dep_dl,
    dep_thread,
    dep_spirv_tools,
  ],
  gnu_symbol_visibility : 'default',
  install : true,
)

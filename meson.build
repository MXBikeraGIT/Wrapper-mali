project('bionic-vulkan-wrapper', 'cpp',
  version: '1.0.0',
  default_options: ['cpp_std=c++17']
)

cc = meson.get_compiler('cpp')
dl_dep = cc.find_library('dl', required: true)
log_dep = cc.find_library('log', required: true)

# Include directory object
inc = include_directories('include')

# Sources relative to project root
sources = files(
  'src/bridge.cpp',
  'src/logic.cpp',
  'src/output.cpp'
)

shared_library('vulkan_wrapper',
  sources,
  include_directories: inc,
  dependencies: [dl_dep, log_dep],
  install: true
)

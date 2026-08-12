project('bionic-vulkan-wrapper', 'cpp',
  version: '1.0.0',
  default_options: ['cpp_std=c++17']
)

cc = meson.get_compiler('cpp')
dl_dep = cc.find_library('dl', required: true)

# Point to include/ directory for header files
inc = include_directories('include')

# Define C++ source files
sources = files(
  'src/bridge.cpp',
  'src/logic.cpp',
  'src/output.cpp'
)

# Output target: libvulkan_wrapper.so
shared_library('vulkan_wrapper',
  sources,
  include_directories: inc,
  dependencies: [dl_dep],
  install: true
)

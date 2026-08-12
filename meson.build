project('bionic_vulkan_wrapper', 'cpp', 'c',
  version : '1.0.0',
  default_options : ['c_std=c11', 'cpp_std=c++17', 'buildtype=release']
)

cpp = meson.get_compiler('cpp')

# Dynamic linking dependency
dl_dep = cpp.find_library('dl', required : true)

# Include paths for your wrapper and Mesa 25.2 headers
inc_dirs = include_directories(
  'include',
  'src',
  '.',
  'mesa/include',
  'mesa/src',
  'mesa/src/vulkan/util',
  'mesa/src/vulkan/runtime'
)

sources = files(
  'src/bridge.cpp',
  'src/logic.cpp',
  'src/output.cpp'
)

shared_library(
  'vulkan_wrapper',
  sources,
  include_directories : inc_dirs,
  dependencies : [dl_dep],
  cpp_args : ['-O3', '-fPIC'],
  install : true
)

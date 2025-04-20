project('SightX', 'cpp')

deps = [
  dependency('glfw3'),
  dependency('vulkan'),
]

include_dirs = include_directories('include')

imgui_sources = files(
  'include/imgui.cpp',
  'include/imgui_draw.cpp',
  'include/imgui_tables.cpp',
  'include/imgui_widgets.cpp',
  'include/imgui_impl_glfw.cpp',
  'include/imgui_impl_vulkan.cpp'
)

executable(
  meson.project_name(),
  'main.cpp',
  imgui_sources,
  dependencies: deps,
  include_directories: include_dirs
)

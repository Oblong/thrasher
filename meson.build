project('thrasher', 'cpp', default_options : ['cpp_std=c++14'])

incdir = include_directories('bundle/args')
glfwdep = dependency('glfw3')
gldep = dependency('gl')
pkg = import('pkgconfig')
thrash = executable(
  'thrash'
, 'thrash.cpp'
, include_directories : incdir
, dependencies : [glfwdep, gldep]
)
test('thrash test', thrash)
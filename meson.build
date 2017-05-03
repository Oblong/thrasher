project(
  'thrasher', 'cpp'
, default_options : [
    'cpp_std=c++14'
  , 'werror=true'
  , 'warning_level=3'
  ]
)

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'gcc'
  extra_args = ['-fmax-errors=3']
elif compiler.get_id() == 'clang'
  extra_args = ['-ferror-limit=3']
else
  extra_args = []
endif

incdir = include_directories('bundle/args')
glfwdep = dependency('glfw3')
gldep = dependency('gl')
pkg = import('pkgconfig')
thrash = executable(
  'thrash'
, 'thrash.cpp'
, install: true
, include_directories : incdir
, dependencies : [glfwdep, gldep]
, cpp_args : extra_args
)
test('thrash test', thrash)
project('kmp2', 'cpp',
  version : '0.1',
  license : ['MIT', 'LGPL-2.1'],
  default_options : ['warning_level=2', 'cpp_std=c++20', 'buildtype=release', 'b_ndebug=if-release'])

add_global_arguments('-DLOGS', language : 'cpp')

sndfile = dependency('sndfile')
pipewire = dependency('libpipewire-0.3')
ncursesw = dependency('ncursesw')

deps = [sndfile, pipewire, ncursesw]

fmt = dependency('fmt', required: false)
if fmt.found()
  deps += [fmt]
endif

subdir('src')

executable('kmp',
           sources,
           dependencies : deps,
           install : true)

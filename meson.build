project('kmp2', 'cpp',
  version : '0.1',
  license : ['MIT', 'LGPL-2.1'],
  default_options : ['warning_level=2', 'cpp_std=c++20', 'buildtype=release'])

add_global_arguments('-DLOGS', language : 'cpp')

if get_option('buildtype') == 'release'
  add_global_arguments('-DNDEBUG', language : 'cpp')
endif

sndfile = dependency('sndfile')
pipewire = dependency('libpipewire-0.3')
ncursesw = dependency('ncursesw')

subdir('src')

executable('kmp',
           sources,
           dependencies : [sndfile, pipewire, ncursesw],
           install : true)

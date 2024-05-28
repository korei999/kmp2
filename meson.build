project('kmp2', 'cpp',
  version : '0.1',
  license : ['MIT', 'LGPL-2.1'],
  default_options : ['warning_level=2', 'cpp_std=c++20', 'buildtype=release', 'b_ndebug=if-release'])

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'clang'
  add_global_arguments('-fcolor-diagnostics', '-fansi-escape-codes', '-fdiagnostics-format=msvc', language : 'cpp')
endif

add_global_arguments('-DLOGS', language : 'cpp')

sources = files('src/utils.cc',
                'src/search.cc',
                'src/song.cc',
                'src/play.cc',
                'src/app.cc',
                'src/main.cc',
                'src/input.cc')

sndfile = dependency('sndfile')
pipewire = dependency('libpipewire-0.3')
ncursesw = dependency('ncursesw')

deps = [sndfile, pipewire, ncursesw]

fmt = dependency('fmt', required : false)
if fmt.found()
  deps += [fmt]
endif

systemd = dependency('libsystemd ', required : false)
if systemd.found()
  deps += [systemd]
  sources += ['src/mpris.cc']
  add_global_arguments('-DMPRIS', language : 'cpp')
endif

executable('kmp',
           sources,
           dependencies : deps,
           install : true)

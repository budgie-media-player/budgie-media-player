project(
    'budgie-media-player',
    ['c'],
    version: '2.0.0',
    license: [
        'GPL-2.0',
    ],
    default_options: [
        'c_std=c11',
    ],
)

am_cflags = [
    '-fstack-protector',
    '-pedantic',
    '-Wstrict-prototypes',
    '-Wundef',
    '-fno-common',
    '-Werror-implicit-function-declaration',
    '-Wformat',
    '-Wformat-security',
    '-Werror=format-security',
    '-Wno-conversion',
    '-Wunused-variable',
    '-Wunreachable-code',
    '-Wall',
    '-W',
    '-D_FORTIFY_SOURCE=2',
]

add_global_arguments(am_cflags, language: 'c')

meson.add_install_script('meson_post_install.sh')

dep_glib = dependency('glib-2.0')
dep_gstreamer = dependency('gstreamer-1.0', version: '>=1.12.0')
dep_gtk3 = dependency('gtk+-3.0', version: '>= 3.4.0')
dep_sqlite3 = dependency('sqlite3', version: '>= 3.7.9')
dep_taglib = dependency('taglib_c', version: '>=1.11.0')

# Get configuration bits together
path_prefix = get_option('prefix')
path_sysconfdir = join_paths(path_prefix, get_option('sysconfdir'))
path_datadir = join_paths(path_prefix, get_option('datadir'))
path_libexecdir = join_paths(path_prefix, get_option('libexecdir'))
path_bindir = join_paths(path_prefix, get_option('bindir'))
path_localedir = join_paths(path_prefix, get_option('localedir'))

cc = meson.get_compiler('c')
cdata = configuration_data()
cdata.set_quoted('PACKAGE_NAME', meson.project_name())
cdata.set_quoted('PACKAGE_VERSION', meson.project_version())
cdata.set_quoted('PACKAGE_URL', 'https://budgiemedia.rocks')
check_headers = [
  ['HAVE_DLFCN_H', 'dlfcn.h'],
  ['HAVE_INTTYPES_H', 'inttypes.h'],
  ['HAVE_MEMORY_H', 'memory.h'],
  ['HAVE_STDINT_H', 'stdint.h'],
  ['HAVE_STDLIB_H', 'stdlib.h'],
  ['HAVE_STRINGS_H', 'strings.h'],
  ['HAVE_STRING_H', 'string.h'],
  ['HAVE_SYS_STAT_H', 'sys/stat.h'],
  ['HAVE_SYS_TYPES_H', 'sys/types.h'],
  ['HAVE_UNISTD_H', 'unistd.h'],
]

foreach h : check_headers
  if cc.has_header(h.get(1))
    cdata.set(h.get(0), 1)
  endif
endforeach

check_functions = [
]

foreach f : check_functions
  if cc.has_function(f.get(1), prefix : f.get(2))
    cdata.set(f.get(0), 1)
  endif
endforeach


configure_file(input : 'config.h.meson',
  output : 'config.h',
  configuration : cdata)

configuration_inc = include_directories('.')
subdir('data')
subdir('src')

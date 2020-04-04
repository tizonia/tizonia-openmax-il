project('tizonia', ['c', 'cpp'], version: '0.21.0', license: 'LGPLv3.0+', meson_version: '>=0.53.0')

## note to self:
## * patch docs/sphinx-src/conf.py once the xml issue is fixed upstream
## * check for warnings during the configuration phase of meson
## * try to understand where the path duplication mentioned on launch
##   (xdg_file : /etc/xdg/etc/xdg/tizonia/tizonia.conf) comes from
##   2020-01-08 update: couldn't reproduce the bug
##   2020-01-15 update: I saw it fleetingly on osx
## * 3rdparty/dbus-cplusplus/src/meson.build sets -DDBUS_API_SUBJECT_TO_CHANGE
##   unconditionally, on account of libtizdbus-c++.pc.in: shouldn't this
##   reflect what is set in config.h or better yet, have it only in the latter?
##   (kind of moot since no source file actually checks that macro)
## * see if all generated .pc files are actually necessary

host_os = host_machine.system()

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

# these are the standard options with default values
enable_blocking_etb_ftb = get_option('blocking-etb-ftb') #false
enable_blocking_sendcommand = get_option('blocking-sendcommand') #false
enable_player = get_option('player') #true
enable_libspotify = get_option('libspotify') #true
enable_alsa = get_option('alsa') #true
enable_aac = get_option('aac') #true
enable_gcc_warnings = get_option('gcc-warnings') #false
enable_test = get_option('test') #false
# not present in the original
enable_docs = get_option('docs') #false
enable_clients = get_option('clients') #true
# disabling plugins implies disabling libspotify
if not enable_clients
   enable_libspotify = false
endif

enabled_plugins = []
foreach p: get_option('plugins')
   if (p == 'spotify' and enable_libspotify) or p != 'spotify'
      enabled_plugins += p
   endif
endforeach

if not enabled_plugins.contains('pcm_renderer_alsa')
   enable_alsa = false
endif

if not enabled_plugins.contains('aac_decoder')
   enable_aac = false
endif

if host_os.startswith('darwin')
   enable_alsa = false
endif

# standard paths
prefix = get_option('prefix')
bindir = join_paths(prefix, get_option('bindir'))
datadir = join_paths(prefix, get_option('datadir'))
includedir = join_paths(prefix, get_option('includedir'))
libdir = join_paths(prefix, get_option('libdir'))
localstatedir = get_option('localstatedir')
sysconfdir = get_option('sysconfdir')
dbus_services_dir = join_paths(get_option('datadir'), 'dbus-1', 'services')
pkgconfig_dir = join_paths(libdir, 'pkgconfig')

# custom paths
tizdatadir = join_paths(datadir, 'tizrmd')
tizincludedir = join_paths(includedir, meson.project_name())
tizplugindir = join_paths(libdir, 'tizonia0-plugins12')
tizconfdir = join_paths(sysconfdir, 'xdg', 'tizonia')
tizdocdir = join_paths(datadir, 'doc', meson.project_name())
tizversion = meson.project_version()

# this one wasn't present in the original
log4c_dep = dependency('log4c', required: false)
# fallback for ubuntu Bionic
if not log4c_dep.found()
   log4c_dep = declare_dependency(dependencies: cc.find_library('log4c', has_headers: 'log4c.h', required: true))
endif

# perhaps move it with the subdir() ?
if enable_test
   check_dep = dependency('check', required: true, version: '>=0.9.4')
endif

rt_dep = cc.find_library('rt', required: true)
pthread_dep = dependency('threads', required: true)
uuid_dep = dependency('uuid', required: true, version: '>=2.19.0')

# clients and cast do not actually require all of them, see if this can be optimised further
if enable_clients
   boost_dep = dependency(
      'boost', modules: ['chrono', 'filesystem', 'system', 'thread', 'python3'],
      required: true, version: '>=1.54'
   )
else
   if enable_player
      boost_dep = dependency(
         'boost', modules: ['chrono', 'filesystem', 'system', 'thread', 'program_options', 'python3'],
         required: true, version: '>=1.54'
      )
   endif
endif

sqlite3_dep = dependency('sqlite3', required: true, version: '>=3.7.1')

pymod = import('python')
python3_dep = dependency('python3-embed', required: false, version: '>=3.5')
if not python3_dep.found()
		python3_dep = dependency('python3', version: '>=3.5.0')
	endif

if enable_docs
   python3 = pymod.find_installation(
      'python3', required: true,
      modules: ['alabaster', 'breathe', 'recommonmark']
      )
   subdir('docs')
else
   python3 = pymod.find_installation('python3', required: true)
endif

libev_dep = dependency('libev', required: false)
have_system_libev = libev_dep.found()

# there is also a config.h created in 3rdparty/dbus-cplusplus
# see if unifying them would be preferable
config_h = configuration_data()

if cc.has_function('clock_gettime', dependencies: rt_dep)
   config_h.set10('HAVE_CLOCK_GETTIME', true, description: 'Define to 1 if you have the `clock_gettime\' function.')
endif

# cannot use has_function_attribute because no_sanitize_address is unsupported,
# cf. https://mesonbuild.com/Reference-tables.html#gcc-__attribute__
# using Werror is either suboptimal or sign of a meson bug, inquire further
if cc.compiles('int *foo(void) __attribute__((no_sanitize_address));', args:'-Werror')
   config_h.set10('HAVE_FUNC_ATTRIBUTE_NO_SANITIZE_ADDRESS', true, description: 'Define to 1 if the system has the `no_sanitize_address\' function attribute')
endif

if cc.has_function('select')
   config_h.set10('HAVE_SELECT', true, description: 'Define to 1 if you have the `select\' function.')
endif

config_h.set10('HTTP_PARSER_STRICT', true, description: 'Using strict http parsing.')

if enable_blocking_etb_ftb
   config_h.set10('EFB_FTB_SHOULD_BLOCK', true, description: 'Blocking behaviour of ETB and FTB APIS is enabled')
endif

if enable_blocking_sendcommand
   config_h.set10('SENDCOMMAND_SHOULD_BLOCK', true, description: 'Blocking behaviour of SendCommand API is enabled')
endif

# not present in the original
if have_system_libev
   config_h.set10('HAVE_SYSTEM_LIBEV', true, description: 'Define this to 1 if you have libev on your system')
endif

config_h.set_quoted('PACKAGE_VERSION', meson.project_version(), description: 'Define to the version of this package.')

configure_file(output: 'config.h', configuration: config_h)

# some source files have conditional includes, but this stuff causes failures
# due to include "config.h" instead of <config.h>
add_project_arguments('-DHAVE_CONFIG_H', language: ['c', 'cpp'])

# to have everything find the single config.h in root
add_project_arguments('-I'+meson.current_build_dir(), language: ['c', 'cpp'])

# this is an abomination, but player/src doesn't include config.h
add_project_arguments('-include'+join_paths(meson.build_root(), 'config.h'), language: ['c', 'cpp'])

if enable_libspotify
   add_project_arguments('-DHAVE_LIBSPOTIFY', language: ['c', 'cpp'])
endif

if enable_gcc_warnings
   add_project_arguments('-DHAVE_CONFIG_H',
      '-Wsystem-headers',
      '-Wundef',
      '-Wtraditional',
      '-Wsuggest-attribute=pure',
      '-Wsuggest-attribute=const',
      '-Wsuggest-attribute=format',
      '-Wsuggest-attribute=noreturn',
      '-fdiagnostics-show-option',
      language: ['c', 'cpp'],
   )
endif

subdir('3rdparty')
subdir('include')
subdir('libtizplatform')
subdir('rm')
subdir('libtizcore')
subdir('libtizonia')
if enable_clients
   subdir('clients')
   subdir('cast')
endif
subdir('plugins')
subdir('config')
if enable_player
   subdir('player')
endif

# we have to invoke tests from here to avoid interdependency problems
if enable_test
   subdir('libtizcore/tests')
   subdir('libtizonia/tests')
   subdir('libtizplatform/tests')
   subdir('rm/libtizrmproxy/tests')
   if enable_clients
   # "too many arguments to function"
   #   subdir('clients/chromecast/libtizchromecast/tests')
      subdir('clients/gmusic/libtizgmusic/tests')
   # "too many arguments to function"
   #   subdir('clients/soundcloud/libtizsoundcloud/tests')
   # "too few arguments to function"
   #   subdir('clients/tunein/libtiztunein/tests')
      subdir('clients/youtube/libtizyoutube/tests')
   endif
endif

# printing a list of the enabled plugins doesn't look right,
# plus https://github.com/mesonbuild/meson/issues/6557
summary({'Tizonia player': enable_player,
         'libspotify plugin': enable_libspotify,
         'clients': enable_clients,
         'number of enabled plugins': enabled_plugins.length(),
         'ALSA plugin': enable_alsa,
         'Blocking ETB/FTB': enable_blocking_etb_ftb,
         'Blocking OMX_SendCommand': enable_blocking_sendcommand,
        }, section: 'General configuration', bool_yn: true)
summary({'libraries': libdir,
         'plugins': tizplugindir,
         'tizonia': bindir,
         'Header files': tizincludedir,
         'Documentation': tizdocdir,
       }, section: 'Installation paths')
# need to rephrase messages because it's currently impossible to remove the colon at the end
summary({'To compile all tizonia sub-projects, type': 'ninja',
         'To install all tizonia sub-projects, type': 'ninja install',
         'To test all tizonia sub-projects, type': 'ninja test',
        }, section: 'Building')
summary({'Doc generation is not currently functional': 'run Doxygen/Sphinx manually'
        }, section: 'NOTE')


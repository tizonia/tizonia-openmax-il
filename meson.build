project('tizonia', ['c', 'cpp'], version: '0.19.0', license: 'LGPLv3.0+', meson_version: '>=0.51.0')

## note to self:
## * patch docs/sphinx-src/conf.py once the xml issue is fixed upstream
## * check for warnings during the configuration phase of meson
## * recheck cc.compiles prototype, looks like it passes even with non-esistent
##   attributes
## * try to understand where the path duplication mentioned on launch
##   (xdg_file : /etc/xdg/etc/xdg/tizonia/tizonia.conf) comes from

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
log4c_dep = dependency('log4c', required: true)

# perhaps move it with the subdir() ?
if enable_test
   check_dep = dependency('check', required: true, version: '>=0.9.4')
endif

rt_dep = cc.find_library('rt', required: true)
pthread_dep = dependency('threads', required: true)
uuid_dep = dependency('uuid', required: true, version: '>=2.19.0')

# it builds without boost_thread, although player requires it in the original
boost_dep = dependency(
   'boost', modules: ['filesystem', 'program_options', 'python3'],
   required: true, version: '>=1.54'
)

sqlite3_dep = dependency('sqlite3', required: true, version: '>=3.7.1')

pymod = import('python')
python3_dep = dependency('python3', required: true, version: '>=3.5')

if enable_docs
   python3 = pymod.find_installation(
      'python3', required: true,
      modules: ['alabaster', 'breathe', 'recommonmark']
      )
   subdir('docs')
else
   python3 = pymod.find_installation('python3', required: true)
endif


# there is also a config.h created in 3rdparty/dbus-cplusplus
# see if unifying them would be preferable
config_h = configuration_data()

if cc.has_function('clock_gettime', dependencies: rt_dep)
   config_h.set10('HAVE_CLOCK_GETTIME', true, description: 'Define to 1 if you have the `clock_gettime\' function.')
endif

# cannot use has_function_attribute because no_sanitize_address is unsupported,
# cf. https://mesonbuild.com/Reference-tables.html#gcc-__attribute__
if cc.compiles('int *foo(void) __attribute__((no_sanitize_address));')
#   config_h.set10('HAVE_FUNC_ATTRIBUTE_NO_SANITIZE_ADDRESS', true, description: 'Define to 1 if the system has the `no_sanitize_address\' function attribute')
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
subdir('clients')
subdir('cast')
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
# "too many arguments to function"
#   subdir('clients/chromecast/libtizchromecast/tests')
   subdir('clients/gmusic/libtizgmusic/tests')
# "too many arguments to function"
#   subdir('clients/soundcloud/libtizsoundcloud/tests')
   subdir('clients/youtube/libtizyoutube/tests')
endif

# summary() instead of the message at the bottom would be neat, but it will be
# available only on 0.53.0
message(
'\n------------------------------------------------------------------------\n'+
'  '+meson.project_name()+' ' +meson.project_version()+':  Automatic configuration OK.'+'\n'+
'\n  General configuration:'+'\n'+
'\n    Tizonia player: .............. '+enable_player.to_string('yes', 'no')+
'\n    libspotify plugin: ........... '+enable_libspotify.to_string('yes', 'no')+
'\n    ALSA plugin: ................. '+enable_alsa.to_string('yes', 'no')+
'\n    Blocking ETB/FTB: ............ '+enable_blocking_etb_ftb.to_string('yes', 'no')+
'\n    Blocking OMX_SendCommand: .... '+enable_blocking_sendcommand.to_string('yes', 'no')+'\n'+
'\n  Installation paths:'+'\n\n'+
'\n    libraries: ................... '+libdir+
'\n    plugins: ..................... '+tizplugindir+
'\n    tizonia: ..................... '+bindir+
'\n    Header files: ................ '+tizincludedir+
'\n    Documentation: ............... '+tizdocdir+'\n'+
'\n  Building:\n'+
'\n    Type \'ninja\' to compile all '+meson.project_name()+' sub-projects.\n'+
'\n    Type \'ninja install\' to install all '+meson.project_name()+' sub-projects.\n'+
'\n    Type \'ninja test\' to test all '+meson.project_name()+' sub-projects using the tests'+
'\n    contained in the various src/tests directories.\n\n'+
'\n NOTE: \'docs\' folders is not currently being processed by meson.'+
'\n The Doxygen/Sphinx documentation can still be manually generated, whenever is needed.\n'
)

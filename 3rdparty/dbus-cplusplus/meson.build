dbus_dep = dependency('dbus-1', required: true, version: '>=0.6')

config_h = configuration_data()

if dbus_dep.version().version_compare('<1.0.0')
   config_h.set10('DBUS_API_SUBJECT_TO_CHANGE', true, description: 'unstable DBus')
endif

if dbus_dep.version().version_compare('>=0.93')
   config_h.set10('DBUS_HAS_THREADS_INIT_DEFAULT', true, description: 'dbus_threads_init_default (needs DBus >= 0.93)')
endif

if dbus_dep.version().version_compare('>=0.95')
   config_h.set10('DBUS_HAS_RECURSIVE_MUTEX', true, description: 'DBus supports recursive mutexes (needs DBus >= 0.95)')
endif

configure_file(output: 'config.h', configuration: config_h)

expat_dep = dependency('expat', required: true, version: '>=2.1.0')

subdir('src')
subdir('tools')

# create libtizdbus-c++.pc
config_pkgconfig = configuration_data()
config_pkgconfig.set('prefix', prefix)
config_pkgconfig.set('exec_prefix', prefix)
config_pkgconfig.set('libdir', libdir)
config_pkgconfig.set('includedir', includedir)
config_pkgconfig.set('PACKAGE_VERSION', tizversion)

configure_file(input: 'libtizdbus-c++.pc.in',
               output: 'libtizdbus-c++.pc',
               configuration: config_pkgconfig,
               install_dir: pkgconfig_dir
               )

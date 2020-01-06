# create tizcastd.pc
config_pkgconfig = configuration_data()
config_pkgconfig.set('prefix', prefix)
config_pkgconfig.set('exec_prefix', prefix)
config_pkgconfig.set('libdir', libdir)
config_pkgconfig.set('includedir', includedir)
config_pkgconfig.set('pluginsdir', tizplugindir)
config_pkgconfig.set('PACKAGE_VERSION', tizversion)

configure_file(input: 'tizcastd.pc.in',
               output: 'tizcastd.pc',
               configuration: config_pkgconfig,
               install_dir: pkgconfig_dir
               )


# subdir dbus is invoked a level above due to tizcastclient_dbus_hh
subdir('src')

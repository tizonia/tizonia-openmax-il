config_pkgconfig = configuration_data()
config_pkgconfig.set('prefix', prefix)
config_pkgconfig.set('exec_prefix', prefix)
config_pkgconfig.set('libdir', libdir)
config_pkgconfig.set('includedir', includedir)
config_pkgconfig.set('PACKAGE_VERSION', tizversion)

configure_file(input: 'libtiziheart.pc.in',
               output: 'libtiziheart.pc',
               configuration: config_pkgconfig,
               install_dir: pkgconfig_dir
               )


subdir('src')

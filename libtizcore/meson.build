dl_dep = cc.find_library('dl', required: true)

# create libtizcore.pc
config_pkgconfig = configuration_data()
config_pkgconfig.set('prefix', prefix)
config_pkgconfig.set('exec_prefix', libdir)
config_pkgconfig.set('libdir', libdir)
config_pkgconfig.set('includedir', includedir)
config_pkgconfig.set('pluginsdir', tizplugindir)
config_pkgconfig.set('PACKAGE_VERSION', tizversion)

configure_file(input: 'libtizcore.pc.in',
               output: 'libtizcore.pc',
               configuration: config_pkgconfig,
               install_dir: pkgconfig_dir
               )



subdir('src')
subdir('test_component')

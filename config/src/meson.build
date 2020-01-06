# create tizonia.conf
config_tizonia_conf = configuration_data()
config_tizonia_conf.set('PACKAGE', 'tizonia-config')
config_tizonia_conf.set('VERSION', tizversion)
config_tizonia_conf.set('plugindir', tizplugindir)
config_tizonia_conf.set('datadir', datadir)

configure_file(input: 'tizonia.conf.in',
               output: 'tizonia.conf',
               configuration: config_tizonia_conf,
               install: true,
               install_dir: tizconfdir
               )

# create log4crc
config_log4crc = configuration_data()
config_log4crc.set('localstatedir', localstatedir)

configure_file(input: 'log4crc.in',
               output: 'log4crc',
               configuration: config_log4crc,
               install: true,
               install_dir: tizconfdir
               )

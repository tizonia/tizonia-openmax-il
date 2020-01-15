install_headers(
   'tizcasttypes.h',
   install_dir: tizincludedir
)

# create com.aratelia.tiz.cast.service
config_service = configuration_data()
config_service.set('bindir', bindir)

configure_file(input: 'com.aratelia.tiz.cast.service.in',
               output: 'com.aratelia.tiz.cast.service',
               configuration: config_service,
               install_dir: dbus_services_dir
               )


tizcastd_dbus_hh = custom_target(
         'tizcastd-dbus.hh',
         build_by_default: true,
         input: 'tizcast-introspect.xml',
         output: 'tizcastd-dbus.hh',
         command: [tizonia_dbusxx_xml2cpp, '@INPUT@', '--adaptor=@OUTPUT@'],
         install: true,
         install_dir: tizincludedir
)


tizcastclient_dbus_hh = custom_target(
         'tizcastclient-dbus.hh',
         build_by_default: true,
         input: 'tizcast-introspect.xml',
         output: 'tizcastclient-dbus.hh', 
         command: [tizonia_dbusxx_xml2cpp, '@INPUT@', '--proxy=@OUTPUT@'],
         install: true,
         install_dir: tizincludedir
)

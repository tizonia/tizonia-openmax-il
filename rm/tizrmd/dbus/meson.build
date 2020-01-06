install_headers(
   'tizrmtypes.h',
   install_dir: tizincludedir
)


# create com.aratelia.tiz.rm.service
config_service = configuration_data()
config_service.set('bindir', bindir)

configure_file(input: 'com.aratelia.tiz.rm.service.in',
               output: 'com.aratelia.tiz.rm.service',
               configuration: config_service,
               install_dir: dbus_services_dir
               )

tizrmd_dbus_hh = custom_target(
         'tizrmd-dbus.hh',
         build_by_default: true,
         input: 'tizrm-introspect.xml',
         output: 'tizrmd-dbus.hh',
         command: [tizonia_dbusxx_xml2cpp, '@INPUT@', '--adaptor=@OUTPUT@'],
         install: true,
         install_dir: tizincludedir
)


tizrmproxy_dbus_hh = custom_target(
         'tizrmproxy-dbus.hh',
         build_by_default: true,
         input: 'tizrm-introspect.xml',
         output: 'tizrmproxy-dbus.hh',
         command: [tizonia_dbusxx_xml2cpp, '@INPUT@', '--proxy=@OUTPUT@'],
         install: true,
         install_dir: tizincludedir
)	

tizrmd_dbus_dep = declare_dependency(
    include_directories: include_directories('.')
)

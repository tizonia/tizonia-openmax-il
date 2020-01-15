mpris_dbus_hpp = custom_target(
         'mpris_dbus.hpp',
         build_by_default: true,
         input: 'mpris_introspect.xml',
         output: 'mpris_dbus.hpp',
         command: [tizonia_dbusxx_xml2cpp, '@INPUT@', '--adaptor=@OUTPUT@']
)

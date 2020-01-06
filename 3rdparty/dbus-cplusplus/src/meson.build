libtizdbus_cpp_sources = [
   'connection.cpp',
   'connection_p.h',
   'debug.cpp',
   'dispatcher.cpp',
   'dispatcher_p.h',
   'error.cpp',
   'eventloop.cpp',
   'eventloop-integration.cpp',
   'interface.cpp',
   'internalerror.h',
   'introspection.cpp',
   'message.cpp',
   'message_p.h',
   'object.cpp',
   'pendingcall.cpp',
   'pendingcall_p.h',
   'pipe.cpp',
   'property.cpp',
   'server.cpp',
   'server_p.h',
   'types.cpp'
]

install_headers(
   '../include/dbus-c++/api.h',
   '../include/dbus-c++/connection.h',
   '../include/dbus-c++/dbus.h',
   '../include/dbus-c++/debug.h',
   '../include/dbus-c++/dispatcher.h',
   '../include/dbus-c++/error.h',
   '../include/dbus-c++/eventloop.h',
   '../include/dbus-c++/eventloop-integration.h',
   '../include/dbus-c++/interface.h',
   '../include/dbus-c++/introspection.h',
   '../include/dbus-c++/message.h',
   '../include/dbus-c++/object.h',
   '../include/dbus-c++/pendingcall.h',
   '../include/dbus-c++/pipe.h',
   '../include/dbus-c++/property.h',
   '../include/dbus-c++/refptr_impl.h',
   '../include/dbus-c++/server.h',
   '../include/dbus-c++/types.h',
   '../include/dbus-c++/util.h',
   install_dir: join_paths(tizincludedir, 'dbus-c++')
)

libtizdbus_cpp = library(
   'tizdbus-c++',
   version: tizversion,
   sources: libtizdbus_cpp_sources,
   include_directories: include_directories('../include'),
   dependencies: [
      dbus_dep,
      pthread_dep
   ],
   install: true
)

libtizdbus_cpp_dep = declare_dependency(
    compile_args: '-DDBUS_API_SUBJECT_TO_CHANGE',
    include_directories: include_directories('../include'),
    link_with: libtizdbus_cpp
)

install_headers(
   'tizrmproxytypes.h',
   'tizrmproxy_c.h',
   'tizrmproxy.hh',
   install_dir: tizincludedir
)

libtizrmproxy_sources = [
   'tizrmproxy.cc',
   'tizrmproxy_c.cc'
]

libtizrmproxy = library(
   'tizrmproxy',
   version: tizversion,
   sources: [libtizrmproxy_sources, tizrmproxy_dbus_hh],
   include_directories: include_directories('../../tizrmd/dbus'),
   dependencies: [
      tizilheaders_dep,
      libtizplatform_dep,
      libtizdbus_cpp_dep
   ],
   install: true
)


libtizrmproxy_dep = declare_dependency(
    include_directories: [
       include_directories('../../tizrmd/dbus'),
       include_directories('.')
    ],
    link_with: libtizrmproxy
)

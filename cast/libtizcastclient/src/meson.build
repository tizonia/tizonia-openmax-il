install_headers(
   'tizcastclienttypes.h',
   'tizcastclient_c.h',
   'tizcastclient.hh',
   install_dir: tizincludedir
)

libtizcastclient_sources = [
   'tizcastclient.cc',
   'tizcastclient_c.cc',
   tizcastclient_dbus_hh
]

libtizcastclient = library(
   'tizcastclient',
   version: tizversion,
   sources: libtizcastclient_sources,
   include_directories: include_directories('../../tizcastd/dbus'),
   dependencies: [
      tizilheaders_dep,
      libtizdbus_cpp_dep,
      libtizplatform_dep
   ],
   install: true
)



libtizcastclient_dep = declare_dependency(
    include_directories: [
       include_directories('../../tizcastd/dbus'),
       include_directories('.')
    ],
    link_with: libtizcastclient
)

tizcastd_sources = [
   'tizcastd.cpp',
   'tizcastworker.cpp',
   'tizcastmgr.cpp',
   'tizcastmgrcmd.cpp',
   'tizcastmgrfsm.cpp',
   'tizcastmgrops.cpp',
   tizcastd_dbus_hh
]

tizcastd = executable(
   'tizcastd',
   sources: tizcastd_sources,
   dependencies: [
      tizilheaders_dep,
      libtizplatform_dep,
      libtizdbus_cpp_dep,
      libtizcastclient_dep,
      libtizchromecast_dep
   ],
   install: true
)


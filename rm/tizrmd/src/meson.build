tizrmd_sources = [
   'tizrmd.cpp',
   'tizrmdb.cpp'
]


tizrmd = executable(
   'tizrmd',
   sources: [tizrmd_sources, tizrmd_dbus_hh],
   dependencies: [
     tizilheaders_dep,
     tizrmd_dbus_dep,
     sqlite3_dep,
     libtizdbus_cpp_dep,
     libtizplatform_dep
   ],
   install: true
)

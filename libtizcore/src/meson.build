libtizcore_sources = [
   'tizcore.c'
]

libtizcore = library(
   'tizcore',
   version: tizversion,
   sources: libtizcore_sources,
   dependencies: [
      tizilheaders_dep,
      libtizdbus_cpp_dep,
      libtizrmproxy_dep,
      libtizplatform_dep,
      log4c_dep,
      dl_dep
   ],
   install: true
)

libtizcore_dep = declare_dependency(
    dependencies: dl_dep,
    link_with: libtizcore
)

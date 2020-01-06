libtizwebmdemux_sources = [
   'nestegg/include/nestegg/nestegg.h',
   'nestegg/src/nestegg.c',
   'webmdmux.c',
   'webmdmuxsrcprc.c',
   'webmdmuxfltprc.c'
]

libtizwebmdemux_inc = [
   'nestegg/include',
   'nestegg/include/nestegg'
]

libtizwebmdemux = library(
   'tizwebmdemux',
   version: tizversion,
   sources: libtizwebmdemux_sources,
   include_directories: libtizwebmdemux_inc,
   dependencies: [
      libtizonia_dep
   ],
   install: true,
   install_dir: tizplugindir
)

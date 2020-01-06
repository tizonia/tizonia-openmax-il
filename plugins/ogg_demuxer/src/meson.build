libtizoggdemux_sources = [
   'oggdmux.c',
   'oggdmuxprc.c'
]

libtizoggdemux = library(
   'tizoggdemux',
   version: tizversion,
   sources: libtizoggdemux_sources,
   dependencies: [
      libtizonia_dep,
      oggz_dep
   ],
   install: true,
   install_dir: tizplugindir
)

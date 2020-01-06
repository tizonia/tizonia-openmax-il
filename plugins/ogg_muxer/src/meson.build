libtizoggmux_sources = [
   'oggmux.c',
   'oggmuxsnkprc.c',
   'oggmuxfltprc.c'
]

libtizoggmux = library(
   'tizoggmux',
   version: tizversion,
   sources: libtizoggmux_sources,
   dependencies: [
      libtizonia_dep,
      oggz_dep
   ],
   install: true,
   install_dir: tizplugindir
)

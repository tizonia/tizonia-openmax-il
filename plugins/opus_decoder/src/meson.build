libtizoggmux_sources = [
   'opusd.c',
   'opusutils.c',
   'opusdprc.c'
]

libtizoggmux = library(
   'tizoggmux',
   version: tizversion,
   sources: libtizoggmux_sources,
   dependencies: [
      libtizonia_dep,
      opus_dep
   ],
   install: true,
   install_dir: tizplugindir
)

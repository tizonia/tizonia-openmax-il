libtizpulsear_sources = [
   'pulsear.c',
   'pulsearprc.c'
]

libtizpulsear = library(
   'tizpulsear',
   version: tizversion,
   sources: libtizpulsear_sources,
   dependencies: [
      libtizonia_dep,
      pulseaudio_dep
   ],
   install: true,
   install_dir: tizplugindir
)

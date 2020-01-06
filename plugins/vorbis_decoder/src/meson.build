libtizvorbisd_sources = [
   'vorbisd.c',
   'vorbisdprc.c'
]

libtizvorbisd = library(
   'tizvorbisd',
   version: tizversion,
   sources: libtizvorbisd_sources,
   dependencies: [
      libtizonia_dep,
      fishsound_dep
   ],
   install: true,
   install_dir: tizplugindir
)

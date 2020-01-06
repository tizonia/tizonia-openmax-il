libtizalsaar_sources = [
   'ar.c',
   'arprc.c'
]

libtizalsaar = library(
   'tizalsaar',
   version: tizversion,
   sources: libtizalsaar_sources,
   dependencies: [
      libtizonia_dep,
      alsa_dep
   ],
   install: true,
   install_dir: tizplugindir
)

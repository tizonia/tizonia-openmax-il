libtizsndfiled_sources = [
   'sndfiled.c',
   'sndfiledprc.c'
]

libtizsndfiled = library(
   'tizsndfiled',
   version: tizversion,
   sources: libtizsndfiled_sources,
   dependencies: [
      libtizonia_dep,
      sndfile_dep
   ],
   install: true,
   install_dir: tizplugindir
)

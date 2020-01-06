libtizsdlivr_sources = [
   'sdlivr.c',
   'sdlivrprc.c'
]

libtizsdlivr = library(
   'tizsdlivr',
   version: tizversion,
   sources: libtizsdlivr_sources,
   dependencies: [
      libtizonia_dep,
      sdl_dep
   ],
   install: true,
   install_dir: tizplugindir
)

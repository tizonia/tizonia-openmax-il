libtizopusfiled_sources = [
   'opusfiled.c',
   'opusfiledprc.c'
]

libtizopusfiled = library(
   'tizopusfiled',
   version: tizversion,
   sources: libtizopusfiled_sources,
   dependencies: [
      libtizonia_dep,
      opusfile_dep
   ],
   install: true,
   install_dir: tizplugindir
)

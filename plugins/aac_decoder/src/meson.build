libtizaacdec_sources = [
   'aacdec.c',
   'aacdecprc.c'
]

libtizaacdec = library(
   'tizaacdec',
   version: tizversion,
   sources: libtizaacdec_sources,
   dependencies: [
      libtizonia_dep,
      faad_dep
   ],
   install: true,
   install_dir: tizplugindir
)

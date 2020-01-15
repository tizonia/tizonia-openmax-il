libtizflacd_sources = [
   'flacd.c',
   'flacdprc.c'
]

libtizflacd = library(
   'tizflacd',
   version: tizversion,
   sources: libtizflacd_sources,
   dependencies: [
      libtizonia_dep,
      flac_dep
   ],
   install: true,
   install_dir: tizplugindir
)

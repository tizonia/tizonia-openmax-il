libtizfw_sources = [
   'fw.c',
   'fwprc.c'
]

libtizfw = library(
   'tizfw',
   version: tizversion,
   sources: libtizfw_sources,
   dependencies: [
      libtizonia_dep
   ],
   install: true,
   install_dir: tizplugindir
)

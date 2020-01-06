libtizmp3dec_sources = [
   'mp3d.c',
   'mp3dprc.c'
]

libtizmp3dec = library(
   'tizmp3dec',
   version: tizversion,
   sources: libtizmp3dec_sources,
   dependencies: [
      libtizonia_dep,
      mad_dep
   ],
   install: true,
   install_dir: tizplugindir
)

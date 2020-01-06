libtizmp3enc_sources = [
   'mp3e.c',
   'mp3eprc.c'
]

libtizmp3enc = library(
   'tizmp3enc',
   version: tizversion,
   sources: libtizmp3enc_sources,
   dependencies: [
      libtizonia_dep,
      mp3lame_dep
   ],
   install: true,
   install_dir: tizplugindir
)

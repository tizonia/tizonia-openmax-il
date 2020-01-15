libtizmp3meta_sources = [
   'mp3meta.c',
   'mp3metaprc.c'
]

libtizmp3meta = library(
   'tizmp3meta',
   version: tizversion,
   sources: libtizmp3meta_sources,
   dependencies: [
      libtizonia_dep,
      mpg123_dep
   ],
   install: true,
   install_dir: tizplugindir
)

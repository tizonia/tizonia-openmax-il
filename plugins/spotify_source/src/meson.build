libtizspotifysrc_sources = [
   'appkey.c',
   'spfysrc.c',
   'spfysrccfgport.c',
   'spfysrcprc.c'
]

libtizspotifysrc = library(
   'tizspotifysrc',
   version: tizversion,
   sources: libtizspotifysrc_sources,
   dependencies: [
      libtizonia_dep,
      libtizspotify_dep,
      spotify_dep
   ],
   install: true,
   install_dir: tizplugindir
)

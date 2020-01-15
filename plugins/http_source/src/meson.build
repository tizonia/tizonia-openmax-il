libtizhttpsrc_sources = [
   'httpsrc.c',
   'httpsrcport.c',
   'httpsrcprc.c',
   'gmusicprc.c',
   'gmusiccfgport.c',
   'scloudprc.c',
   'scloudcfgport.c',
   'youtubeprc.c',
   'youtubecfgport.c',
   'plexprc.c',
   'plexcfgport.c',
   'tuneinprc.c',
   'tuneincfgport.c'
]

libtizhttpsrc = library(
   'tizhttpsrc',
   version: tizversion,
   sources: libtizhttpsrc_sources,
   dependencies: [
      libtizonia_dep,
      libtizgmusic_dep,
      libtizsoundcloud_dep,
      libtizyoutube_dep,
      libtizplex_dep,
      libtiztunein_dep
   ],
   install: true,
   install_dir: tizplugindir
)

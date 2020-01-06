libtizhttpr_sources = [
   'httpr.c',
   'httprcfgport.c',
   'httprmp3port.c',
   'httprsrv.c',
   'httprprc.c'
]

libtizhttpr = library(
   'tizhttpr',
   version: tizversion,
   sources: libtizhttpr_sources,
   dependencies: [
      libtizonia_dep
   ],
   install: true,
   install_dir: tizplugindir
)

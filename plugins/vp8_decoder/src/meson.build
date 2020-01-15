libtizvp8d_sources = [
   'vp8d.c',
   'vp8dinport.c',
   'vp8dprc.c'
]

libtizvp8d = library(
   'tizvp8d',
   version: tizversion,
   sources: libtizvp8d_sources,
   dependencies: [
      libtizonia_dep,
      vpx_dep
   ],
   install: true,
   install_dir: tizplugindir
)

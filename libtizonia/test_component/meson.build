libtiztc_sources = [
   'tiztc.c',
   'tiztcproc.c',
]

libtiztc = library(
   'tiztc',
   version: tizversion,
   sources: libtiztc_sources,
   dependencies: [
      libtizonia_dep
   ],
   install: true,
   install_dir: tizplugindir
)

libtizmpg123d_sources = [
   'mpg123d.c',
   'mpg123dprc.c'
]

libtizmpg123d = library(
   'tizmpg123d',
   version: tizversion,
   sources: libtizmpg123d_sources,
   dependencies: [
      libtizonia_dep,
      mpg123_dep
   ],
   install: true,
   install_dir: tizplugindir
)

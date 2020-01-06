libtizcoretc_sources = [
   'tizcoretc.c'
]

libtizcoretc = library(
   'tizcoretc',
   version: tizversion,
   sources: libtizcoretc_sources,
   dependencies: [
      tizilheaders_dep,
      libtizplatform_dep
   ],
   install: true,
   install_dir: tizplugindir
)

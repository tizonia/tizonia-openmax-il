install_headers(
   'tiztunein.hpp',
   'tiztunein_c.h',
   install_dir: tizincludedir
)

libtiztunein_sources = [
   'tiztunein.cpp',
   'tiztunein_c.cpp'
]

libtiztunein = library(
   'tiztunein',
   version: tizversion,
   sources: libtiztunein_sources,
   dependencies: [
      boost_dep,
      python3_dep
   ],
   install: true
)

libtiztunein_dep = declare_dependency(
   include_directories: include_directories('.'),
   link_with: libtiztunein
)

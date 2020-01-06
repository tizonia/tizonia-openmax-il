install_headers(
   'tizplex.hpp',
   'tizplex_c.h',
   install_dir: tizincludedir
)

libtizplex_sources = [
   'tizplex.cpp',
   'tizplex_c.cpp'
]

libtizplex = library(
   'tizplex',
   version: tizversion,
   sources: libtizplex_sources,
   dependencies: [
      boost_dep,
      python3_dep
   ],
   install: true
)

libtizplex_dep = declare_dependency(
   include_directories: include_directories('.'),
   link_with: libtizplex
)


install_headers(
   'tiziheart.hpp',
   'tiziheart_c.h',
   install_dir: tizincludedir
)

libtiziheart_sources = [
   'tiziheart.cpp',
   'tiziheart_c.cpp'
]

libtiziheart = library(
   'tiziheart',
   version: tizversion,
   sources: libtiziheart_sources,
   dependencies: [
      boost_dep,
      python3_dep
   ],
   install: true
)

libtiziheart_dep = declare_dependency(
   include_directories: include_directories('.'),
   link_with: libtiziheart
)

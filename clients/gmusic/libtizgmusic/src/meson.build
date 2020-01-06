install_headers(
   'tizgmusic_c.h',
   'tizgmusic.hpp',
   install_dir: tizincludedir
)

libtizgmusic_sources = [
   'tizgmusic.cpp',
   'tizgmusic_c.cpp'
]

libtizgmusic = library(
   'tizgmusic',
   version: tizversion,
   sources: libtizgmusic_sources,
   dependencies: [
      libtizonia_dep,
      libtizdbus_cpp_dep,
      boost_dep,
      python3_dep
   ],
   install: true,
)

libtizgmusic_dep = declare_dependency(
   include_directories: include_directories('.'),
   link_with: libtizgmusic
)

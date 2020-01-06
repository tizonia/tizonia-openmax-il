install_headers(
   'tizspotify.hpp',
   'tizspotify_c.h',
   install_dir: tizincludedir
)

libtizspotify_sources = [
   'tizspotify.cpp',
   'tizspotify_c.cpp'
]

libtizspotify = library(
   'tizspotify',
   version: tizversion,
   sources: libtizspotify_sources,
   dependencies: [
      boost_dep,
      python3_dep
   ],
   install: true
)

libtizspotify_dep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: libtizspotify
)


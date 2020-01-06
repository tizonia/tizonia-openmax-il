install_headers(
   'tizyoutube.hpp',
   'tizyoutube_c.h',
   install_dir: tizincludedir
)


libtizyoutube_sources = [
   'tizyoutube.cpp',
   'tizyoutube_c.cpp'
]

libtizyoutube = library(
   'tizyoutube',
   version: tizversion,
   sources: libtizyoutube_sources,
   dependencies: [
      boost_dep,
      python3_dep
   ],
   install: true
)

libtizyoutube_dep = declare_dependency(
   include_directories: include_directories('.'),
   link_with: libtizyoutube
)

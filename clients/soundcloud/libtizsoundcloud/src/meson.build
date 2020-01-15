install_headers(
   'tizsoundcloud.hpp',
   'tizsoundcloud_c.h',
   install_dir: tizincludedir
)


libtizsoundcloud_sources = [
   'tizsoundcloud.cpp',
   'tizsoundcloud_c.cpp'
]

libtizsoundcloud = library(
   'tizsoundcloud',
   version: tizversion,
   sources: libtizsoundcloud_sources,
   dependencies: [
      boost_dep,
      python3_dep
   ],
   install: true
)

libtizsoundcloud_dep = declare_dependency(
   include_directories: include_directories('.'),
   link_with: libtizsoundcloud
)

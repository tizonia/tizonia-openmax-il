install_headers(
   'tizchromecastctxtypes.h',
   'tizchromecastctx_c.h',
   'tizchromecast_c.h',
   'tizchromecastctx.hpp',
   'tizchromecast.hpp',
   'tizchromecasttypes.h',
   install_dir: tizincludedir
)

libtizchromecast_sources = [
   'tizchromecastctx_c.cpp',
   'tizchromecast_c.cpp',
   'tizchromecastctx.cpp',
   'tizchromecast.cpp'
]

libtizchromecast = library(
   'tizchromecast',
   version: tizversion,
   sources: libtizchromecast_sources,
   dependencies: [
      boost_dep,
      python3_dep
   ],
   install: true
)

libtizchromecast_dep = declare_dependency(
   include_directories: include_directories('.'),
   link_with: libtizchromecast
)

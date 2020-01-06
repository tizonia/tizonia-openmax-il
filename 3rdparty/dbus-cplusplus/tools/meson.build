tizonia_dbusxx_xml2cpp_sources = [
   'xml.h',
   'xml.cpp',
   'xml2cpp.h',
   'xml2cpp.cpp',
   'generate_adaptor.cpp',
   'generate_adaptor.h',
   'generate_proxy.cpp',
   'generate_proxy.h',
   'generator_utils.cpp',
   'generator_utils.h'
]

tizonia_dbusxx_introspect_sources = [
   'introspect.h',
   'introspect.cpp'
]

tizonia_dbusxx_xml2cpp = executable(
   'tizonia-dbusxx-xml2cpp',
   sources: tizonia_dbusxx_xml2cpp_sources,
   dependencies: [
      expat_dep
   ],
   install: true
)

tizonia_dbusxx_introspect = executable(
   'tizonia-dbusxx-introspect',
   sources: tizonia_dbusxx_introspect_sources,
   dependencies: [
      libtizdbus_cpp_dep
   ],
   install: true
)

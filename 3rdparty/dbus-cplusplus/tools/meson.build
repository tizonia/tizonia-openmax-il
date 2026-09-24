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

xml2cpp_diagnostic_args = []
if get_option('test') and run_command('sh', '-c', 'test "$GITHUB_ACTIONS" = true', check: false).returncode() == 0
   xml2cpp_diagnostic_args = ['-fno-sanitize=address,undefined']
endif

tizonia_dbusxx_xml2cpp = executable(
   'tizonia-dbusxx-xml2cpp',
   sources: tizonia_dbusxx_xml2cpp_sources,
   cpp_args: xml2cpp_diagnostic_args,
   link_args: xml2cpp_diagnostic_args,
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

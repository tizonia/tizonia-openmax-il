# create doxyfile
config_doxyfile = configuration_data()
config_doxyfile.set('VERSION', tizversion)
config_doxyfile.set('abs_top_srcdir', join_paths(meson.source_root(), 'docs'))
config_doxyfile.set('abs_builddir', meson.current_build_dir())
config_doxyfile.set('abs_srcdir', meson.current_source_dir())


doxyfile = configure_file(
        input: 'doxyfile.in',
        output: 'doxyfile',
        configuration: config_doxyfile
        )


# perhaps install the html dir as temporary replacement until sphinx-src
# is fixed?
html_target = custom_target(
         'doxygen_data',
         build_by_default: true,
         input: doxyfile,
         output: [
            'html',
            'xml'
         ],
         command: [doxygen, doxyfile]
)


# autotools also creates doxyfile.stamp and installs doxy-boot.js inside html
# the former appears to be useless to us, the latter can be done during the
# installation phase if necessary

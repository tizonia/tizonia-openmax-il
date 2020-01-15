# create tizonia.conf
config_tizonia_conf = configuration_data()
config_tizonia_conf.set('abs_top_builddir', meson.source_root() )
config_tizonia_conf.set('bindir', bindir)
config_tizonia_conf.set('datadir',datadir)
config_tizonia_conf.set('libdir', libdir)


configure_file(input: 'tizonia.conf.in',
               output: 'tizonia.conf',
               configuration: config_tizonia_conf,
               install: false
               )


# create check_tizonia.h
config_check_tizonia_h = configuration_data()
config_check_tizonia_h.set('abs_top_builddir', meson.source_root())

configure_file(input: 'check_tizonia.h.in',
               output: 'check_tizonia.h',
               configuration: config_check_tizonia_h,
               install: false
               )


check_tizonia_sources = [
   'check_tizonia.c'
]

check_tizonia = executable(
   'check_tizonia',
   check_tizonia_sources,
   dependencies: [
      check_dep,
      libtizgmusic_dep,
      libtizonia_dep
   ]
)

test('check_tizonia', check_tizonia)

# create tizonia.conf
config_tizonia_conf = configuration_data()
config_tizonia_conf.set('bindir', bindir)
config_tizonia_conf.set('datadir', bindir)
config_tizonia_conf.set('abs_top_builddir', meson.source_root())


configure_file(input: 'tizonia.conf.in',
               output: 'tizonia.conf',
               configuration: config_tizonia_conf
               )

# create check_tizcore.h
config_tizcore_h = configuration_data()
config_tizcore_h.set('abs_top_builddir', meson.source_root())

configure_file(input: 'check_tizcore.h.in',
               output: 'check_tizcore.h',
               configuration: config_tizcore_h 
               )


check_tizcore_sources = [
   'check_tizcore.c'
]

check_tizcore = executable(
   'check_tizcore',
   check_tizcore_sources,
   dependencies: [
      check_dep,
      libtizplatform_dep,
      libtizcore_dep,
      tizilheaders_dep
   ]
)

test('check_tizcore', check_tizcore)

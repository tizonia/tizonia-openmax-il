# create tizonia.conf
config_tizonia_conf = configuration_data()
config_tizonia_conf.set('abs_top_builddir', meson.source_root())
config_tizonia_conf.set('bindir', bindir)
config_tizonia_conf.set('datadir',datadir)
config_tizonia_conf.set('libdir', libdir)

configure_file(input: 'tizonia.conf.in',
               output: 'tizonia.conf',
               configuration: config_tizonia_conf,
               install: false
               )

# create gendb.sh
config_gendb_sh = configuration_data()
config_gendb_sh.set('abs_top_builddir', meson.source_root())

configure_file(input: 'gendb.sh.in',
               output: 'gendb.sh',
               configuration: config_gendb_sh,
               install: false
               )

# create check_tizrmproxy.h
config_check_tizrmproxy_h = configuration_data()
config_check_tizrmproxy_h.set('abs_top_builddir', meson.source_root())

check_tizrmproxy_h = configure_file(input: 'check_tizrmproxy.h.in',
               output: 'check_tizrmproxy.h',
               configuration: config_check_tizrmproxy_h,
               install: false
               )

# create updatedb.sh
config_updatedb_sh = configuration_data()
config_updatedb_sh.set('abs_top_builddir', meson.source_root())

configure_file(input: 'updatedb.sh.in',
               output: 'updatedb.sh',
               configuration: config_updatedb_sh,
               install: false
               )


check_tizrmproxy_sources = [
   'check_tizrmproxy.c'
]

check_tizrmproxy = executable(
   'check_tizrmproxy',
   check_tizrmproxy_sources,
   check_tizrmproxy_h,
   dependencies: [
      check_dep,
      tizilheaders_dep,
      libtizplatform_dep,
      libtizrmproxy_dep,
   ]
)

test('check_tizrmproxy', check_tizrmproxy)

###EXTRA_DIST = \
###	db_acquire_and_destroy_no_release.sql3 \
###	db_acquire_and_release.sql3 \
###	db_busy_resource_management.after.sql3 \
###	db_busy_resource_management.before.sql3 \
###	db_resource_preemption.after.sql3 \
###	db_resource_preemption.before.sql3 \
###	db_wait_cancel_wait.after.sql3 \
###	db_wait_cancel_wait.before.sql3

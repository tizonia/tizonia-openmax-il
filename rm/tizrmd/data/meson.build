# create tizonia-rm-db-dump.sh
config_db_dump = configuration_data()
config_db_dump.set('prefix', prefix)
config_db_dump.set('exec_prefix', prefix)
config_db_dump.set('bindir', bindir)
config_db_dump.set('datadir', datadir)

configure_file(input: 'tizonia-rm-db-dump.sh.in',
               output: 'tizonia-rm-db-dump.sh',
               configuration: config_db_dump,
               install_dir: bindir,
               install_mode: 'rwxr-xr-x'
               )


# create tizonia-rm-db-generate.sh
config_db_generate = configuration_data()
config_db_generate.set('prefix', prefix)
config_db_generate.set('exec_prefix', prefix)
config_db_generate.set('bindir', bindir)
config_db_generate.set('datadir', datadir)

tizonia_rm_db_generate_sh = configure_file(input: 'tizonia-rm-db-generate.sh.in',
               output: 'tizonia-rm-db-generate.sh',
               configuration: config_db_generate,
               install_dir: bindir,
               install_mode: 'rwxr-xr-x'
               )


install_data(
   'tizonia-rm-db-initial.sql3',
   install_dir: tizdatadir
)


tizrm_db = custom_target(
         'tizrm.db',
         build_by_default: true,
         input: 'tizonia-rm-db-initial.sql3',
         output: 'tizrm.db', 
         command: ['sh', tizonia_rm_db_generate_sh, '@INPUT@', '@OUTPUT@'],
         install: true,
         install_dir: tizdatadir
)

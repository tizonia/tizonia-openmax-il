libcurl_dep = dependency('libcurl', required: true, version: '>=7.18.0')

# create tizplatform_config.h
tizplatform_config_h = configuration_data()
tizplatform_config_h.set('sysconfdir', sysconfdir)

configure_file(input: 'tizplatform_config.h.in',
               output: 'tizplatform_config.h',
               configuration: tizplatform_config_h
               )


libtizplatform_sources = [
   'http-parser/http_parser.c',
   'avl/avl.c',
   'tizplatform.c',
   'tizlog.c',
   'tizomxutils.c',
   'tizmem.c',
   'tizsync.c',
   'tizqueue.c',
   'tizpqueue.c',
   'tizbuffer.c',
   'tizvector.c',
   'tizthread.c',
   'tizuuid.c',
   'tizrc.c',
   'tizsoa.c',
   'tizev.c',
   'tizmap.c',
   'tizhttp.c',
   'tizlimits.c',
   'tizprintf.c',
   'tizshufflelst.c',
   'tizurltransfer.c'
]

install_headers(
   'utarray/utarray.h',
   'avl/avl.h',
   'http-parser/http_parser.h',
   'tizplatform.h',
   'tizmacros.h',
   'tizplatform_internal.h',
   'tizlog.h',
   'tizomxutils.h',
   'tizmem.h',
   'tizpqueue.h',
   'tizqueue.h',
   'tizsync.h',
   'tizbuffer.h',
   'tizvector.h',
   'tizthread.h',
   'tizuuid.h',
   'tizrc.h',
   'tizsoa.h',
   'tizev.h',
   'tizmap.h',
   'tizhttp.h',
   'tizlimits.h',
   'tizprintf.h',
   'tizshufflelst.h',
   'tizurltransfer.h',
   install_dir: tizincludedir
)

libtizplatform_deps = [
   tizilheaders_dep,
   libcurl_dep,
   pthread_dep,
   uuid_dep,
   log4c_dep
]

if have_system_libev
   libtizplatform_sources += [
      'ev/ev.c'
   ]
   install_headers(
      'ev/ev.h',
      'ev/ev_vars.h',
      'ev/ev_wrap.h',
      'ev/ev_epoll.c',
      install_dir: tizincludedir
   )
   libtizplatform_deps += [
      libev_dep
   ]
endif



libtizplatform = library(
   'tizplatform',
   version: tizversion,
   sources: libtizplatform_sources,
   dependencies: libtizplatform_deps,
   install: true
)

libtizplatform_dep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: libtizplatform,
    dependencies: pthread_dep
)

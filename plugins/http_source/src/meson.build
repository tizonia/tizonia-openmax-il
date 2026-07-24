libtizhttpsrc_sources = [
   'httpsrc.c',
   'httpsrcport.c',
   'httpsrcprc.c',
   'gmusicprc.c',
   'gmusiccfgport.c',
   'youtubeprc.c',
   'youtubecfgport.c',
   'plexprc.c',
   'plexcfgport.c',
   'tuneinprc.c',
   'tuneincfgport.c',
   'iheartprc.c',
   'iheartcfgport.c'
]

libtizhttpsrc_deps = [
   libtizonia_dep,
   libtizgmusic_dep,
   libtizyoutube_dep,
   libtizplex_dep,
   libtiztunein_dep,
   libtiziheart_dep
]

if enable_soundcloud
   libtizhttpsrc_sources += [
      'scloudprc.c',
      'scloudcfgport.c'
   ]
   libtizhttpsrc_deps += [libtizsoundcloud_dep]
endif

libtizhttpsrc = library(
   'tizhttpsrc',
   version: tizversion,
   sources: libtizhttpsrc_sources,
   dependencies: libtizhttpsrc_deps,
   install: true,
   install_dir: tizplugindir
)

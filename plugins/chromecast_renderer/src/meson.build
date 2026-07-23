libtizchromecastrnd_sources = [
   'chromecastrnd.c',
   'cc_prc.c',
   'cc_cfgport.c',
   'cc_httpprc.c',
   'cc_gmusicprc.c',
   'cc_gmusiccfgport.c',
   'cc_tuneinprc.c',
   'cc_tuneincfgport.c',
   'cc_youtubeprc.c',
   'cc_youtubecfgport.c',
   'cc_plexprc.c',
   'cc_plexcfgport.c',
   'cc_iheartprc.c',
   'cc_iheartcfgport.c'
]

libtizchromecastrnd_deps = [
   libtizonia_dep,
   libtizcastclient_dep,
   libtizgmusic_dep,
   libtizplex_dep,
   libtizyoutube_dep,
   libtiztunein_dep,
   libtiziheart_dep
]

if enable_soundcloud
   libtizchromecastrnd_sources += [
      'cc_scloudprc.c',
      'cc_scloudcfgport.c'
   ]
   libtizchromecastrnd_deps += [libtizsoundcloud_dep]
endif

libtizchromecastrnd = library(
   'tizchromecastrnd',
   version: tizversion,
   sources: libtizchromecastrnd_sources,
   dependencies: libtizchromecastrnd_deps,
   install: true,
   install_dir: tizplugindir
)

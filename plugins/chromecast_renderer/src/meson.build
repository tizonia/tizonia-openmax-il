libtizchromecastrnd_sources = [
   'chromecastrnd.c',
   'cc_prc.c',
   'cc_cfgport.c',
   'cc_httpprc.c',
   'cc_gmusicprc.c',
   'cc_gmusiccfgport.c',
   'cc_scloudprc.c',
   'cc_scloudcfgport.c',
   'cc_tuneinprc.c',
   'cc_tuneincfgport.c',
   'cc_youtubeprc.c',
   'cc_youtubecfgport.c',
   'cc_plexprc.c',
   'cc_plexcfgport.c'
]

libtizchromecastrnd = library(
   'tizchromecastrnd',
   version: tizversion,
   sources: libtizchromecastrnd_sources,
   dependencies: [
      libtizonia_dep,
      libtizcastclient_dep,
      libtizgmusic_dep,
      libtizplex_dep,
      libtizyoutube_dep,
      libtizsoundcloud_dep,
      libtiztunein_dep
   ],
   install: true,
   install_dir: tizplugindir
)

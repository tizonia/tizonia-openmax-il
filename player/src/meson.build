if enable_libspotify
   install_headers(
      'services/spotify/tizspotifygraph.hpp',
      'services/spotify/tizspotifygraphfsm.hpp',
      'services/spotify/tizspotifygraphops.hpp',
      'services/spotify/tizspotifyconfig.hpp',
      'services/spotify/tizspotifymgr.hpp',
      install_dir: tizincludedir
   )
endif

if enable_libspotify
   spotify_sources = [
      'services/spotify/tizspotifygraph.cpp',
      'services/spotify/tizspotifygraphfsm.cpp',
      'services/spotify/tizspotifygraphops.cpp',
      'services/spotify/tizspotifymgr.cpp'
   ]
else
   spotify_sources = []
endif

tizonia_sources = [
   'main.cpp',
   'tizplayapp.cpp',
   'tizprogramopts.cpp',
   'tizomxutil.cpp',
   'tizgraphutil.cpp',
   'tizgraphcback.cpp',
   'tizdaemon.cpp',
   'tizprobe.cpp',
   'tizplaylist.cpp',
   'tizgraphfactory.cpp',
   'tizgraphmgrcmd.cpp',
   'tizgraphmgrops.cpp',
   'tizgraphmgrfsm.cpp',
   'tizgraphmgr.cpp',
   'tizgraphfsm.cpp',
   'tizgraphops.cpp',
   'tizgraphcmd.cpp',
   'tizgraph.cpp',
   'decoders/tizdecgraphmgr.cpp',
   'decoders/tizdecgraph.cpp',
   'decoders/tizmp3graph.cpp',
   'decoders/tizaacgraph.cpp',
   'decoders/tizopusgraph.cpp',
   'decoders/tizoggopusgraph.cpp',
   'decoders/tizvorbisgraph.cpp',
   'decoders/tizflacgraph.cpp',
   'decoders/tizoggflacgraph.cpp',
   'decoders/tizpcmgraph.cpp',
   'decoders/tizmpeggraph.cpp',
   'httpserv/tizhttpservmgr.cpp',
   'httpserv/tizhttpservgraph.cpp',
   'httpserv/tizhttpservgraphfsm.cpp',
   'httpserv/tizhttpservgraphops.cpp',
   'httpclnt/tizhttpclntmgr.cpp',
   'httpclnt/tizhttpclntgraph.cpp',
   'httpclnt/tizhttpclntgraphfsm.cpp',
   'httpclnt/tizhttpclntgraphops.cpp',
   'services/tizservicegraph.cpp',
   'services/tizservicegraphfsm.cpp',
   spotify_sources,
   'services/googlemusic/tizgmusicgraph.cpp',
   'services/googlemusic/tizgmusicgraphops.cpp',
   'services/googlemusic/tizgmusicmgr.cpp',
   'services/soundcloud/tizscloudgraph.cpp',
   'services/soundcloud/tizscloudgraphops.cpp',
   'services/soundcloud/tizscloudmgr.cpp',
   'services/youtube/tizyoutubegraph.cpp',
   'services/youtube/tizyoutubegraphfsm.cpp',
   'services/youtube/tizyoutubegraphops.cpp',
   'services/youtube/tizyoutubemgr.cpp',
   'services/plex/tizplexgraph.cpp',
   'services/plex/tizplexgraphops.cpp',
   'services/plex/tizplexmgr.cpp',
   'services/tunein/tiztuneingraph.cpp',
   'services/tunein/tiztuneingraphfsm.cpp',
   'services/tunein/tiztuneingraphops.cpp',
   'services/tunein/tiztuneinmgr.cpp',
   'services/chromecast/tizchromecastgraph.cpp',
   'services/chromecast/tizchromecastgraphfsm.cpp',
   'services/chromecast/tizchromecastgraphops.cpp',
   'services/chromecast/tizchromecastmgr.cpp',
   'tizplaybackevents.cpp',
   'tizprogressdisplay.cpp',
   'mpris/tizmprismgr.cpp',
   'mpris/tizmprisprops.cpp',
   'mpris/tizmprisif.cpp'
]


tizonia_inc = [
   include_directories('services'),
   include_directories('services/googlemusic'),
   include_directories('services/soundcloud'),
   include_directories('services/youtube'),
   include_directories('services/plex'),
   include_directories('services/tunein')
]

tizonia = executable(
   'tizonia',
   sources: [tizonia_sources, mpris_dbus_hpp],
   include_directories: tizonia_inc,
   dependencies: [
      tizilheaders_dep,
      libtizcore_dep,
      libtizdbus_cpp_dep,
      libtizplatform_dep,
      taglib_dep,
      libmediainfo_dep,
      boost_dep
   ],
   install: true
)

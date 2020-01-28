if enabled_clients.contains('gmusic')
   subdir('gmusic')
endif
if enabled_clients.contains('soundcloud')
   subdir('soundcloud')
endif
if enabled_clients.contains('youtube')
   subdir('youtube')
endif
if enabled_clients.contains('plex')
   subdir('plex')
endif
if enabled_clients.contains('chromecast')
   subdir('chromecast')
endif
if enabled_clients.contains('spotify')
   subdir('spotify')
endif
# trying it in a different way
###if enable_libspotify
###   subdir('spotify')
###endif
if enabled_clients.contains('tunein')
   subdir('tunein')
endif

meson.add_install_script('meson_post_install.py', python3.get_install_dir())


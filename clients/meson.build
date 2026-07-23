subdir('gmusic')
if enable_soundcloud
   subdir('soundcloud')
endif
subdir('youtube')
subdir('plex')
subdir('chromecast')
if enable_libspotify
   subdir('spotify')
endif
subdir('tunein')
subdir('iheart')

meson.add_install_script('meson_post_install.py', python3.get_install_dir())

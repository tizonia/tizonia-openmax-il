subdir('gmusic')
subdir('soundcloud')
subdir('youtube')
subdir('plex')
subdir('chromecast')
if enable_libspotify
   subdir('spotify')
endif
subdir('tunein')

meson.add_install_script('meson_post_install.py', python3.get_install_dir())


if enabled_plugins.contains('mpeg_audio_decoder') or enabled_plugins.contains('mp3_metadata')
   mpg123_dep = dependency('libmpg123', required: true, version: '>=1.16.0')
endif

if enabled_plugins.contains('ogg_demuxer') or enabled_plugins.contains('ogg_muxer')
   oggz_dep = dependency('oggz', required: true, version: '>=1.1.1')
endif

if enable_clients and enabled_plugins.contains('chromecast_renderer')
   subdir('chromecast_renderer')
endif

if enabled_plugins.contains('file_reader')
   subdir('file_reader')
endif

if enabled_plugins.contains('file_writer')
   subdir('file_writer')
endif

if enabled_plugins.contains('flac_decoder')
   subdir('flac_decoder')
endif

if enabled_plugins.contains('http_renderer')
   subdir('http_renderer')
endif

if enable_clients and enabled_plugins.contains('http_source')
   subdir('http_source')
endif

if enabled_plugins.contains('mp3_decoder')
   subdir('mp3_decoder')
endif

if enabled_plugins.contains('mp3_encoder')
   subdir('mp3_encoder')
endif

if enabled_plugins.contains('mp3_metadata')
   subdir('mp3_metadata')
endif

if enabled_plugins.contains('mpeg_audio_decoder')
   subdir('mpeg_audio_decoder')
endif

if enabled_plugins.contains('ogg_demuxer')
   subdir('ogg_demuxer')
endif

if enabled_plugins.contains('ogg_muxer')
   subdir('ogg_muxer')
endif

if enabled_plugins.contains('opus_decoder')
   subdir('opus_decoder')
endif

if enabled_plugins.contains('opusfile_decoder')
   subdir('opusfile_decoder')
endif

if enabled_plugins.contains('pcm_decoder')
   subdir('pcm_decoder')
endif

if enabled_plugins.contains('pcm_renderer_pa')
   subdir('pcm_renderer_pa')
endif

if enabled_plugins.contains('vorbis_decoder')
   subdir('vorbis_decoder')
endif

if enabled_plugins.contains('vp8_decoder')
   subdir('vp8_decoder')
endif

if enabled_plugins.contains('webm_demuxer')
   subdir('webm_demuxer')
endif

if enabled_plugins.contains('yuv_renderer')
   subdir('yuv_renderer')
endif

if enable_libspotify
   subdir('spotify_source')
endif

if enable_alsa
   subdir('pcm_renderer_alsa')
endif

if enable_aac
   subdir('aac_decoder')
endif

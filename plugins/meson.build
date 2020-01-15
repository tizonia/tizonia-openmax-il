mpg123_dep = dependency('libmpg123', required: true, version: '>=1.16.0')
oggz_dep = dependency('oggz', required: true, version: '>=1.1.1')

subdir('chromecast_renderer')
subdir('file_reader')
subdir('file_writer')
subdir('flac_decoder')
subdir('http_renderer')
subdir('http_source')
subdir('mp3_decoder')
subdir('mp3_encoder')
subdir('mp3_metadata')
subdir('mpeg_audio_decoder')
subdir('ogg_demuxer')
subdir('ogg_muxer')
subdir('opus_decoder')
subdir('opusfile_decoder')
subdir('pcm_decoder')
subdir('pcm_renderer_pa')
subdir('vorbis_decoder')
subdir('vp8_decoder')
subdir('webm_demuxer')
subdir('yuv_renderer')

if enable_libspotify
   subdir('spotify_source')
endif

if enable_alsa
   subdir('pcm_renderer_alsa')
endif

if enable_aac
   subdir('aac_decoder')
endif

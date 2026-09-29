playback_smoke_plugin_paths = [
   join_paths(meson.build_root(), 'plugins', 'file_reader', 'src'),
   join_paths(meson.build_root(), 'plugins', 'flac_decoder', 'src'),
   join_paths(meson.build_root(), 'plugins', 'http_source', 'src'),
   join_paths(meson.build_root(), 'plugins', 'mp3_decoder', 'src'),
   join_paths(meson.build_root(), 'plugins', 'ogg_demuxer', 'src'),
   join_paths(meson.build_root(), 'plugins', 'opus_decoder', 'src'),
   join_paths(meson.build_root(), 'plugins', 'opusfile_decoder', 'src'),
   join_paths(meson.build_root(), 'plugins', 'pcm_decoder', 'src'),
   join_paths(meson.build_root(), 'plugins', 'pcm_renderer_alsa', 'src'),
   join_paths(meson.build_root(), 'plugins', 'pcm_renderer_pa', 'src'),
   join_paths(meson.build_root(), 'plugins', 'vorbis_decoder', 'src'),
]

playback_smoke_config = configuration_data()
playback_smoke_config.set('player', join_paths(meson.build_root(), 'player', 'src', 'tizonia'))
playback_smoke_config.set('plugin_paths', ';'.join(playback_smoke_plugin_paths))

playback_smoke = configure_file(
   input: 'playback_smoke.py.in',
   output: 'playback_smoke.py',
   configuration: playback_smoke_config,
)

test(
   'playback-smoke',
   python3,
   args: playback_smoke,
   suite: 'playback-smoke',
   timeout: 180,
)

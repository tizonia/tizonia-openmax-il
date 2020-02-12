OpenMAX IL Options
==================

The section describes a number of options that are more typically used during
development. These command-line options allow querying the OpenMAX IL subsystem
and learn about the installed OpenMAX IL components and their supported roles.

OPTIONS
-------

``-L [ --comp-list ]``
    Enumerate all the OpenMAX IL components in the system.

``-R [ --roles-of-comp ] arg``
    Display the OpenMAX IL roles found in component <arg>.

``-C [ --comps-of-role ] arg``
    Display the OpenMAX IL components that implement role <arg>.

EXAMPLES
--------

.. code-block:: bash

   $ tizonia --comp-list
   This software is part of the Tizonia project <https://tizonia.org>

   Component at index [0] -> [OMX.Aratelia.audio_decoder.opus]
   Component at index [1] -> [OMX.Aratelia.audio_decoder.aac]
   Component at index [2] -> [OMX.Aratelia.audio_renderer.pulseaudio.pcm]
   Component at index [3] -> [OMX.Aratelia.audio_decoder.mp3]
   Component at index [4] -> [OMX.Aratelia.file_reader.binary]
   Component at index [5] -> [OMX.Aratelia.audio_renderer.http]
   Component at index [6] -> [OMX.Aratelia.container_demuxer.webm]
   Component at index [7] -> [OMX.Aratelia.file_writer.binary]
   Component at index [8] -> [OMX.Aratelia.iv_renderer.yuv.overlay]
   Component at index [9] -> [OMX.Aratelia.audio_renderer.chromecast]
   Component at index [10] -> [OMX.Aratelia.audio_decoder.mpeg]
   Component at index [11] -> [OMX.Aratelia.audio_renderer.alsa.pcm]
   Component at index [12] -> [OMX.Aratelia.container_demuxer.ogg]
   Component at index [13] -> [OMX.Aratelia.audio_encoder.mp3]
   Component at index [14] -> [OMX.Aratelia.audio_metadata_eraser.mp3]
   Component at index [15] -> [OMX.Aratelia.tizonia.test_component]
   Component at index [16] -> [OMX.Aratelia.container_muxer.ogg]
   Component at index [17] -> [OMX.Aratelia.video_decoder.vp8]
   Component at index [18] -> [OMX.Aratelia.audio_decoder.pcm]
   Component at index [19] -> [OMX.Aratelia.audio_source.spotify.pcm]
   Component at index [20] -> [OMX.Aratelia.ilcore.test_component]
   Component at index [21] -> [OMX.Aratelia.audio_decoder.opusfile.opus]
   Component at index [22] -> [OMX.Aratelia.audio_decoder.vorbis]
   Component at index [23] -> [OMX.Aratelia.audio_source.http]
   Component at index [24] -> [OMX.Aratelia.audio_decoder.flac]


   $ tizonia --roles-of-comp OMX.Aratelia.audio_renderer.chromecast
   This software is part of the Tizonia project <https://tizonia.org>

   Component [OMX.Aratelia.audio_renderer.chromecast] : role #0 -> [audio_renderer.chromecast]
   Component [OMX.Aratelia.audio_renderer.chromecast] : role #1 -> [audio_renderer.chromecast.gmusic]
   Component [OMX.Aratelia.audio_renderer.chromecast] : role #2 -> [audio_renderer.chromecast.scloud]
   Component [OMX.Aratelia.audio_renderer.chromecast] : role #3 -> [audio_renderer.chromecast.tunein]
   Component [OMX.Aratelia.audio_renderer.chromecast] : role #4 -> [audio_renderer.chromecast.youtube]
   Component [OMX.Aratelia.audio_renderer.chromecast] : role #5 -> [audio_renderer.chromecast.plex]


   $ tizonia --comps-of-role audio_renderer.chromecast.gmusic
   This software is part of the Tizonia project <https://tizonia.org>

   Role [audio_renderer.chromecast.gmusic] found in [OMX.Aratelia.audio_renderer.chromecast]

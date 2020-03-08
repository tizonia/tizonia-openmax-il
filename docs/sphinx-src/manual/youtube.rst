YouTube
=======

Streaming from YouTube is possible without credentials. Simply use one of the
command-line options provided.

CONFIGURATION
-------------

The Tizonia configuration file contains a block under the ``[tizonia]`` section
like the one pictured below (see also :ref:`tizonia-config-label`).

.. code-block:: bash

   # YouTube configuration
   # -------------------------------------------------------------------------
   # To avoid passing this information on the command line, uncomment and
   # configure as needed.
   #
   # youtube.api_key = youtube api key. Optional but RECOMMENDED to avoid
   #                   problems when the daily quota of the interal api key
   #                   is exceeded. To create your YouTube api key, see info at:
   #                   https://www.slickremix.com/docs/get-api-key-for-youtube/
   # youtube.buffer_seconds = size of the audio buffer (in seconds) to use
   #                          while downloading streams. Default: 60.
   #                          Increase in case of cuts.

``youtube.buffer_seconds``
  This is the minimum size of the audio buffer (in seconds) that Tizonia will
  use while downloading the audio streams. It may be increased in case of
  cuts, but usually not required. Default: 60.


OPTIONS
-------

``--youtube-audio-stream arg``
    Play a YouTube audio stream from a video url or video id.

``--youtube-audio-playlist arg``
    Play a YouTube audio playlist from a playlist url or playlist id.

``--youtube-audio-mix arg``
    Play a YouTube mix from a video url or video id.

``--youtube-audio-search arg``
    Search and play YouTube audio streams.

``--youtube-audio-mix-search arg``
    Play a YouTube mix from a search term.

``--youtube-audio-channel-uploads arg``
    Play all videos uploaded to a YouTube channel (arg = channel url or name).

``--youtube-audio-channel-playlist arg``
    Play a playlist from particular YouTube channel (arg = '<channel-name[space]play list-name>').


EXAMPLES
--------

.. code-block:: bash

   $ tizonia --youtube-audio-mix-search 'the final countdown'

   $ tizonia --youtube-audio-channel-playlist 'UCGJdzJQ3R1BpahSvcFq23HA masters of metal'

   $ tizonia --youtube-audio-channel-uploads 'UCGJdzJQ3R1BpahSvcFq23HA'

   $ tizonia --youtube-audio-stream v2AC41dglnM

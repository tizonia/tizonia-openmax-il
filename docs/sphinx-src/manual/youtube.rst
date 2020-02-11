YouTube
=======

It is not necessary to add any credentials to `tizonia.conf`` to stream audio
from YouTube.

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
   # youtube.buffer_seconds = size of the audio buffer (in seconds) to use
   #                          while downloading streams. Default: 60.
   #                          Increase in case of cuts.


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

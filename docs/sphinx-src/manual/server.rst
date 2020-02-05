Shoutcast Server
================

Tizonia can act as a simplified Icecast/SHOUTcast LAN streaming server. In this
mode, Tizonia can stream local mp3 files to a single client. This is useful to,
for example, stream a collection of ``mp3`` files to a client application
running on another machine or on a mobile phone within your home.

OPTIONS
-------

``--server``
    Stream media files using the SHOUTcast/ICEcast streaming protocol.

``-p [ --port ] arg``
    TCP port to be used for Icecast/SHOUTcast streaming. Optional. Default: 8010.

``--station-name arg``
    The Icecast/SHOUTcast station name. Optional.

``--station-genre arg``
    The Icecast/SHOUTcast station genre. Optional.

``--no-icy-metadata``
    Disables Icecast/SHOUTcast metadata in the stream. Optional.

``--bitrate-modes arg``
    A comma-separated list of bitrate modes (e.g. 'CBR,VBR'). Only media with these bitrate modes will be streamed. Optional. Default: any.

``--sampling-rates arg``
    A comma-separated list of sampling rates. Only media with these rates will be streamed. Optional. Default: any.

EXAMPLES
--------

.. code-block:: bash

   $ tizonia --server --recurse --shuffle $HOME/Music
   This software is part of the Tizonia project <https://tizonia.org>

   [Tizonia Radio]: Server streaming on http://galactus:8010
   [Tizonia Radio]: Streaming media with sampling rates [ANY].
   [Tizonia Radio]: Streaming media with bitrate modes [CBR,VBR].

   Playlist length: 954. File extensions in playlist: .mp3
   [http/mp3] [server] : '/home/joni/Music/The Official Uk Top 40 Singles Chart/Iggy Azalea Feat Charli XCX - Fancy
      Fancy (Explicit Edit), Iggy Azalea Feat Charli XCX
        Track # : 20
        Album : The Official Uk Top 40 Singles Chart
        Year : 2014
        Duration : 3m:20s
        Size : 7.7 MiB
        Genre : Top 40
        Comment : Some music
        2 Ch, 44.1 KHz, 320 Kbps

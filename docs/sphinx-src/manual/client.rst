Shoutcast Client
================

Tizonia can act as client of Icecast/SHOUTcast streaming servers. In this mode,
Tizonia can stream Internet radio stations from Internet servers or from
another instance of Tizonia acting as a streaming server in the same LAN. The
supported media formats while operating in this mode are ``mp3``, ``aac``,
``flac``, ``ogg`` and ``opus``.

OPTIONS
-------

``--station-id arg``
    Give a name/id to the remote stream. Optional.

EXAMPLES
--------

.. code-block:: bash

   $ tizonia http://galactus:8010
   This software is part of the Tizonia project <https://tizonia.org>

   [http] [Connecting to radio station] : 'http://galactus:8010'.
        Content-Type : audio/mpeg
        icy-br : 320
        ice-audio-info : bitrate=320000;channels=2;samplerate=44100
        icy-name : Tizonia Radio (galactus:8010)
        icy-description : Tizonia Streaming Server
        icy-genre : Unknown Genre
        icy-url : https://tizonia.org
        icy-pub : 0
        Server : Tizonia HTTP Renderer 0.19.0
        Cache-Control : no-cache
   [http/mp3] [Connected] : 'http://galactus:8010'.
        2 Ch, 44.1 KHz, 16:s:b


   $ tizonia http://vis.media-ice.musicradio.com/LBCUKMP3Low
   This software is part of the Tizonia project <https://tizonia.org>

   [http] [Connecting to radio station] : 'http://vis.media-ice.musicradio.com/LBCUKMP3Low'.
        Accept-Ranges : none
        Content-Type : audio/mpeg
        icy-br : 48
        ice-audio-info : ice-samplerate=44100;ice-bitrate=48;ice-channels=1
        icy-br : 48
        icy-description : LBC UK
        icy-genre : Talk
        icy-name : LBC UK
        icy-private : 0
        icy-pub : 1
        Server : Icecast 2.3.3-kh11
        Cache-Control : no-cache, no-store
        Pragma : no-cache
        Access-Control-Allow-Origin : *
        Access-Control-Allow-Headers : Origin, Accept, X-Requested-With, Content-Type
        Access-Control-Allow-Methods : GET, OPTIONS, HEAD
        Connection : close
        Expires : Mon, 26 Jul 1997 05:00:00 GMT
   [http/mp3] [Connected] : 'http://vis.media-ice.musicradio.com/LBCUKMP3Low'.
        1 Ch, 44.1 KHz, 16:s:b

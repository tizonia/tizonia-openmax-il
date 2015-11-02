.. Tizonia documentation


Introduction
============

The 500-foot view
-----------------

**Tizonia** is an open source music player and audio streaming client/server
for Linux. It features support for Spotify and Google Play Music and is based
on its own multimedia framework (OpenMAX IL 1.2-based).


The project's current features
------------------------------

* Simple, no-fuss command-line music player utility.
* Cloud streaming audio services:

  * *Spotify and Google Play Music currently supported. SoundCloud to be added soon.*

* Local media playback with support for most of the common high-quality audio formats:

  * *Formats currently supported: mp3, mp2, mpa, m2a, aac, vorbis, opus, wav,
    aiff, and flac*.

* Support for traditional Icecast/SHOUTcast radio streams:

  * mp3, aac, and opus stations (ogg to be added soon).

* LAN audio streaming server (Icecast/SHOUTcast):

  * *Currently: mp3 stations can be created from local media (flac and opus to
    be added in the near future).*

* MPRIS v2 remote control interface.

* Based on Tizonia's own internal multimedia framework.

  * A full implementation of the OpenMAX IL 1.2 provisional specification:
  * No ffmpeg, libav, gstreamer or libvlc dependencies.


Future project's goals
----------------------

* LAN streaming (Icecast/SHOUTcast) of cloud streams.
* Multi-room synchronised playback.
* Zeroconf/AVAHI networking.
* Music Player Daemon protocol support.

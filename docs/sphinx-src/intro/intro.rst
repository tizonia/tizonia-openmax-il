.. Tizonia documentation


Introduction
============

500-foot view
-------------

Tizonia is an open-source sofware project built around a multimedia framework
based on OpenMAX IL. `OpenMAX IL <https://en.wikipedia.org/wiki/OpenMAX>`_ is
an open standard from `The Khronos Group <http://www.khronos.org/openmax/>`_
that enables the creation and integration of software and hardware accelerated
media streaming components like audio and video encoders and decoders, camera
components, and media processing algorithms.

In addition to the OpenMAX IL-based multimedia framework, the Tizonia project
features a command-line music player and audio streaming client/server for
Linux with support for Spotify, Google Play Music and SoundCloud cloud
streaming services.


Current features
----------------

* Simple, no-fuss command-line music player utility.
* Cloud audio streaming services:

  * *Spotify Premium, Google Play Unlimited, and SoundCloud.*

* Support for most common high-quality audio formats from local media:

  * *Formats currently supported: mp3, mp2, mpa, m2a, aac, vorbis, opus, wav,
    aiff, and flac*.

* Playback of Icecast/SHOUTcast radio streams:

  * mp3, aac, and opus stations (ogg to be added soon).

* LAN audio streaming server (Icecast/SHOUTcast):

  * *Currently: mp3 stations can be created from local media (flac and opus to
    be added in the near future).*

* MPRIS v2 remote control interface.

* Based on Tizonia's own internal multimedia framework.

  * A full implementation of the OpenMAX IL 1.2 provisional
    specification. I.e. no ffmpeg, libav, gstreamer or libvlc dependencies.


Future goals
------------

* LAN streaming (Icecast/SHOUTcast) of cloud streams.
* Additional cloud audio streaming services (e.g. Dirble, YouTube, etc.).
* Multi-room synchronised playback.
* Zeroconf/AVAHI networking.
* Music Player Daemon protocol support.
* Video streaming features.

.. Tizonia documentation


Introduction
============

500-foot view
-------------

Tizonia is a free and open-source multimedia software project based on
OpenMAX IL.

`OpenMAX IL <https://en.wikipedia.org/wiki/OpenMAX>`_ is an open standard
maintained by the `The Khronos Group <http://www.khronos.org/openmax/>`_ that
enables the creation and integration of software and hardware accelerated media
streaming components like audio and video encoders and decoders, camera
components, and media processing algorithms.

Tizonia provides a number of libraries that implement the various features and
components of the OpenMAX IL API, including the OpenMAX IL Core, the OpenMAX IL
plugin infrastructure, Resource Management and a number of plugin
implementations (primarily audio and video software decoders, and media sources
and renderers).

From a user-side perspective, the Tizonia project also features a command-line
music player and audio streaming client/server for Linux with support for
Spotify, Google Play Music and SoundCloud on-demand audio streaming services.

Tizonia development is hosted on `GitHub
<https://github.com/tizonia/tizonia-openmax-il>`_ and software binary releases
are available for Debian-based Linux distributions from `Bintray
<https://bintray.com/tizonia>`_.


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

  * A full implementation of the OpenMAX IL 1.2 provisional specification.

  * No ffmpeg, libav, gstreamer or libvlc dependencies.


Roadmap
-------

* LAN streaming (Icecast/SHOUTcast) of any audio stream (including cloud
  services).
* Integration of additional audio streaming services (e.g. Dirble, YouTube,
  etc.).
* Multi-room synchronised playback.
* Zeroconf/AVAHI networking.
* Music Player Daemon protocol support.
* Hardware-acceleration and Video support.

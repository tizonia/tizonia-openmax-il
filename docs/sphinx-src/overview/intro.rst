.. Tizonia documentation


The Tizonia Project
===================

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
Spotify, Google Play Music, YouTube, SoundCloud, and Dirble on-demand streaming
services.

Tizonia development is hosted on `GitHub
<https://github.com/tizonia/tizonia-openmax-il>`_ and software binary releases
are available for Debian-based Linux distributions from `Bintray
<https://bintray.com/tizonia>`_. There are AUR (Arch User Repository) packages
avaialable as well.


Current features
----------------

* Simple, no-fuss command-line music player utility.
* Audio streaming from cloud services:

  * *Spotify Premium, Google Play Unlimited, YouTube, SoundCloud, and Dirble.*

* Audio playback from local media, with support for most common high-quality
  audio formats:

  * *Formats currently supported: mp3, mp2, mpa, m2a, aac, vorbis, opus, wav,
    aiff, and flac*.

* Playback of Icecast/SHOUTcast radio streams:

  * mp3, aac, and opus stations supported.

* Icecast/SHOUTcast LAN audio streaming server (*experimental*):

  * *Currently: stations can be created from local mp3 media.*

* MPRIS v2 remote control interface with support for previous, next, pause,
  play, stop and volume commands.

* Tizonia has its own internal multimedia framework.

  * A full implementation of the OpenMAX IL 1.2 provisional specification.

  * Therefore, with no depedencies on ffmpeg, libav, gstreamer or libvlc
    frameworks.


Roadmap
-------

* Chromecast support.
* Integration of additional audio streaming services (e.g. Pandora, Deezer,
  etc.).
* LAN streaming (Icecast/SHOUTcast) of any audio stream (including cloud
  services).
* Multi-room synchronised playback (via Chromecast).
* Hardware-acceleration and video support.

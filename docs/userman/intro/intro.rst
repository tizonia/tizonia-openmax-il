.. Tizonia documentation


Introduction
============

**Tizonia** is an open source music player and audio streaming client/server
for Linux. It features support for Spotify and it is based on its own
multimedia framework based on `OpenMAX IL 1.2 Provisional Specification
<http://www.khronos.org/news/press/khronos-group-releases-openmax-il-1.2-provisional-specification>`_.

Some of the project current features are:

* Simple, no-fuss command-line music player utility:
  * Support for the most common high-quality audio formats:
    * Formats currently supported: mp3, mp2, mpa, m2a, aac, vorbis, opus, wav, aiff, and flac.
  * With client support for traditional Icecast/SHOUTcast radio stations as
    well as cloud streaming audio services:
    * Spotify currently supported. SoundCloud and Google Play Music to come soon.
  * Audio streaming server capabilities: Icecast/SHOUTcast streaming
    server:
    * Currently: mp3 stations only, flac and opus to be added in the near future.
  * Based on Tizonia's own internal multimedia framework. No ffmpeg, libav,
    gstreamer or libvlc dependencies.
  * MPRIS v2 remote control interface.

* A full implementation of the OpenMAX IL 1.2 provisional specification:
  * Alignment to the 1.2 specification, with full support for both *Base* and
  *Interop* profiles, including tunnelling and resource management.
  * Support for the new 1.2 features and updates:
    * Support for arbitrary buffers (i.e. dynamically allocated vs statically
      allocated as in 1.1.2)
    * Updated state machine, including changes to eliminate potential race
      conditions during tunnelling.
    * Ability to cancel certain pending state transitions.
    * Improved support for component roles.
    * Updated error codes and events.
    * Slaving behaviour of ports.
    * Improved port compatibility checks.
    * IL Core updates, including support for IL Core extensions and explicit
      tunnel tear-down API.

Some of the project's future goals are:

* Music Player Daemon protocol support.
* Zeroconf/AVAHI networking.
* Multi-room synchronised playback (ala Logitech Media Server).

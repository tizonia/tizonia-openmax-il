# Project Overview

#### Table Of Contents

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->


- [`tizonia`: A command-line cloud music player and audio streaming client/server.](#tizonia-a-command-line-cloud-music-player-and-audio-streaming-clientserver)
- [A fully-featured OpenMAX IL 1.2 multimedia framework](#a-fully-featured-openmax-il-12-multimedia-framework)
- [Tizonia's OpenMAX IL 1.2 plugins](#tizonias-openmax-il-12-plugins)
- [Skema: Tizonia's test execution framework for OpenMAX IL components.](#skema-tizonias-test-execution-framework-for-openmax-il-components)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## `tizonia`: A command-line cloud music player and audio streaming client/server.

* Stream playlists from Spotify (Spotify Premium required).
* Search and stream audio from Google Play Music (free and paid tiers).
* Search and stream audio from YouTube.
* Search and stream audio from SoundCloud.
* Search and stream Internet radio stations with Dirble.
* Search and stream audio from Plex servers.
* Cast cloud music to your Chromecast devices.
* Playback of local media files (mp3, mp2, mpa, m2a, aac, ogg/vorbis, opus,
  wav, aiff, and flac).
* Icecast/SHOUTcast streaming LAN server (mp3) (*experimental*).
* Icecast/SHOUTcast streaming client (mp3, aac, and opus).
* Daemon and command line modes (no GUI).
* MPRIS D-BUS v2 media player remote control interface (basic controls only).
* Based on Tizonia's own OpenMAX IL-based multimedia framework. That means, no
  gstreamer, libav, or ffmpeg dependencies.

## A fully-featured OpenMAX IL 1.2 multimedia framework

1. 'libtizonia' : OpenMAX IL 1.2 component framework
  * A C library for creating OpenMAX IL 1.2 plugins (encoders, decoders,
  parsers, sinks, etc, for audio/video/other).
  * Full support for OpenMAX IL 1.2 Base and Interop profiles.
2. 'libtizcore' : OpenMAX IL 1.2 Core implementation
  * A C library for discovery and dynamic loading of OpenMAX IL 1.2 plugins.
  * Support for all of the OMX IL 1.2 standard Core APIs, including *OMX_SetupTunnel* and *OMX_TeardownTunnel*.
3. 'libtizplatform' : OS abstraction/utility library
  * A C library with wrappers and utilities for:
    * memory allocation,
    * threading and synchronization primitives,
    * evented I/O (via libev)
    * FIFO and priority queues,
    * dynamic arrays,
    * associative arrays,
    * small object allocation,
    * config file parsing,
    * HTTP parser,
    * uuids,
    * etc..
4. OpenMAX IL 1.2 Resource Management (RM)
  * 'tizrmd' : a D-Bus-based Resource Manager daemon server.
  * 'libtizrmproxy' : a C client library to interface with the RM daemon.

## Tizonia's OpenMAX IL 1.2 plugins

  * Spotify streaming service client ([libspotify](https://github.com/mopidy/libspotify-deb)),
  * Google Play Music streaming service client (based on [gmusicapi](https://github.com/simon-weber/gmusicapi)),
  * YouTube audio streaming service client (based on [pafy](https://github.com/mps-youtube/pafy)),
  * SoundCloud streaming service client (based on [soundcloud-python](https://github.com/soundcloud/soundcloud-python)),
  * Dirble internet radio station directory (Dirble REST API + libcurl)
  * Plex server client (based on [plexapi](https://github.com/pkkid/python-plexapi)),
  * Chromecast renderer (based on [pychromecast](https://github.com/balloob/pychromecast)),
  * mp3 decoders (libmad and libmpg123),
  * mpeg audio (mp2) decoder (libmpg123),
  * Sampled sound formats decoder (various pcm formats, including wav, etc, based on libsndfile)
  * AAC decoder (libfaad),
  * OPUS decoders (libopus and libopusfile)
  * FLAC decoder (libflac)
  * VORBIS decoder (libfishsound)
  * PCM renderers (ALSA and Pulseaudio)
  * OGG demuxer (liboggz)
  * WEBM demuxer (libnestegg)
  * HTTP renderer (i.e. ala icecast, for LAN streaming)
  * HTTP source (libcurl)
  * mp3 encoder (LAME),
  * a VP8 video decoder (libvpx),
  * a YUV video renderer (libsdl)
  * general purpose plugins, like binary file readers and writers
  * etc...

## Skema: Tizonia's test execution framework for OpenMAX IL components.
  * Test execution framework to build and test arbitrary OpenMAX IL graphs (tunneled and
    non-tunneled) using a custom, [easy-to-write XML syntax](http://github.com/tizonia/tizonia-openmax-il/wiki/Mp3Playback101).
  * Skema's Github repo: http://github.com/tizonia/skema
  * Skema's documentation is located in Tizonia's wiki: https://github.com/tizonia/tizonia-openmax-il/wiki/Skema

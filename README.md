<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [Tizonia](#tizonia)
- [Usage](#usage)
- [Installation](#installation)
  - [Configuration](#configuration)
  - [Upgrade](#upgrade)
    - [IMPORTANT: If you are upgrading to v0.6.0 from a previous release](#important-if-you-are-upgrading-to-v060-from-a-previous-release)
- [The Tizonia project](#the-tizonia-project)
  - [`tizonia`: A command-line cloud music player and audio streaming client/server.](#tizonia-a-command-line-cloud-music-player-and-audio-streaming-clientserver)
  - [A fully-featured OpenMAX IL 1.2 multimedia framework](#a-fully-featured-openmax-il-12-multimedia-framework)
  - [Tizonia's OpenMAX IL 1.2 plugins](#tizonias-openmax-il-12-plugins)
  - [Skema: Tizonia's test execution framework for OpenMAX IL components.](#skema-tizonias-test-execution-framework-for-openmax-il-components)
- [Building Tizonia](#building-tizonia)
- [License](#license)
- [More information](#more-information)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Tizonia

* A command-line music streaming client/server for Linux.
* With support for Spotify, Google Play Music (including Unlimited), YouTube,
  SoundCloud, Dirble and Deezer.
* A multimedia framework based on OpenMAX IL 1.2.

[![Build Status](https://travis-ci.org/tizonia/tizonia-openmax-il.png)](https://travis-ci.org/tizonia/tizonia-openmax-il)  |  [![Coverity Scan Build Status](https://scan.coverity.com/projects/594/badge.svg)](https://scan.coverity.com/projects/594)  |  [![Documentation Status](https://readthedocs.org/projects/tizonia-openmax-il/badge/?version=master)](https://readthedocs.org/projects/tizonia-openmax-il/?badge=master)

# Usage

![alt text](https://github.com/tizonia/tizonia-openmax-il/blob/master/docs/animated-gifs/tizonia-usage-screencast.gif "Tizonia usage")

# Installation

Tizonia's Debian packages are available from
[Bintray](https://bintray.com/tizonia), with the following distro/arch
combinations:

| Ubuntu Trusty (14.04) | Ubuntu Xenial (16.04) | Debian Jessie (8) | Raspbian Jessie (8) |
|        :---:          |        :---:          |        :---:      |       :---:         |
|     amd64, armhf      |     amd64, armhf      |    amd64, armhf   |      armhf          |
| [ ![Download](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-trusty/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-trusty/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-xenial/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-xenial/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/debian/tizonia-jessie/images/download.svg) ](https://bintray.com/tizonia/debian/tizonia-jessie/_latestVersion)  | [ ![Download](https://api.bintray.com/packages/tizonia/raspbian/tizonia-jessie/images/download.svg) ](https://bintray.com/tizonia/raspbian/tizonia-jessie/_latestVersion) |

> NOTE: Elementary OS and Linux Mint are also supported on releases based on Ubuntu 'Trusty' or Ubuntu 'Xenial'.

This script installs the
[latest](https://github.com/tizonia/tizonia-openmax-il/releases/latest)
release and all the dependencies, including [gmusicapi](https://github.com/simon-weber/gmusicapi), [soundcloud](https://github.com/soundcloud/soundcloud-python), [pafy](https://github.com/mps-youtube/pafy), and [youtube-dl](https://github.com/rg3/youtube-dl).

```bash

    $ curl -kL https://github.com/tizonia/tizonia-openmax-il/raw/master/tools/install.sh | bash
    # Or its shortened version:
    $ curl -kL https://goo.gl/Vu8qGR | bash

```

> NOTE: The usual disclaimers apply: trust no-one. Have a look at the installation script before running it in your system.

## Configuration

To use *Spotify*, *Google Play Music*, *SoundCloud*, *Dirble* and *Deezer*
(optional), introduce your credentials in Tizonia's config file (see
instructions inside the file for more information):

```bash

    $ mkdir -p $HOME/.config/tizonia
    $ cp /etc/tizonia/tizonia.conf/tizonia.conf $HOME/.config/tizonia/tizonia.conf

    ( now edit $HOME/.config/tizonia/tizonia.conf )

```

## Upgrade

To upgrade Tizonia, run 'apt-get' as usual, but also make sure the Python dependencies are up-to-date.

```bash

    $ sudo apt-get update && sudo apt-get upgrade

    # (Note that new versions of some of these Python packages are released frequently)
    $ sudo -H pip install --upgrade youtube-dl pafy gmusicapi soundcloud

```

### IMPORTANT: If you are upgrading to v0.6.0 from a previous release

If you are upgrading to v0.6.0, please note that plugins are now being
installed in a different directory, ${libdir}/tizonia0-plugins12; that means,
you will need to update your existing *tizonia.conf*, or else *tizonia* will
not work.  Just add 'tizonia0-plugins12' to the 'component-paths' configuration
variable in *tizonia.conf*.

i.e. before 0.6.0:

```
    component-paths = /usr/lib/arm-linux-gnueabihf;
```


after 0.6.0:

```
    component-paths = /usr/lib/arm-linux-gnueabihf/tizonia0-plugins12;
```

# The Tizonia project
The components of the Tizonia project are listed below:

## `tizonia`: A command-line cloud music player and audio streaming client/server.

* Stream playlists from Spotify (Spotify Premium required).
* Search and stream audio from Google Play Music (including Unlimited features).
* Search and stream audio from YouTube.
* Search and stream audio from SoundCloud.
* Search and stream Internet radio stations with Dirble.
* Search and stream audio from Deezer (subscription is not required).
* Playback of local media files (mp3, mp2, mpa, m2a, aac, ogg/vorbis, opus,
  wav, aiff, and flac).
* Icecast/SHOUTcast streaming LAN server (mp3).
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
  * Google Play Music streaming service client (based on [gmusicapi](https://github.com/simon-weber/gmusicapi))
  * YouTube audio streaming service client (based on [pafy](https://github.com/mps-youtube/pafy))
  * SoundCloud streaming service client (based on [soundcloud-python](https://github.com/soundcloud/soundcloud-python))
  * Dirble internet radio station directory (Dirble REST API + libcurl)
  * Deezer streaming service client (based on [mopidy-deezer](https://github.com/rusty-dev/mopidy-deezer))
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

# Building Tizonia

See [BUILDING.md](BUILDING.md) for instructions on how to build Tizonia from source.


# License

Tizonia OpenMAX IL is released under the GNU Lesser General Public License
version 3.

# More information

For more information, please visit the project web site at http://www.tizonia.org

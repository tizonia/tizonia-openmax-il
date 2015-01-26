# Tizonia #

* A music player and audio streaming client/server for Linux.
* With support for Spotify.
* A multimedia framework based on OpenMAX IL 1.2 provisional specification.

[![Build Status](https://travis-ci.org/tizonia/tizonia-openmax-il.png)](https://travis-ci.org/tizonia/tizonia-openmax-il)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/594/badge.svg)](https://scan.coverity.com/projects/594)

[![Documentation Status](https://readthedocs.org/projects/tizonia-openmax-il/badge/?version=latest)](https://readthedocs.org/projects/tizonia-openmax-il/?badge=latest)

## Introduction ##

The Tizonia project consists of a number of resources.

### `tizonia`: music player and audio streaming client/server ###

* Playback of audio formats from local media (formats: mp3, mp2, mpa, m2a, aac,
  ogg/vorbis, opus, wav, aiff, and flac).
* Spotify client.
* Icecast/SHOUTcast streaming server (formats: mp3).
* Icecast/SHOUTcast streaming client (formats: mp3, aac, and opus, more to be added in the future).
* Daemon and command line modes (no GUI).
* MPRIS D-BUS v2 media player remote control interface (early days, work-in-progress).
* Completely based on OpenMAX IL 1.2. No gstreamer, libav, or ffmpeg needed for audio
  decoding.
* Written in C++.

### A multimedia framework based on OpenMAX IL 1.2 ###

1. 'libtizonia' : An OpenMAX IL 1.2 component framework
  * A C library for creating OpenMAX IL 1.2 plugins (encoders, decoders,
  parsers, sinks, etc, for audio/video/other).
  * Full support for OpenMAX IL 1.2 Base and Interop profiles.
2. 'libtizcore' : An OpenMAX IL 1.2 Core implementation
  * A C library for discovery and dynamic loading of OpenMAX IL 1.2 plugins.
  * Support for all of the OMX IL 1.2 standard Core APIs, including *OMX_SetupTunnel* and *OMX_TeardownTunnel*.
3. 'libtizplatform' : An OS abstraction/utility library
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
4. An OpenMAX IL 1.2 Resource Management (RM) framework
  * 'tizrmd' : a D-Bus-based Resource Manager server.
  * 'libtizrmproxy' : a C client library to communicate with the RM daemon.
5. OpenMAX IL 1.2 plugins
  * mp3 decoders (libmad and libmpg123),
  * Spotify streaming service client (libspotify),
  * mpeg audio (mp2) decoder (libmpg123),
  * Sampled sound formats decoder (various pcm formats, like wav, etc, based on libsndfile)
  * AAC decoder (libfaad),
  * OPUS decoders (libopus and libopusfile)
  * FLAC decoder (libflac)
  * VORBIS decoder (libfishsound)
  * PCM renderers (ALSA and Pulseaudio)
  * OGG demuxer (liboggz)
  * HTTP renderer (i.e. ala icecast)
  * HTTP source (based on libcurl)
  * mp3 encoder (based on LAME),
  * a VP8 video decoder (libvpx),
  * a YUV video renderer (libsdl)
  * general purpose plugins, like binary file readers and writers
  * etc...

### Skema: A Python test execution framework for OpenMAX IL 1.2 components ###
  * Skema is a test framework for execution of arbitrary OpenMAX IL graphs (tunneled and
    non-tunneled) using a custom, [easy-to-write XML syntax](http://github.com/tizonia/tizonia-openmax-il/wiki/Mp3Playback101).
  * Skema repo: http://github.com/tizonia/skema

## How to build ##

To build and install from source, follow the following steps (Ubuntu 14.04 is assumed).

### Dependencies ###

```bash

    $ sudo apt-get update -qq && sudo apt-get install -qq \
    build-essential autoconf automake autotools-dev \
    libtool libmad0-dev liblog4c-dev \
    libasound2-dev libdbus-1-dev \
    libdbus-c++-dev libsqlite3-dev libboost-all-dev \
    uuid-dev libsdl1.2-dev libvpx-dev libmp3lame-dev libfaad-dev \
    libev-dev libtag1-dev libfishsound-dev libmediainfo-dev \
    libcurl3-dev libpulse-dev libmpg123-dev libvorbis-dev libopus-dev \
    libopusfile-dev libogg-dev libflac-dev liboggz2-dev \
    libsndfile1-dev curl check wget sqlite3 dbus-x11

```

### libspotify binaries ###

To stream music from Spotify, libspotify needs to be present in your system. A
suitable 'libspotify' flavour can be downloaded from Spotify's website:

    https://developer.spotify.com/technologies/libspotify/

However, the Makefile(s) found in the tarballs listed in Spotify's website may
or may not work for you out of the box. Alternatively, you can download from my
site patched versions of the i686 and x86_64 tarballs that will work in any
regular Ubuntu or Debian-based system.

E.g.: To download and install the *x86_64* version of the library, replace
*$INSTALL_DIR* with your favorite location; replace *x86_64* with *i686* if you
need the 32-bit version of the library):

```bash

    $ wget http://www.juanrubio.me/tizonia/libspotify-12.1.51-Linux-x86_64.tgz
    $ tar zxvf libspotify-12.1.51-Linux-x86_64.tgz
    $ cd libspotify-12.1.51-Linux-x86_64
    $ make install prefix=$INSTALL_DIR

```

### Building the multimedia framework ###

From the top of Tizonia's repo (replace *$INSTALL_DIR* with your favorite
location), type the following:

```bash

    $ autoreconf -ifs
    $ ./configure --enable-silent-rules --prefix=$INSTALL_DIR CFLAGS="-O3 -DNDEBUG"
    $ make
    $ make install

```

### Building 'tizonia', the music player and streaming client/server ###

After completing the steps above, change directory to the 'tizonia' sub-folder
inside the repo (again replace *$INSTALL_DIR* with your favorite location), and
type the following:

```bash

    $ cd tizonia
    $ autoreconf -ifs
    $ ./configure --enable-silent-rules --prefix=$INSTALL_DIR CFLAGS="-O3 -DNDEBUG"
    $ make
    $ make install

```

### Tizonia config and the D-BUS service activation files ###

Place *tizonia.conf* and the Resource Manager's D-BUS activation file to the
following locations:

```bash

    $ cp config/tizonia.conf ~/.tizonia.conf
    $ mkdir -p ~/.local/share/dbus-1/services
    $ cp rm/dbus/com.aratelia.tiz.rm.service ~/.local/share/dbus-1/services

```

## 'tizonia' usage ##

```bash

$ tizonia
tizonia 0.1.0. Copyright (C) 2015 Juan A. Rubio
This software is part of Tizonia <http://tizonia.org>

LGPLv3: GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

General options:
  -h [ --help ]         Print the usage message.
  -v [ --version ]      Print the version information.
  -R [ --recurse ]      Recursively process the given folder.
  -S [ --shuffle ]      Shuffle the playlist.
  -d [ --daemon ]       Run in the background.

OpenMAX IL options:
  --list-comp           Enumerate all the OpenMAX IL components in the system.
  --roles-of-comp arg   Display the OpenMAX IL roles found in component <arg>.
  --comps-of-role arg   Display the OpenMAX IL components that implement role
                        <arg>.

Audio streaming server options:
  --server              Stream media files using the SHOUTcast/ICEcast
                        streaming protocol.
  -p [ --port ] arg     TCP port used for SHOUTcast/ICEcast streaming. Default:
                        8010.
  --station-name arg    The SHOUTcast/ICEcast station name.
  --station-genre arg   The SHOUTcast/ICEcast station genre.
  --bitrate-modes arg   A comma-separated-list of bitrate modes (e.g.
                        'CBR,VBR') that will be allowed in the playlist.
                        Default: any.
  --sampling-rates arg  A comma-separated-list of sampling rates that will be
                        allowed in the playlist. Default: any.

Spotify client options:
  --spotify-user arg     Spotify user's name.
  --spotify-password arg Spotify user's password.
  --spotify-playlist arg Spotify playlist name.

```

#### Known issues ####

`tizonia` makes heavy use the the the
[Boost Meta State Machine (MSM)](http://www.boost.org/doc/libs/1_55_0/libs/msm/doc/HTML/index.html)
library by Christophe Henry (MSM is in turn based on
[Boost MPL](http://www.boost.org/doc/libs/1_56_0/libs/mpl/doc/index.html)).

MSM is used to generate a number of state machines that control the tunneled
OpenMAX IL components for the various playback uses cases. The state machines
are quite large and MSM is known for not being easy on the compilers. Building
`tizonia` requires a bit of patience and a whole lot of RAM (2.5+ GB).

You may see GCC crashing like below; simply keep running `make -j1` or `make
-j1 install` until the application is fully built (it will eventually, given
the sufficient amount RAM).

```bash

Making all in src
  CXX      tizonia-tizplayapp.o
  CXX      tizonia-main.o
  CXX      tizonia-tizomxutil.o
  CXX      tizonia-tizprogramopts.o
  CXX      tizonia-tizgraphutil.o
  CXX      tizonia-tizgraphcback.o
  CXX      tizonia-tizprobe.o
  CXX      tizonia-tizdaemon.o
  CXX      tizonia-tizplaylist.o
  CXX      tizonia-tizgraphfactory.o
  CXX      tizonia-tizgraphmgrcmd.o
  CXX      tizonia-tizgraphmgrops.o
  CXX      tizonia-tizgraphmgrfsm.o
  CXX      tizonia-tizgraphmgr.o
  CXX      tizonia-tizdecgraphmgr.o
g++: internal compiler error: Killed (program cc1plus)
Please submit a full bug report,
with preprocessed source if appropriate.
See <file:///usr/share/doc/gcc-4.8/README.Bugs> for instructions.
make[2]: *** [tizonia-tizplayapp.o] Error 4
make[2]: *** Waiting for unfinished jobs....
g++: internal compiler error: Killed (program cc1plus)
Please submit a full bug report,
with preprocessed source if appropriate.
See <file:///usr/share/doc/gcc-4.8/README.Bugs> for instructions.

```

## License ##

Tizonia OpenMAX IL is released under the GNU Lesser General Public License
version 3.

## More information ##

For more information, please visit the project web site at http://tizonia.org

# Tizonia #

A command line music player and streaming server for Linux.

The Tizonia project also contains a multimedia framework based on OpenMAX IL 1.2
provisional specification.

[![Build Status](https://travis-ci.org/tizonia/tizonia-openmax-il.png)](https://travis-ci.org/tizonia/tizonia-openmax-il)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/594/badge.svg)](https://scan.coverity.com/projects/594)

## Introduction ##

The Tizonia project consists of a number of resources.

### `tizonia`: a command line music player and audio streaming server ###

* Features:
    * Playback of audio formats from local media files (pcm, mp3, aac, vorbis,
      opus, and flac encodings).
    * Icecast/Shoutcast client (mp3, aac, and opus streams supported).
    * Icecast/Shoutcast server (mp3 streams).
    * MPRIS D-BUS v2 interface for controlling local playback.
    * Entirely based on OpenMAX IL 1.2.

### 'libtizonia' : An OpenMAX IL 1.2 component framework/library ###

* Full support for OpenMAX IL 1.2 Base and Interop profile components.

### 'libtizcore' : An OpenMAX IL 1.2 Core implementation ###

* Enables discovery and dynamic loading of OpenMAX IL 1.2 plugin components
  (encodes, decoders, container parsrs, sinks, etc).
* Support for all the usual OMX IL 1.2 Core APIs, including *OMX_SetupTunnel* and *OMX_TeardownTunnel*.

### 'libtizplatform' : An OS abstraction/utility library ###

* A number of wrappers and utilities to ease the creation of OpenMAX IL 1.2
  components. With APIs and resources like:
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

### An OpenMAX IL 1.2 Resource Management (RM) framework ###

* Including:
  * 'tizrmd' : a D-Bus-based Resource Manager server.
  * 'libtizrmproxy' : a client library to communicate with the RM server.

### OpenMAX IL 1.2 plugins ###

* Including:
  * two mp3 decoders (one based on libmad and another based libmpg123),
  * Spotify client (coming soon, based on libspotify),
  * AAC decoder (based on libfaad),
  * two OPUS decoders (based on libopus and libopusfile)
  * FLAC decoder (based on libflac)
  * VORBIS decoder (based on libfishsound)
  * two PCM renderers (ALSA and Pulseaudio)
  * OGG demuxer (based on liboggz)
  * an HTTP renderer (i.e. ala icecast)
  * an HTTP source (based on libcurl)
  * a VP8 video decoder (based on libvpx),
  * a YUV video renderer (based on libsdl)
  * binary file readers and writers
  * mp3 encoder (based on LAME),
  * etc...

## How to build ##

On Ubuntu 14.04, the following build instructions should work ok.

### Dependencies ###

```bash

    $ sudo apt-get update -qq && sudo apt-get install -qq \
    build-essential autoconf automake autotools-dev \
    libtool libmad0-dev liblog4c-dev \
    libasound2-dev libdbus-1-dev \
    libdbus-c++-dev libsqlite3-dev libboost-all-dev \
    uuid-dev libsdl1.2-dev libvpx-dev libavcodec-dev \
    libavformat-dev libavdevice-dev libmp3lame-dev libfaad-dev \
    libev-dev libtag1-dev libfishsound-dev libmediainfo-dev \
    libcurl3-dev libpulse-dev libmpg123-dev libvorbis-dev libopus-dev \
    libopusfile-dev libogg-dev libflac-dev liboggz2-dev \
    libsndfile1-dev curl check wget sqlite3 dbus-x11

```

### Building the libraries, plugins and RM framework ###

From the top of the repo (Replace *$INSTALL_DIR* with your favorite location):

```bash

    $ autoreconf -ifs
    $ ./configure --enable-silent-rules --prefix=$INSTALL_DIR CFLAGS="-O3 -DNDEBUG"
    $ make
    $ make install

```

### Building 'tizonia', the music player/streaming server ###

From the'tizonia' sub-folder (again replace *$INSTALL_DIR* with your favorite location):

```bash

    $ cd tizonia
    $ autoreconf -ifs
    $ ./configure --enable-silent-rules --prefix=$INSTALL_DIR CFLAGS="-O3 -DNDEBUG"
    $ make
    $ make install

```

### Tizonia config file and the D-BUS service file ###

Copy the *tizonia.conf* file and the Resource Manager's D-BUS service file to a
suitable location:

```bash

    $ cp config/tizonia.conf ~/.tizonia.conf
    $ mkdir -p ~/.local/share/dbus-1/services
    $ cp rm/dbus/com.aratelia.tiz.rm.service ~/.local/share/dbus-1/services

```

## 'tizonia' usage ##

```bash

$ tizonia
tizonia 0.1.0. Copyright (C) 2014 Juan A. Rubio
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
```

#### Known issues ####

`tizonia` makes heavy use the the the
[Boost Meta State Machine (MSM)](http://www.boost.org/doc/libs/1_55_0/libs/msm/doc/HTML/index.html)
library by Christophe Henry (MSM is in turn based on
[Boost MPL](http://www.boost.org/doc/libs/1_56_0/libs/mpl/doc/index.html)).

MSM is used to generate a number of state machines that control the tunneled
OpenMAX IL components for the various playback uses cases. The state machines
are quite large and MSM is known for not being easy on the compilers. So
building `tizonia` requires a bit of patience and a whole lot of RAM (2.5+ GB).

You may see GCC crashing like below; simply keep running `make -j1` or `make
-j1 install` until all of tizonia's objects get built (they all will eventually,
if you have the sufficient amount RAM).

(At some point, I'll look into optimising `tizonia` so that building it requires
less RAM and/or time).

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

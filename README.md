# Tizonia OpenMAX IL #

An experimental implementation for Linux of the OpenMAX IL 1.2 provisional
specification.

[![Build Status](https://travis-ci.org/tizonia/tizonia-openmax-il.png)](https://travis-ci.org/tizonia/tizonia-openmax-il)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/594/badge.svg)](https://scan.coverity.com/projects/594)

_This is a highly experimental, rapidly-changing project. APIs might change overnight._

## Introduction ##

The Tizonia OpenMAX IL project consists of a number of resources.

### An OpenMAX IL 1.2 component framework/library ###

* Support for Base and Interop profiles.
* TODO: Buffer sharing.

### An OpenMAX IL 1.2 Core implementation ###

* Support for all the usual OMX IL 1.2 Core APIs, including *OMX_SetupTunnel*.
* TODO: IL 1.2 Core extension APIs.

### An OS abstraction/utility library ###

* Wrappers and utilities for:
    * memory allocation,
    * threading and synchronization primitives,
    * FIFO and priority queues,
    * dynamic arrays,
    * associative arrays,
    * small object allocation,
    * HTTP parsing,
    * uuids,
    * config file parsing,
    * evented I/O (via libev)
    * etc..

### An OpenMAX IL Resource Management (RM) framework ###

* Including:
  * a C client library,
  * a D-Bus-based RM server written in C++.

### A number of OpenMAX IL plugins ###

* Including:
  * mp3 decoder (based on libmad),
  * mp3 encoder (based on LAME),
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
  * etc...

### `tplay`: a command line music player and streaming server ###

* Features:
    * Uses graphs of tunneled OpenMAX IL components.
    * Playback of audio formats from local files (mp3, aac, vorbis, opus,
      flac).
    * Playback of remote Icecast/Shoutcast streams (currently mp3, aac, and
      opus streams are supported).
    * Serving of Icecast/Shoutcast streams (mp3).
    * MPRIS D-BUS v2 interface.

## How to build ##

On Ubuntu 14.04, the following build instructions should work ok.

### Dependencies ###

```bash

    $ sudo apt-get update -qq && sudo apt-get install -qq \
    autoconf automake autotools-dev build-essential \
    libtool libmad0-dev liblog4c-dev \
    libasound2-dev libdbus-1-dev \
    libdbus-c++-dev libsqlite3-dev libboost-all-dev \
    uuid-dev libsdl1.2-dev libvpx-dev libavcodec-dev \
    libavformat-dev libavdevice-dev libmp3lame-dev libfaad-dev \
    libev-dev libtag1-dev libfishsound-dev libmediainfo-dev \
    libcurl3-dev libpulse-dev \
    curl check wget sqlite3 dbus-x11

```

### Building the base libraries, plugins and the RM server ###

From the top of the repo (Replace *$INSTALL_DIR* with your favorite location):

```bash

    $ autoreconf -ifs
    $ ./configure --enable-silent-rules --prefix=$INSTALL_DIR CFLAGS="-O3 -DNDEBUG"
    $ make
    $ make install

```

### Building 'tplay' ###

From the'tplay' folder (again replace *$INSTALL_DIR* with your favorite location):

```bash

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

## 'tplay' usage information ##

```bash

$ tplay
tplay 0.1.0. Copyright (C) 2014 Juan A. Rubio
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

## License ##

Tizonia OpenMAX IL is released under the GNU Lesser General Public License
version 3.

## More information ##

For more information, please visit the project web site at http://tizonia.org

# Tizonia #

* A music player and audio streaming client/server for Linux.
* With support for Spotify and Google Play Music.
* A multimedia framework based on OpenMAX IL 1.2 provisional specification.

[![Build Status](https://travis-ci.org/tizonia/tizonia-openmax-il.png)](https://travis-ci.org/tizonia/tizonia-openmax-il)  |  [![Coverity Scan Build Status](https://scan.coverity.com/projects/594/badge.svg)](https://scan.coverity.com/projects/594)  |  [![Documentation Status](https://readthedocs.org/projects/tizonia-openmax-il/badge/?version=master)](https://readthedocs.org/projects/tizonia-openmax-il/?badge=master)

## Introduction ##

_Tizonia is still under development and there are no pre-built binary releases yet (coming soon)_.

For now you can clone this repo and build everything from source following the instructions below.

## The Tizonia project

For the resources that can be found here, please keep reading further.

### `tizonia`: music player and audio streaming client/server ###

* Spotify client (Spotify Premium account required).
* Google Play Music client (with support for All Access features, if a subscription is available).
* Playback of local media (mp3, mp2, mpa, m2a, aac, ogg/vorbis, opus, wav,
  aiff, and flac).
* Icecast/SHOUTcast streaming LAN server (mp3).
* Icecast/SHOUTcast streaming client (mp3, aac, and opus).
* Daemon and command line modes (no GUI).
* MPRIS D-BUS v2 media player remote control interface (basic functions only).
* Based on own OpenMAX IL-based multimedia framework. No gstreamer, libav, or ffmpeg libraries needed.

### OpenMAX IL 1.2 multimedia framework ###

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
4. OpenMAX IL 1.2 Resource Management (RM) framework
  * 'tizrmd' : a D-Bus-based Resource Manager daemon server.
  * 'libtizrmproxy' : a C client library to interface with the RM daemon.
5. OpenMAX IL 1.2 codecs/plugins
  * Spotify streaming service client (libspotify),
  * Google Play Music streaming service client (based on gmusicapi)
  * mp3 decoders (libmad and libmpg123),
  * mpeg audio (mp2) decoder (libmpg123),
  * Sampled sound formats decoder (various pcm formats, including wav, etc, based on libsndfile)
  * AAC decoder (libfaad),
  * OPUS decoders (libopus and libopusfile)
  * FLAC decoder (libflac)
  * VORBIS decoder (libfishsound)
  * PCM renderers (ALSA and Pulseaudio)
  * OGG demuxer (liboggz)
  * HTTP renderer (i.e. ala icecast, for LAN streaming)
  * HTTP source (based on libcurl)
  * mp3 encoder (based on LAME),
  * a VP8 video decoder (libvpx),
  * a YUV video renderer (libsdl)
  * general purpose plugins, like binary file readers and writers
  * etc...

### Skema: Tizonia's Python-based test execution framework ###
  * Test execution framework to build and test arbitrary OpenMAX IL graphs (tunneled and
    non-tunneled) using a custom, [easy-to-write XML syntax](http://github.com/tizonia/tizonia-openmax-il/wiki/Mp3Playback101).
  * Skema's Github repo: http://github.com/tizonia/skema

## Building from source ##

To build and install from source, follow the these steps (Ubuntu 14.04 is
assumed, but should work on other relatively recent debian-based distros).

### Dependencies ###

```bash

    $ sudo apt-get update -qq && sudo apt-get install -qq \
    build-essential autoconf autoconf-archive \
    automake autotools-dev libtool libmad0-dev liblog4c-dev \
    libasound2-dev libdbus-1-dev \
    libdbus-c++-dev libsqlite3-dev libboost-all-dev \
    uuid-dev libsdl1.2-dev libvpx-dev libmp3lame-dev libfaad-dev \
    libev-dev libtag1-dev libfishsound-dev libmediainfo-dev \
    libcurl3-dev libpulse-dev libmpg123-dev libvorbis-dev libopus-dev \
    libopusfile-dev libogg-dev libflac-dev liboggz2-dev \
    libsndfile1-dev curl check wget sqlite3 dbus-x11 \
    python-setuptools

```

### Adjusting environment variables

This assumes we are installing to a temporary directory inside the $HOME
folder. The following environment variables are exported before starting the
build.

```bash

    $ INSTALL_DIR="$HOME/temp"
    $ LIB_DIR="$INSTALL_DIR/lib"
    $ PKG_CONFIG_DIR="$LIB_DIR/pkgconfig"

    $ export PKG_CONFIG_PATH="$PKG_CONFIG_DIR"
    $ export LD_LIBRARY_PATH="$LIB_DIR"
    $ export PYTHONPATH=$PYTHONPATH:$LIB_DIR/python2.7/site-packages

```

### libspotify ###

To stream music from Spotify, libspotify needs to be present in your
system. You can install libspotify from
[Modipy](https://github.com/mopidy/libspotify-deb) APT archive, like this:

```bash

    $ wget -q -O - http://apt.mopidy.com/mopidy.gpg | sudo apt-key add - \
        && echo "deb http://apt.mopidy.com/ stable main contrib non-free" | sudo tee -a /etc/apt/sources.list \
        && echo "deb-src http://apt.mopidy.com/ stable main contrib non-free" | sudo tee -a /etc/apt/sources.list \
        && sudo apt-get update \
        && sudo apt-get install libspotify12 libspotify-dev -qq

```

### Google Play Music ###

To stream from Google Play Music, you need to install Simon Weber's
[gmusicapi](https://github.com/simon-weber/gmusicapi) python library. You may
want to install it from source as the latest version in pip
[6.0.0](https://pypi.python.org/pypi/gmusicapi/6.0.0) sometimes is out-of-date
and may or may not work. In order to satisfy all of 'gmusicapi's depedencies
you may need to download and install a fresher version of setuptools (see
https://pypi.python.org/pypi/setuptools).


```bash

    $ echo "OPTIONAL: Downloading and installing a fresh version of setuptools"
    $ wget https://bootstrap.pypa.io/ez_setup.py -O - | sudo python

    $ echo "Installing the latest gmusicapi from source..."
    $ git clone https://github.com/simon-weber/gmusicapi \
      && cd gmusicapi \
      && sudo python setup.py install

```


### Building the multimedia framework ###

From the top of Tizonia's repo, type the following:

```bash

    $ autoreconf -ifs \
        && ./configure --enable-silent-rules --prefix=$INSTALL_DIR CFLAGS="-O2 -s -DNDEBUG" \
        && make \
        && make install

```

### Building 'tizonia', the music player and streaming client/server ###

After completing the steps above, change directory to the 'player' sub-folder
inside the repo and build the application using the following commands:

```bash

    $ cd player
    $ autoreconf -ifs \
        &&./configure --enable-silent-rules --prefix=$INSTALL_DIR CXXFLAGS="-O2 -s -DNDEBUG -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security" \
        && make \
        && make install

```

### Tizonia configuration file ###

Copy *tizonia.conf* to the following location:

```bash

    $ mkdir -p $HOME/.config/tizonia \
        && cp config/src/tizonia.conf $HOME/.config/tizonia

```

### Resource Manager's D-BUS service activation file (optional) ###

OpenMAX IL Resource Management is present but disabled by default. In case this
is to be used (prior to that, needs to be explicitly enabled in tizonia.conf),
copy the Resource Manager's D-BUS activation file like this:

```bash

    $ mkdir -p ~/.local/share/dbus-1/services \
        && cp rm/tizrmd/dbus/com.aratelia.tiz.rm.service ~/.local/share/dbus-1/services

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
  -r [ --recurse ]      Recursively process a given path.
  -s [ --shuffle ]      Shuffle the playlist.
  -d [ --daemon ]       Run in the background.

OpenMAX IL options:
  -L [ --comp-list ]         Enumerate all the OpenMAX IL components in the
                             system.
  -R [ --roles-of-comp ] arg Display the OpenMAX IL roles found in component
                             <arg>.
  -C [ --comps-of-role ] arg Display the OpenMAX IL components that implement
                             role <arg>.

Audio streaming server options:
  --server              Stream media files using the SHOUTcast/ICEcast
                        streaming protocol.
  -p [ --port ] arg     TCP port to be used for Icecast/SHOUTcast streaming.
                        Default: 8010.
  --station-name arg    The Icecast/SHOUTcast station name. Optional.
  --station-genre arg   The Icecast/SHOUTcast station genre. Optional.
  --no-icy-metadata     Disables Icecast/SHOUTcast metadata in the stream.
  --bitrate-modes arg   A comma-separated list of bitrate modes (e.g.
                        'CBR,VBR'). Only these bitrate omdes will allowed in
                        the playlist. Default: all.
  --sampling-rates arg  A comma-separated list of sampling rates. Only these
                        sampling rates will be allowed in the playlist.
                        Default: all.

Spotify options:
  --spotify-user arg     Spotify user name (Optional: may also be provided via
                         config file).
  --spotify-password arg Spotify user password (Optional: may also be provided
                         via config file).
  --spotify-playlist arg Spotify playlist name.

Google Play Music options:
  --gmusic-user arg                    Google Play Music user's name (not
                                       required if provided via config file).
  --gmusic-password arg                Google Play Music user's password (not
                                       required if provided via config file).
  --gmusic-device-id arg               Google Play Music device id (not
                                       required if provided via config file).
  --gmusic-artist arg                  Play tracks from the user's library by
                                       artist.
  --gmusic-album arg                   Play tracks from the user's library by
                                       album.
  --gmusic-playlist arg                Play a playlist from the user's library.
  --gmusic-station arg                 Play a station from the user's library.
  --gmusic-feeling-lucky-station       Play the user's 'I'm Feeling Lucky'
                                       station.
  --gmusic-all-access-promoted-tracks  Play All Access promoted tracks.
  --gmusic-all-access-album arg        Search and play All Access tracks by
                                       album (best match only).
  --gmusic-all-access-artist arg       Search and play All Access tracks by
                                       artist (best match only).
  --gmusic-all-access-tracks arg       Search and play All Access tracks by
                                       name (50 best matches only).

```

#### Known issues ####

The `tizonia` player app makes heavy use the the the
[Boost Meta State Machine (MSM)](http://www.boost.org/doc/libs/1_55_0/libs/msm/doc/HTML/index.html)
library by Christophe Henry (MSM is in turn based on
[Boost MPL](http://www.boost.org/doc/libs/1_56_0/libs/mpl/doc/index.html)).

MSM is used to generate a number of state machines that control the tunneled
OpenMAX IL components for the various playback uses cases. The state machines
are quite large and MSM is known for not being easy on the compilers. Building
`tizonia` requires quite a bit of RAM (~2 GB).

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

# Tizonia #

* A command-line music streaming client/server for Linux.
* With support for Spotify, Google Play Music (including Unlimited), YouTube,
  SoundCloud and Dirble.
* A multimedia framework based on OpenMAX IL 1.2.

[![Build Status](https://travis-ci.org/tizonia/tizonia-openmax-il.png)](https://travis-ci.org/tizonia/tizonia-openmax-il)  |  [![Coverity Scan Build Status](https://scan.coverity.com/projects/594/badge.svg)](https://scan.coverity.com/projects/594)  |  [![Documentation Status](https://readthedocs.org/projects/tizonia-openmax-il/badge/?version=master)](https://readthedocs.org/projects/tizonia-openmax-il/?badge=master)

## Install the latest release

To install the
[latest release](https://github.com/tizonia/tizonia-openmax-il/releases/latest):

```bash

    $ curl -kL https://github.com/tizonia/tizonia-openmax-il/raw/master/tools/install.sh | bash
    # Or its shortened version:
    $ curl -kL https://goo.gl/Vu8qGR | bash

```

> NOTE: This installs the latest Tizonia Debian packages from [Bintray](https://bintray.com/tizonia) along with all their dependencies.

Finally, to use *Spotify*, *Google Play Music*, *SoundCloud* and *Dirble*,
introduce your credentials in Tizonia's config file (see instructions inside
the file for more information):

```bash

    $ mkdir -p $HOME/.config/tizonia
    $ cp /etc/tizonia/tizonia.conf/tizonia.conf $HOME/.config/tizonia/tizonia.conf

    ( now edit $HOME/.config/tizonia/tizonia.conf )

```

The Debian packages are hosted on [Bintray](https://bintray.com/tizonia), with
the following distro/arch combinations (NOTE: 14.04 packages not being built anymore):

| Ubuntu Xenial (16.04) | Debian Jessie (8) | Raspbian Jessie (8) |
|        :---:          |        :---:      |       :---:         |
|     amd64, armhf      |    amd64, armhf   |      armhf          |
| [ ![Download](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-xenial/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-xenial/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/debian/tizonia-jessie/images/download.svg) ](https://bintray.com/tizonia/debian/tizonia-jessie/_latestVersion)  | [ ![Download](https://api.bintray.com/packages/tizonia/raspbian/tizonia-jessie/images/download.svg) ](https://bintray.com/tizonia/raspbian/tizonia-jessie/_latestVersion) |

### IMPORTANT: If your are upgrading to v0.6.0 from a previous release

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

## 'tizonia' usage ##

![alt text](https://github.com/tizonia/tizonia-openmax-il/blob/master/docs/animated-gifs/tizonia-usage-screencast.gif "Tizonia usage")

## The Tizonia project

### `tizonia`: command line music player and audio streaming client/server ###

* Stream playlists from Spotify (Spotify Premium required).
* Search and stream audio from Google Play Music (including Unlimited features).
* Search and stream audio from YouTube.
* Search and stream audio from SoundCloud.
* Search and stream Internet radio stations with Dirble.
* Playback of local media files (mp3, mp2, mpa, m2a, aac, ogg/vorbis, opus,
  wav, aiff, and flac).
* Icecast/SHOUTcast streaming LAN server (mp3).
* Icecast/SHOUTcast streaming client (mp3, aac, and opus).
* Daemon and command line modes (no GUI).
* MPRIS D-BUS v2 media player remote control interface (basic controls only).
* Based on Tizonia's own OpenMAX IL-based multimedia framework. That means, no
  gstreamer, libav, or ffmpeg dependencies.

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
  * Google Play Music streaming service client (based on 'gmusicapi' Python module + libcurl)
  * YouTube audio streaming service client (based on 'pafy' Python module + libcurl)
  * SoundCloud streaming service client (based on 'soundcloud' Python module + libcurl)
  * Dirble internet radio station directory (Dirble REST API + libcurl)
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
  * HTTP source (based on libcurl)
  * mp3 encoder (based on LAME),
  * a VP8 video decoder (libvpx),
  * a YUV video renderer (libsdl)
  * general purpose plugins, like binary file readers and writers
  * etc...

### Skema: Tizonia's Python test execution framework ###
  * Test execution framework to build and test arbitrary OpenMAX IL graphs (tunneled and
    non-tunneled) using a custom, [easy-to-write XML syntax](http://github.com/tizonia/tizonia-openmax-il/wiki/Mp3Playback101).
  * Skema's Github repo: http://github.com/tizonia/skema

## Building from source ##

To build and install from source, follow these steps (Ubuntu 16.04 is assumed,
but should work on other recent Debian-based distros).

### Dependencies ###

To install all the development dependencies, the easiest way is to use the
'tizonia-dev-build' tool. This script lives under the 'tools' folder and
maintains an up-to-date list of all the packages that are required in a
Debian-compatible system to be able to build Tizonia from source.

> NOTE: The following command installs Mopidy's 'libspotify-dev' package, the
> 'gmusicapi', 'soundcloud', 'youtube-dl' and 'pafy' python packages, among
> other things.

```bash

    $ cd tools

    # Define the following environment variables
    $ export TIZONIA_REPO_DIR=/path/to/tizonia/repo # (e.g. /home/juan/work/tizonia)
    $ export TIZONIA_INSTALL_DIR=/path/to/install/basedir # (e.g. /home/juan/temp)

    # Install everything needed to build Tizonia on a Debian/Ubuntu system
    $ ./tizonia-dev-build --deps

```

### Building ###

After the dependencies have been installed, build and install the Tizonia
OpenMAX IL framework, the IL plugins and the 'tizonia' command-line
application:

```bash

   $ cd tools

   # Configure all Tizonia sub-projects with 'release' flags, build and
   # install.
   $ ./tizonia-dev-build --release --install

   or

   # Configure with 'debug' flags, build and install.
   $ ./tizonia-dev-build --debug --install

```

Alternatively, from the top of Tizonia's repo, one can also do the familiar:

```bash

    $ autoreconf -ifs
    $ ./configure
    $ make
    $ make install

```

### Tizonia configuration file ###

Copy *tizonia.conf* into the user's config folder:

```bash

    $ mkdir -p $HOME/.config/tizonia \
        && cp config/src/tizonia.conf $HOME/.config/tizonia

```

### Resource Manager's D-BUS service activation file (optional) ###

OpenMAX IL Resource Management is present but disabled by default. In case this
is to be used (prior to that, needs to be explicitly enabled in tizonia.conf),
copy the Resource Manager's D-BUS activation file to some place where it can be
found by the DBUS services. E.g:

```bash

    $ mkdir -p ~/.local/share/dbus-1/services \
        && cp rm/tizrmd/dbus/com.aratelia.tiz.rm.service ~/.local/share/dbus-1/services

```

### Known issues ###

The `tizonia` player app makes heavy use the the the
[Boost Meta State Machine (MSM)](http://www.boost.org/doc/libs/1_55_0/libs/msm/doc/HTML/index.html)
library by Christophe Henry (MSM is in turn based on
[Boost MPL](http://www.boost.org/doc/libs/1_56_0/libs/mpl/doc/index.html)).

MSM is used to generate a number of state machines that control the tunneled
OpenMAX IL components for the various playback uses cases. The state machines
are quite large and MSM is known for not being easy on the compilers. Building
the `tizonia` command-line app in 'debug' configuration (with debug symbols,
etc) requires quite a bit of RAM.

You may see GCC crashing like below; simply keep running `make -j1` or `make
-j1 install` until the application is fully built (it will finish eventually,
given the sufficient amount RAM). An alternative to that is to build in
'release' mode.

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

For more information, please visit the project web site at http://www.tizonia.org

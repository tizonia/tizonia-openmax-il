<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [Building from Source](#building-from-source)
  - [Dependencies](#dependencies)
  - [Building](#building)
    - ['Debug' variant](#debug-variant)
    - ['Release' variant](#release-variant)
    - [Single Debian package created with 'checkinstall'](#single-debian-package-created-with-checkinstall)
    - [The tradional method](#the-tradional-method)
  - [Tizonia's configuration file](#tizonias-configuration-file)
  - [Resource Manager's D-BUS service activation file (optional)](#resource-managers-d-bus-service-activation-file-optional)
  - [Known issues](#known-issues)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## Building from Source ##

To build and install from source, follow these steps (Ubuntu 16.04 is assumed,
but should work on other recent Debian-based distros).

### Dependencies ###

To install all the development dependencies, the easiest way is to use the
'tizonia-dev-build' tool. This script lives under the 'tools' directory and
maintains an up-to-date list of all the packages that are required in a
Debian-compatible system to build Tizonia from source.

> NOTE: The following command installs Mopidy's 'libspotify-dev' package, the
> 'gmusicapi', 'soundcloud', 'pafy' and 'youtube-dl python packages, among
> other things.

```bash

    Setup the following environment variables
    $ export TIZONIA_REPO_DIR=/path/to/tizonia/repo # (e.g. /home/user/tizonia-openmax-il)
    $ export TIZONIA_INSTALL_DIR=/path/to/install/dir # (e.g. /usr or /home/user/temp)
    $ export PATH=$TIZONIA_REPO_DIR/tools:$PATH

    Install everything needed to build Tizonia from source (Debian derivative assumed)
    $ tools/tizonia-dev-build --deps

```

### Building

Once all the dependencies have been installed, build and install the OpenMAX IL
framework, all plugins and the 'tizonia' command-line application, using any of
the following methods.

#### 'Debug' variant

The following command re-configures all sub-projects with 'debug' flags, builds
and installs them.

```bash

   $ tools/tizonia-dev-build --debug --install

```

#### 'Release' variant

The following command re-configures all sub-projects with 'release' flags,
builds and installs them.

```bash

   $ tools/tizonia-dev-build --release --install

```

#### Single Debian package created with 'checkinstall'

The following command configures all sub-projects with 'release' flags
appropriate for a Debian/Ubuntu system, builds the whole repo, then using
[checkinstall](https://debian-administration.org/article/147/Installing_packages_from_source_code_with_checkinstall)
creates a single Debian package and installs it in the system. The package can
then be removed via 'dpkg' or even moved to another machine for testing.

> NOTE: This is not how the Debian packages hosted on Bintray are created. The
  packages hosted on Bintray are fully 'debianized' packages created using the
  'tizonia-qemu-debootstrap-env' script.

```bash

   This produces and install a Debian package called 'tizonia-all-testing'
   $ tools/tizonia-dev-build --debian

   To remove from the system, run:
   $ dpkg -r tizonia-all-testing

```

#### The tradional method

Alternatively, from the top of Tizonia's repo, one can also do the familiar:

```bash

    $ autoreconf -ifs
    $ ./configure
    $ make
    $ make install

```

### Tizonia's configuration file ###

Copy *tizonia.conf* into the user's config folder:

```bash

    $ mkdir -p $HOME/.config/tizonia \
        && cp $TIZONIA_REPO_DIR/config/src/tizonia.conf $HOME/.config/tizonia

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

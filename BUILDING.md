<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->


- [Building from Source](#building-from-source)
  - [Dependencies](#dependencies)
  - [Building](#building)
    - ['Debug' variant](#debug-variant)
    - ['Release' variant](#release-variant)
    - [Single Debian package created with 'checkinstall'](#single-debian-package-created-with-checkinstall)
    - [The traditional method](#the-traditional-method)
    - [Conditional compilation of sub-projects](#conditional-compilation-of-sub-projects)
      - [Excluding the `player` sub-project](#excluding-the-player-sub-project)
      - [Excluding the `plugins/spotify_source` sub-project](#excluding-the-pluginsspotify_source-sub-project)
  - [Tizonia's configuration file](#tizonias-configuration-file)
  - [Resource Manager's D-BUS service activation file (optional)](#resource-managers-d-bus-service-activation-file-optional)
  - [Known issues](#known-issues)
  - [Speeding up (re-)compilation using ccache](#speeding-up-re-compilation-using-ccache)
  - [Creating a JSON compilation database, for use with e.g. Emacs RTags](#creating-a-json-compilation-database-for-use-with-eg-emacs-rtags)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## Building from Source ##

To build and install from source, follow these steps (Ubuntu 16.04 is assumed,
but should work on other recent Debian-based distros).

### Dependencies ###

To install all the development dependencies, the `tizonia-dev-build` tool is
the easiest way. This script lives under the `tools` directory and internally
maintains an up-to-date list of all the packages that are required in a
Debian-compatible system to build Tizonia from source.

> NOTE: The following command installs Mopidy's 'libspotify-dev' package, and
> the 'gmusicapi', 'soundcloud', 'pafy' and 'youtube-dl' Python PIP packages,
> plus a few other necessary debian packages.

```bash

    Setup the following environment variables
    $ export TIZONIA_REPO_DIR=/path/to/tizonia/repo # (e.g. /home/user/tizonia-openmax-il)
    $ export TIZONIA_INSTALL_DIR=/path/to/install/dir # (e.g. /usr or /home/user/temp)
    $ export PATH=$TIZONIA_REPO_DIR/tools:$PATH

    Now install everything that is required to build Tizonia from source (Debian derivative assumed)
    $ tools/tizonia-dev-build --deps

```

### Building

Once all the dependencies have been installed, build and install the OpenMAX IL
framework, all plugins and the 'tizonia' command-line application, using any of
the following methods.

#### 'Debug' variant

The following command re-configures all sub-projects with 'debug' type of
flags, and then proceeds to build and install them.

```bash

   $ tools/tizonia-dev-build --debug --install

```

#### 'Release' variant

The following command re-configures all sub-projects with 'release' type of
flags, builds and installs them.

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

#### The traditional method

Alternatively, from the top of Tizonia's repo, one can also do the familiar:

```bash

    $ autoreconf -ifs
    $ ./configure    # or ./configure --disable-player to disable the command-line player program
    $ make
    $ make install

```

#### Conditional compilation of sub-projects

##### Excluding the `player` sub-project

Some people are only interested in building the OpenMAX IL framework, without
the `tizonia` player application (that lives under the 'player'
sub-directory). During configuration, it can be disabled by including the
`--disable-player` option:

```bash

   # Disable compilation of the command-line player program.
   $ ./configure --disable-player

```

Alternatively, the `--no-player` option may be added to `tizonia-dev-build` to
disable configuration and build of the `tizonia` player.

```bash

   # Build and install in DEBUG mode without the command-line player program.
   $ tools/tizonia-dev-build --no-player --debug --install

```

##### Excluding the `plugins/spotify_source` sub-project

The `--without-libspotify` option may be included to disable configuration and
build of the libspotify-based OpenMAX IL component. This option will also
disable the support for this plugin in the `tizonia` player program.

```bash

   # Disable support for the spotify_source plugin.
   $ ./configure --without-libspotify

```

##### Excluding the `plugins/pcm_renderer_alsa` sub-project

The `--without-alsa` option may be included to disable configuration and
build of the ALSA-based OpenMAX IL pcm renderer.

```bash

   # Disable support for the ALSA pcm renderer plugin.
   $ ./configure --without-alsa

```

### Tizonia's configuration file ###

Copy *tizonia.conf* into the user's config folder:

```bash

    $ mkdir -p $HOME/.config/tizonia \
        && cp $TIZONIA_REPO_DIR/config/src/tizonia.conf $HOME/.config/tizonia

```

### Resource Manager's D-BUS service activation file (optional) ###

OpenMAX IL Resource Management is present but disabled by default. This is a
feature required on an compliant OpenMAX IL 1.2 implementation. Currently,
there is no other use beyond enabling OpenMAX IL compliance. In case this is to
be used during OpenMAX IL conformance testing (prior to that, it needs to be
explicitly enabled in tizonia.conf), copy the Resource Manager's D-BUS
activation file to some place where it can be found by the DBUS services. E.g:

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
'release' mode (especially if you are on a 32-bit distro).

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

### Speeding up (re-)compilation using ccache ###

[ccache](https://ccache.samba.org/) is a compiler cache. It speeds up
recompilation by caching previous compilations. With Tizonia, this is very
useful during development, especially since the 'player' application is so hard
to build due to the boost machinery.

On a debian system, ccache can be installed using:

```
$ sudo apt-get install ccache
```

Once ccache is installed, `tizonia-dev-build` will detect its presence and
start making use of it to (dramatically) reduce compilation time in most cases.

### Creating a JSON compilation database, for use with e.g. Emacs RTags ###

JSON compilation databases are used nowdays by many tools to provide
information on how a single compilation unit is processed. This helps these
programs to perform many useful tasks, like static analyses of various
kinds. [RTags](https://github.com/Andersbakken/rtags) is an example of program
that uses a JSON compilation database to index C/C++ code and keep a persistent
file-based database, for use within Emacs to provide powerful integrations.

`tizonia-dev-build` has support for `bear` (a program that creates JSON
databases) and RTags. To use, install both programs:

```
$ sudo apt-get install bear

# For RTags, visit its GitHub for up-to-date installation instructions
# https://github.com/Andersbakken/rtags

```

and finally, create the compilation db with these two commands:

```
$ tizonia-dev-build --debug (or --release)

# followed by

$ tizonia-dev-build --bear
```

After that, to keep the database up-to-date after code changes, use:

```
$ tizonia-dev-build --make (or --install)
```

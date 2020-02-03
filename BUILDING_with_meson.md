# Building from Source (with Meson) [RECOMMENDED]

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->

- [Prerequisites](#prerequisites)
- [Dependencies](#dependencies)
- [Building](#building)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

### Prerequisites

To build Tizonia using the Meson build system, you need at least Meson
'>=0.53.0'.

```
$ sudo -H pip3 install meson --upgrade
```

Ninja is the recommended Meson backend.
```
$ sudo apt-get ninja-build
```

### Dependencies

To install all the development dependencies, the `tizonia-dev-build` tool is
the recommended way. This script lives under the `tools` directory and internally
maintains an up-to-date list of all the packages that are required in a
Debian-compatible system to build Tizonia from source.

> NOTE: The following command installs Mopidy's 'libspotify-dev' package, and
> the 'gmusicapi', 'soundcloud', 'pafy' and 'youtube-dl' Python PIP packages,
> plus a few other necessary debian packages.

```bash
# Setup the following environment variables
$ export TIZONIA_REPO_DIR=/path/to/tizonia/repo # (e.g. /home/user/tizonia-openmax-il)
$ export TIZONIA_INSTALL_DIR=/path/to/install/dir # (e.g. /usr or /home/user/temp)
$ export PATH=$TIZONIA_REPO_DIR/tools:$PATH

# Set the PYTHONPATH accordingly (replace X to match the python version on your system)
$ export PYTHONPATH=$TIZONIA_INSTALL_DIR/lib/python3.X/site-packages:$PYTHONPATH

# Now install everything that is required to build Tizonia from source (Debian derivative assumed)
$ tizonia-dev-build --deps
```

### Building

Building Tizonia with [Meson](https://mesonbuild.com/) is easy.

First, declare flags you wish (in `CFLAGS`, `CXXFLAGS`, `FFLAGS` and `FCFLAGS`).
Then invoke `meson` with the various parameters you might want to specify, e.g:

```
$ meson --buildtype=plain --prefix=/usr --libdir=/usr/lib64 --libexecdir=/usr/lib \
--bindir=/usr/bin --sbindir=/usr/sbin --includedir=/usr/include --datadir=/usr/share \
--mandir=/usr/share/man --infodir=/usr/share/info --localedir=/usr/share/locale \
--sysconfdir=/etc --localstatedir=/var --sharedstatedir=/var/lib --wrap-mode=nodownload \
--auto-features=enabled . build
```

> NOTE: some distributions (e.g. Ubuntu) will use `/usr/lib` instead of
> `/usr/lib64` on some architectures, use your best judgement.

If you wish to set options (as per `meson_options.txt`), you can pass them with
`-D` to the configuration line.

Then build with:

```
$ ninja -v -j1 -C build
```

> NOTE: -j1 is recommended as it reduces the memory requirements during
> compilation.

And install with:
```
$ DESTDIR=<mydir> /usr/bin/ninja install -v -j1 -C build
```
On OSX things are still experimental. You'll have to install the required dependencies with brew and
therefore use `/usr/local` as prefix and adjust everything else accordingly. Also you will have to
pass `-Dpkg_config_path=/usr/local/lib` and `-Dcmake_prefix_path=/usr/local/lib`, plus other modifications.

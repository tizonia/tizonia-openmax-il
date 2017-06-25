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
- [Building Tizonia](#building-tizonia)
- [Changelog](#changelog)
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
release and all the dependencies, including [gmusicapi](https://github.com/simon-weber/gmusicapi), [soundcloud](https://github.com/soundcloud/soundcloud-python), [pafy](https://github.com/mps-youtube/pafy), [youtube-dl](https://github.com/rg3/youtube-dl), and a number of other supporting python packages.

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
    $ sudo -H pip install --upgrade youtube-dl pafy gmusicapi soundcloud simplejson pycrypto eyed3 Pykka pathlib

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

See [PROJECT.md](PROJECT.md) to learn more about the project.

# Building Tizonia

See [BUILDING.md](BUILDING.md) for instructions on how to build Tizonia from source.

# Changelog

See [CHANGELOG.md](CHANGELOG.md).

# License

Tizonia OpenMAX IL is released under the GNU Lesser General Public License
version 3.

# More information

For more information, please visit the project web site at http://www.tizonia.org

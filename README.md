<a href="https://tizonia.org/">
    <img src="https://avatars2.githubusercontent.com/u/3161606?s=400&v=4" alt="Tizonia logo" title="The Tizonia Project" align="right" height="100" />
</a>

# Tizonia

* A command-line music streaming client/server for Linux.
* With support for Spotify, Google Play Music (including Unlimited), YouTube,
  SoundCloud, and Dirble.
* A multimedia framework based on [OpenMAX IL 1.2](https://www.khronos.org/news/press/khronos-group-releases-openmax-il-1.2-provisional-specification).

---

[![Build Status](https://travis-ci.org/tizonia/tizonia-openmax-il.png)](https://travis-ci.org/tizonia/tizonia-openmax-il)  |  [![Coverity Scan Build Status](https://scan.coverity.com/projects/594/badge.svg)](https://scan.coverity.com/projects/594)  |  [![Documentation Status](https://readthedocs.org/projects/tizonia-openmax-il/badge/?version=latest)](https://readthedocs.org/projects/tizonia-openmax-il/?badge=latest) | [![Codecov](https://img.shields.io/codecov/c/github/tizonia/tizonia-openmax-il.svg)](https://codecov.io/gh/tizonia/tizonia-openmax-il) | [![GitHub commits](https://img.shields.io/github/commits-since/tizonia/tizonia-openmax-il/v0.11.0.svg)](https://github.com/tizonia/tizonia-openmax-il/compare/v0.11.0...master) | [![Codacy Badge](https://api.codacy.com/project/badge/Grade/b002a7f1ba464093b48fb7c9620f8ae7)](https://www.codacy.com/app/tizonia/tizonia-openmax-il?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tizonia/tizonia-openmax-il&amp;utm_campaign=Badge_Grade) | [![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/1359/badge)](https://bestpractices.coreinfrastructure.org/projects/1359) | [![license](https://img.shields.io/github/license/tizonia/tizonia-openmax-il.svg)](https://github.com/tizonia/tizonia-openmax-il/blob/master/COPYING.LESSER)

![alt text](https://github.com/tizonia/tizonia-openmax-il/blob/master/docs/animated-gifs/tizonia-usage-screencast.gif "Tizonia usage")

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [News](#news)
- [Installation](#installation)
  - [Debian / Ubuntu / Raspbian](#debian--ubuntu--raspbian)
  - [Arch User Repository (AUR)](#arch-user-repository-aur)
  - [Snap Package](#snap-package)
  - [Docker Image](#docker-image)
  - [Configuration](#configuration)
  - [Upgrade](#upgrade)
- [Roadmap](#roadmap)
- [Project](#project)
- [Building](#building)
- [Documentation](#documentation)
- [Changelog](#changelog)
- [License](#license)
- [More information](#more-information)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# News

:heavy_check_mark: [2017-12-28]
[Snap Package](#snap-package) and [Docker Image](#docker-image) available.

:heavy_check_mark: [2017-12-17] Tizonia
[v0.11.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.11.0). Bug-fix release.

:heavy_check_mark: [2017-10-22] Tizonia
[v0.10.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.10.0). Bug-fix release.

:heavy_check_mark: [2017-08-26] [tizonia-all](https://aur.archlinux.org/packages/tizonia-all/) and [tizonia-all-git](https://aur.archlinux.org/packages/tizonia-all-git/) packages submitted to the [Arch User Repository](https://aur.archlinux.org/)

:heavy_check_mark: [2017-05-04] [Summer of Code 2017: Add OpenMAX state tracker
in Mesa/Gallium that uses
Tizonia](https://summerofcode.withgoogle.com/projects/#4737166321123328) (X.Org
Foundation project, with Gurkirpal Singh and Julien Isorce).

# Installation

## Debian / Ubuntu / Raspbian

Available from [Bintray](https://bintray.com/tizonia), with the following distro/arch
combinations:

| Ubuntu Trusty (14.04) | Ubuntu Xenial (16.04) | Ubuntu Bionic (18.04) | Debian Jessie (8) | Raspbian Jessie (8) | Debian Stretch (9) | Raspbian Stretch (9) |
|        :---:          |        :---:          |        :---:          |        :---:      |       :---:         |        :---:       |        :---:         |
|        amd64          |     amd64, armhf      |      Coming Soon      |    amd64, armhf   |       armhf         |     amd64, armhf   |        armhf         |
| [ ![Download](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-trusty/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-trusty/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-xenial/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-xenial/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-bionic/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-bionic/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/debian/tizonia-jessie/images/download.svg) ](https://bintray.com/tizonia/debian/tizonia-jessie/_latestVersion)  | [ ![Download](https://api.bintray.com/packages/tizonia/raspbian/tizonia-jessie/images/download.svg) ](https://bintray.com/tizonia/raspbian/tizonia-jessie/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/debian/tizonia-stretch/images/download.svg) ](https://bintray.com/tizonia/debian/tizonia-stretch/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/raspbian/tizonia-stretch/images/download.svg) ](https://bintray.com/tizonia/raspbian/tizonia-stretch/_latestVersion) |

> NOTE: Elementary OS and Linux Mint are supported on releases based on Ubuntu 'Trusty' or Ubuntu 'Xenial'.

Please note that the **recommended** way to install Tizonia in a
Debian-compatible system is running the following command, that installs the
[latest](https://github.com/tizonia/tizonia-openmax-il/releases/latest) release
along with all its dependencies.

```bash

    $ curl -kL https://github.com/tizonia/tizonia-openmax-il/raw/master/tools/install.sh | bash
    # Or its shortened version:
    $ curl -kL https://goo.gl/Vu8qGR | bash

```

> NOTE: The usual disclaimers apply: trust no-one. You should have a look at
> the installation script before running it on your system!.

## Arch User Repository (AUR)
 - [tizonia-all (0.11.0)](https://aur.archlinux.org/packages/tizonia-all/)
 - [tizonia-all-git](https://aur.archlinux.org/packages/tizonia-all-git/)

```bash

    $ yaourt -S tizonia-all # for the latest stable release
    # or
    $ yaourt -S tizonia-all-git # for the bleeding edge

```

## Snap Package

A 'snap' package is now available to download from the 'Global' snap store
('stable' channel). For more details visit:

- Tizonia's landing page on [Snapcraft.io](https://snapcraft.io/tizonia)
or
- Tizonia on [uApp Explorer](https://uappexplorer.com/snap/ubuntu/tizonia)

To install, first visit [Install
Snapd](https://docs.snapcraft.io/core/install?_ga=2.41936226.1106178805.1514500852-128158267.1514500852)
to make sure that your Linux distro is supported. Follow the instructions there
to get the 'snapd' service running on your system, and finally use this command
to install Tizonia:

```bash

$ sudo snap install tizonia

```

Tizonia's snapcraft.yaml file is hosted on this repository:

- [tizonia-snap](https://github.com/tizonia/tizonia-snap/)

## Docker Image

Tizonia can also be run from a Docker container. A Docker image is available from the Docker hub:

- [docker-tizonia](https://hub.docker.com/r/tizonia/docker-tizonia/)

## Configuration

To use *Spotify*, *Google Play Music*, *SoundCloud*, and *Dirble*, introduce
your credentials in Tizonia's config file (see instructions inside the file for
more information):

```bash

    $ mkdir -p $HOME/.config/tizonia
    $ cp /etc/tizonia/tizonia.conf $HOME/.config/tizonia/tizonia.conf

    ( now edit $HOME/.config/tizonia/tizonia.conf )

    ( NOTE: If you have installed the 'snap' package, use the following commands instead. )

    $ mkdir -p $HOME/snap/tizonia/current/.config/tizonia
    $ cp /var/lib/snapd/snap/tizonia/current/etc/xdg/tizonia/tizonia.conf $HOME/snap/tizonia/current/.config/tizonia

    ( now edit $HOME/snap/tizonia/current/.config/tizonia )

```

## Upgrade

To upgrade Tizonia, run 'apt-get' as usual, but also make sure the Python dependencies are up-to-date.

```bash

    $ sudo apt-get update && sudo apt-get upgrade

    # (Note that new versions of some of these Python packages are released frequently)
    $ sudo -H pip install --upgrade gmusicapi soundcloud youtube-dl pafy pycountry titlecase

```

# Roadmap

- Chromecast support.
- YouTube live streams support.
- REPL command-line interface.
- OS X port.

# Project

See [PROJECT.md](PROJECT.md) to learn more about this project.

# Building

See [BUILDING.md](BUILDING.md) for instructions on how to build Tizonia from source.

# Documentation

See [docs.tizonia.org](http://docs.tizonia.org/) for the most up-to-date project documentaion.

The [Wiki](https://github.com/tizonia/tizonia-openmax-il/wiki) may also contain
some useful information (although in need of an update).

# Changelog

See [CHANGELOG.md](CHANGELOG.md) for details on what has gone into each
release of the project.

# License

Tizonia OpenMAX IL is released under the GNU Lesser General Public License
version 3.

# More information

For more information, please visit the project web site at http://www.tizonia.org

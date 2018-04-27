<a href="https://tizonia.org/">
    <img src="https://avatars2.githubusercontent.com/u/3161606?s=400&v=4" alt="Tizonia logo" title="The Tizonia Project" align="right" height="100" />
</a>

# The Tizonia Project

* A command-line music streaming client/server for Linux.
* Support for Spotify (Premium), Google Play Music (free and paid tiers), YouTube,
  SoundCloud, Dirble, Plex servers and Chromecast devices.
* The first open-source implementation of [OpenMAX IL 1.2](https://www.khronos.org/news/press/khronos-group-releases-openmax-il-1.2-provisional-specification).

---

[![Build Status](https://travis-ci.org/tizonia/tizonia-openmax-il.png)](https://travis-ci.org/tizonia/tizonia-openmax-il)  |  [![Coverity Scan Build Status](https://scan.coverity.com/projects/594/badge.svg)](https://scan.coverity.com/projects/594)  | [![Codecov](https://img.shields.io/codecov/c/github/tizonia/tizonia-openmax-il.svg)](https://codecov.io/gh/tizonia/tizonia-openmax-il) | [![GitHub commits](https://img.shields.io/github/commits-since/tizonia/tizonia-openmax-il/v0.14.0.svg)](https://github.com/tizonia/tizonia-openmax-il/compare/v0.14.0...master) | [![Codacy Badge](https://api.codacy.com/project/badge/Grade/b002a7f1ba464093b48fb7c9620f8ae7)](https://www.codacy.com/app/tizonia/tizonia-openmax-il?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tizonia/tizonia-openmax-il&amp;utm_campaign=Badge_Grade) | [![license](https://img.shields.io/github/license/tizonia/tizonia-openmax-il.svg)](https://github.com/tizonia/tizonia-openmax-il/blob/master/COPYING.LESSER) | [![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/1359/badge)](https://bestpractices.coreinfrastructure.org/projects/1359) | [![Gitter chat](https://badges.gitter.im/tizonia/tizonia-openmax-il.png)]

<img src="https://github.com/tizonia/tizonia-openmax-il/blob/master/docs/animated-gifs/tizonia-usage-screencast.gif" width="440"/> <img src="https://github.com/tizonia/tizonia-openmax-il/blob/master/docs/animated-gifs/tizonia-usage-screencast2.gif" width="440"/>

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->

- [News](#news)
- [Installation](#installation)
  - [Debian / Ubuntu / Raspbian](#debian--ubuntu--raspbian)
  - [Arch User Repository (AUR)](#arch-user-repository-aur)
  - [Snap Package](#snap-package)
  - [Docker Image](#docker-image)
  - [Configuration](#configuration)
  - [Upgrade](#upgrade)
- [Roadmap](#roadmap)
- [Resources](#resources)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# News

- [2018-04-20] Tizonia
[v0.14.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.14.0). Added
support for Google Play Music [stations for
non-subscribers](https://support.google.com/googleplaymusic/answer/6250894?hl=en)
and YouTube channels (uploads and playlists). Also added option to play the
user's entire Google Play Music library.

- [2018-03-09] Tizonia
[v0.13.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.13.0). Added
support for Plex servers. Various improvements and bug fixes in Google Music,
SoundCloud, Dirble and Spotify.

- [2018-02-11] Tizonia
[v0.12.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.12.0). Chromecast
support is now available for Google Play Music, YouTube, SoundCloud, and Dirble
playlists and standalone HTTP radio stations (casting of Spotify and local
media to be included in a future release).

- [2017-12-28]
[Snap Package](#snap-package) and [Docker Image](#docker-image) available.

- [2017-08-26] [tizonia-all](https://aur.archlinux.org/packages/tizonia-all/) and [tizonia-all-git](https://aur.archlinux.org/packages/tizonia-all-git/) packages submitted to the [Arch User Repository](https://aur.archlinux.org/)

- [2017-05-04] [Summer of Code 2017: Add OpenMAX state tracker
in Mesa/Gallium that uses
Tizonia](https://summerofcode.withgoogle.com/projects/#4737166321123328) (X.Org
Foundation project, with Gurkirpal Singh and Julien Isorce).

# Installation

## Debian / Ubuntu / Raspbian

Available from [Bintray](https://bintray.com/tizonia), with the following distro/arch
combinations:

| Ubuntu Xenial (16.04) | Ubuntu Bionic (18.04) | Debian Stretch (9) | Raspbian Stretch (9) | Debian Buster (10) |
|        :---:          |        :---:          |        :---:       |        :---:         |       :---:        |
|     amd64, armhf      |     amd64, armhf      |     amd64, armhf   |        armhf         |       amd64        |
| [ ![Download](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-xenial/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-xenial/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-bionic/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-bionic/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/debian/tizonia-stretch/images/download.svg) ](https://bintray.com/tizonia/debian/tizonia-stretch/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/raspbian/tizonia-stretch/images/download.svg) ](https://bintray.com/tizonia/raspbian/tizonia-stretch/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tizonia/debian/tizonia-buster/images/download.svg) ](https://bintray.com/tizonia/debian/tizonia-buster/_latestVersion) |

> NOTE: Elementary OS and Linux Mint are (or will be) supported on releases based on Ubuntu 'Xenial' or 'Bionic'.

Please note that the **recommended** way to install Tizonia on a
Debian-compatible system is by running the following command:

```bash

    $ curl -kL https://github.com/tizonia/tizonia-openmax-il/raw/master/tools/install.sh | bash

    # Or its shortened version:

    $ curl -kL https://goo.gl/Vu8qGR | bash

```
> NOTE: This script installs the [latest](https://github.com/tizonia/tizonia-openmax-il/releases/latest)
> release and all the dependencies.

> NOTE: The usual disclaimers apply: trust no-one. You should have a look at
> the installation script before running it on your system!.

## Arch User Repository (AUR)

- [tizonia-all (for the latest released version)](https://aur.archlinux.org/packages/tizonia-all/)
- [tizonia-all-git (HEAD of master branch)](https://aur.archlinux.org/packages/tizonia-all-git/)

> NOTE [24/03/2018]: AUR packages are now updated and tracking v0.14.0.

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
and make sure that your Linux distro is supported. Follow the instructions
to get the 'snapd' service running on your system, and finally use this command
to install Tizonia:

```bash

$ sudo snap install tizonia

```

Tizonia's snapcraft.yaml file is hosted in its own repository:

- [tizonia-snap](https://github.com/tizonia/tizonia-snap/)

## Docker Image

Tizonia can also be run from a Docker container. A Docker image is available from the Docker hub:

- [docker-tizonia](https://hub.docker.com/r/tizonia/docker-tizonia/)

## Configuration

To use *Spotify*, *Google Play Music*, *SoundCloud*, *Dirble* and *Plex* introduce
your credentials in Tizonia's config file (see instructions inside this file for
more information):

```bash
    ( On first use, Tizonia outputs its configuration file, if it is not there yet )

    $ tizonia --help

    ( now edit $HOME/.config/tizonia/tizonia.conf )

    ( NOTE: If Tizonia was installed from the 'snap' package, use this path instead )
    ( $HOME/snap/tizonia/current/.config/tizonia/tizonia.conf )


```

## Upgrade

To upgrade Tizonia, run 'apt-get' as usual, but also make sure the Python dependencies are up-to-date.

```bash

    $ sudo apt-get update && sudo apt-get upgrade

    # (Note that new versions of some of these Python packages are released frequently)
    $ sudo -H pip install --upgrade gmusicapi soundcloud youtube-dl pafy pycountry titlecase pychromecast plexapi fuzzywuzzy eventlet

```

# Roadmap

- Pandora support.
- Tidal support
- Support for YouTube live streams.
- REPL command-line interface.
- OS X port.

# Resources

- See [PROJECT.md](PROJECT.md) to discover other facts about the project.

- See [CONTRIBUTING.md](CONTRIBUTING.md) to learn how to contribute to Tizonia.

- See [BUILDING.md](BUILDING.md) for instructions on how to build Tizonia from source.

- See [docs.tizonia.org](http://docs.tizonia.org/) for the project's official documentation.

- The [Wiki](https://github.com/tizonia/tizonia-openmax-il/wiki) may also
contain some useful information (although in need of an update).

- See [CHANGELOG.md](CHANGELOG.md) for details on what has gone into each
release of the project.

- Tizonia OpenMAX IL is released under the [GNU Lesser General Public
License](COPYING.LESSER) version 3.

- Finally, please visit the project web site at http://www.tizonia.org

<a href="https://tizonia.org/">
    <img src="https://avatars2.githubusercontent.com/u/3161606?s=400&v=4" alt="Tizonia logo" title="The Tizonia Project" align="right" height="100" />
</a>

# The Tizonia Project

* A command-line streaming music client/server for Linux.
* Support for Spotify (Premium), Google Play Music (free and paid tiers), YouTube,
  SoundCloud, TuneIn Internet Radio, Plex servers and Chromecast devices.
* The first open-source implementation of [OpenMAX IL
  1.2](https://www.khronos.org/news/press/khronos-group-releases-openmax-il-1.2-provisional-specification).

---

<div align="center">
  <a href="https://travis-ci.org/tizonia/tizonia-openmax-il">
    <img src="https://travis-ci.org/tizonia/tizonia-openmax-il.png" />
  </a>

  <a href="https://scan.coverity.com/projects/594">
    <img src="https://scan.coverity.com/projects/594/badge.svg" />
  </a>

  <a href="https://codecov.io/gh/tizonia/tizonia-openmax-il">
    <img src="https://img.shields.io/codecov/c/github/tizonia/tizonia-openmax-il.svg" />
  </a>

  <a href="https://github.com/tizonia/tizonia-openmax-il/compare/v0.21.0...master">
    <img src="https://img.shields.io/github/commits-since/tizonia/tizonia-openmax-il/v0.21.0.svg" />
  </a>

</div>

<div align="center">

  <a href="https://www.codacy.com/app/tizonia/tizonia-openmax-il?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tizonia/tizonia-openmax-il&amp;utm_campaign=Badge_Grade">
    <img src="https://api.codacy.com/project/badge/Grade/b002a7f1ba464093b48fb7c9620f8ae7" />
  </a>

  <a href="https://github.com/tizonia/tizonia-openmax-il/blob/master/COPYING.LESSER">
    <img src="https://img.shields.io/github/license/tizonia/tizonia-openmax-il.svg" />
  </a>

  <a href="https://bestpractices.coreinfrastructure.org/projects/1359">
    <img src="https://bestpractices.coreinfrastructure.org/projects/1359/badge" />
  </a>

</div>

<div align="center">

  <a href="https://gitter.im/tizonia/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=body_badge">
    <img src="https://badges.gitter.im/tizonia/tizonia-openmax-il.svg" />
  </a>

  <a href="https://github.com/tizonia/tizonia-openmax-il/issues">
    <img src="https://img.shields.io/badge/contributions-welcome-brightgreen.svg?style=plastic" />
  </a>

</div>

<div align="center">
  <h3>
    <a href="https://tizonia.org">
      Website
    </a>
    <span> | </span>
    <a href="https://docs.tizonia.org">
      Documentation
    </a>
    <span> | </span>
    <a href="https://github.com/tizonia/tizonia-openmax-il/contribute">
      Contributing
    </a>
    <span> | </span>
    <a href="https://gitter.im/tizonia/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=body_badge">
      Chat
    </a>
  </h3>
</div>

<div align="center">
  <sub>By
  <a href="https://juanrubio.org">Juan A. Rubio</a> and
  <a href="https://github.com/tizonia/tizonia-openmax-il/graphs/contributors">
    contributors
  </a>
</div>

---


<div align="center">
  <img src="https://raw.githubusercontent.com/tizonia/tizonia-openmax-il/develop/docs/animated-gifs/tizonia-usage-screencast.gif" />
</div>

<!-- [![](https://raw.githubusercontent.com/tizonia/tizonia-openmax-il/master/docs/animated-gifs/tizonia-usage-screencast2.gif)](https://raw.githubusercontent.com/tizonia/tizonia-openmax-il/master/docs/animated-gifs/tizonia-usage-screencast2.gif) -->

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
  - [Debian / Ubuntu / Raspbian](#debian--ubuntu--raspbian-1)
- [Hall of Fame](#hall-of-fame)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# News

**Latest news**

- [2020-03-08] Tizonia
[v0.21.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.21.0). TODO.

> See Tizonia's website: [TODO](https://tizonia.org/news/2020/02/20/release-0-20-0/)

<details><summary><b>Show more</b></summary>

- [2020-01-19] Tizonia
[v0.20.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.20.0). Added
support for TuneIn Internet radios, color-themes and a new build system based
on Meson (thanks [@lgbaldoni!](https://github.com/lgbaldoni)). A man page has
been added and the documentation site has been refreshed. Also `tizonia-remote`
is now distributed in the Debian package plus a good number of fixes, including
reviving Chromecast support (still more work needed).

> See Tizonia's website: [TuneIn Internet radio and podcasts, and color-themes
> in Tizonia 0.20.0](https://tizonia.org/news/2020/02/20/release-0-20-0/)

- [2019-12-13] Tizonia
[v0.19.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.19.0). Another
maintenance release with a good bunch of improvements and bug fixes: Dirble
removal (the service is sadly gone), Python 3 migration (please see the section
[Upgrade (Debian / Ubuntu / Raspbian)](#upgrade-debian--ubuntu--raspbian) to learn
how to install the new Python 3 dependencies!).

> See Tizonia's website: [Global Spotify playlist search and
> other improvements in Tizonia
> 0.19.0](https://tizonia.org/news/2019/03/19/release-0-19-0/)

- [2019-03-13] Tizonia
[v0.18.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.18.0). Various
improvements and bug fixes in Google Music and Spotify.

> See Tizonia's website: [Fixed 'Google Play Music tracks cut short' in Tizonia 0.18.0](https://tizonia.org/news/2019/03/17/release-0-18-0/)

- [2019-01-17] Tizonia
[v0.17.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.17.0). Fixed
Spotify login issues. A regression introduced in v0.16.0. This issue was
identified and fixed thanks to the great feedback provided by the users in
issue [#531](https://github.com/tizonia/tizonia-openmax-il/issues/531).

> See Tizonia's website: [Spotify login issues fixed in Tizonia 0.17.0](https://tizonia.org/news/2019/01/17/release-0-17-0/)

- [2018-12-03] Tizonia
[v0.16.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.16.0). Improved
Spotify support with more options to dicover new music rather than just playing
the content that you know and love. Last but not least,
[docker-tizonia](https://hub.docker.com/r/tizonia/docker-tizonia/) has been
updated! (many thanks to [Josh5](https://github.com/Josh5)).

> See Tizonia's website: [More ways to discover music on Spotify with Tizonia 0.16.0](https://tizonia.org/news/2018/12/03/release-0-16-0/)

- [2018-06-15] Tizonia
[v0.15.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.15.0). Reworked
Spotify support to overcome playlist search problems that arised in
libspotify. Now [spotipy](https://github.com/plamere/spotipy) is being used to
retrieve track, artist, album, and playlist metadata from Spotify.

> See Tizonia's website: [Totally revamped Spotify support in Tizonia 0.15.0](https://tizonia.org/news/2018/06/15/release-0-15-0/)

- [2018-04-20] Tizonia
[v0.14.0](https://github.com/tizonia/tizonia-openmax-il/releases/tag/v0.14.0). Added
support for Google Play Music [stations for
non-subscribers](https://support.google.com/googleplaymusic/answer/6250894?hl=en)
and YouTube channels (uploads and playlists). Also added option to play the
user's entire Google Play Music library.

> See Tizonia's website: [Tizonia v0.14.0 adds YouTube Channels and Google Play Music stations for non-subscribers](https://tizonia.org/news/2018/04/21/release-0-14-0/)

- [2017-12-28]
[Snap Package](#snap-package) and [Docker Image](#docker-image) available.

> See Tizonia's website: [Tizonia v0.13.0 adds support for Plex media servers](https://tizonia.org/news/2018/03/09/release-0-13-0/)

- [2017-08-26] [tizonia-all](https://aur.archlinux.org/packages/tizonia-all/) and [tizonia-all-git](https://aur.archlinux.org/packages/tizonia-all-git/) packages submitted to the [Arch User Repository](https://aur.archlinux.org/)

> See Tizonia's website: [Snap package and Docker image available now!](https://tizonia.org/news/2017/12/30/snap-package-and-docker-image/)

- [2017-05-04] [Summer of Code 2017: Add OpenMAX state tracker
in Mesa/Gallium that uses
Tizonia](https://summerofcode.withgoogle.com/projects/#4737166321123328) (X.Org
Foundation project, with Gurkirpal Singh and Julien Isorce).

</details>

&nbsp;&nbsp;

# Installation

## Debian / Ubuntu / Raspbian

Run the following command to install Tizonia on a Debian-compatible system:

```bash
$ curl -kL https://github.com/tizonia/tizonia-openmax-il/raw/master/tools/install.sh | bash
# Or its shortened version:
$ curl -kL https://goo.gl/Vu8qGR | bash
```

> NOTE: This script installs the
> [latest](https://github.com/tizonia/tizonia-openmax-il/releases/latest)
> release and all the dependencies.

> DISCLAIMER: Trust no-one. Please have a look at the installation script
> before running it on your system!.


Debian packages are stored in [Bintray](https://bintray.com/tizonia), with the
following distro/arch combinations:

<div align="center">
    <table>
        <thead>
            <tr>
                <th align="center">Ubuntu Xenial</th>
                <th align="center">Ubuntu Bionic</th>
                <th align="center">Debian Buster</th>
                <th align="center">Debian Bullseye</th>
                <th align="center">Raspbian Buster</th>
            </tr>
            <tr>
                <th align="center">(16.04)</th>
                <th align="center">(18.04)</th>
                <th align="center">(10)</th>
                <th align="center">(11)</th>
                <th align="center">(10)</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td align="center">amd64</td>
                <td align="center">amd64, armhf</td>
                <td align="center">amd64, armhf</td>
                <td align="center">amd64</td>
                <td align="center">armhf</td>
            </tr>
        </tbody>
    </table>
</div>

<!-- | [ ![](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-xenial/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-xenial/_latestVersion) | [ ![](https://api.bintray.com/packages/tizonia/ubuntu/tizonia-bionic/images/download.svg) ](https://bintray.com/tizonia/ubuntu/tizonia-bionic/_latestVersion) | [ ![](https://api.bintray.com/packages/tizonia/debian/tizonia-buster/images/download.svg) ](https://bintray.com/tizonia/debian/tizonia-buster/_latestVersion) | [ ![](https://api.bintray.com/packages/tizonia/raspbian/tizonia-buster/images/download.svg) ](https://bintray.com/tizonia/raspbian/tizonia-buster/_latestVersion) | [ ![](https://api.bintray.com/packages/tizonia/debian/tizonia-bullseye/images/download.svg) ](https://bintray.com/tizonia/debian/tizonia-bullseye/_latestVersion) | -->


> NOTE: Elementary OS, Linux Mint, Kali Linux are supported on releases based
> on Ubuntu 'Xenial' or 'Bionic', Debian 'Buster' or 'Bullseye'. To install
> Tizonia on other versions of Debian or Ubuntu-based distros, use the Snap
> package or have a look at
> [#631](https://github.com/tizonia/tizonia-openmax-il/issues/631).


## Arch User Repository (AUR)

- [tizonia-all (for the latest released version)](https://aur.archlinux.org/packages/tizonia-all/)
- [tizonia-all-git (HEAD of master branch)](https://aur.archlinux.org/packages/tizonia-all-git/)

<details><summary><b>Show details</b></summary>

```bash
# Please note that if you are upgrading your existing
# Tizonia installation, you *need* to uninstall it before building a new version.
# See GitHub issue https://github.com/tizonia/tizonia-openmax-il/issues/485

# For the latest stable release
$ git clone https://aur.archlinux.org/tizonia-all.git
$ cd tizonia-all
$ makepkg -si

# There is also a -git package:
$ git clone https://aur.archlinux.org/tizonia-all-git.git
$ cd tizonia-all
$ makepkg -si

```

</details>


## Snap Package

A 'snap' package is now available to download from the 'Global' snap store
('stable' channel). For more details visit:

- Tizonia's landing page on [Snapcraft.io](https://snapcraft.io/tizonia)

<details><summary><b>Show details</b></summary>

To install, first visit [Install
Snapd](https://docs.snapcraft.io/core/install?_ga=2.41936226.1106178805.1514500852-128158267.1514500852)
and make sure that your Linux distro is supported. Follow the instructions to
get the 'snapd' service running on your system, and finally use this command to
install Tizonia:

```bash

$ sudo snap install tizonia

```

</details>


Tizonia's snapcraft.yaml file is hosted in a separate repository:

- [tizonia-snap](https://github.com/tizonia/tizonia-snap/)


## Docker Image

Tizonia may also be run from a Docker container. A Docker image is available
from the Docker hub:

- [docker-tizonia](https://hub.docker.com/r/tizonia/docker-tizonia/)

&nbsp;&nbsp;

# Configuration

To use *Spotify*, *Google Play Music*, *SoundCloud* and *Plex* you need to
introduce your credentials in Tizonia's config file. No credentials needed to
stream from *YouTube* or *TuneIn*.

<details><summary><b>Show details</b></summary>

```bash
( On first use, Tizonia outputs its configuration file, if it is not there yet )

$ tizonia --help

( now edit $HOME/.config/tizonia/tizonia.conf )

( NOTE: If Tizonia was installed from the 'snap' package, use this path instead )
( $HOME/snap/tizonia/current/.config/tizonia/tizonia.conf )
```

> NOTE: See full instructions inside [tizonia.conf](https://docs.tizonia.org/manual/config.html).

</details>

&nbsp;&nbsp;

# Upgrade

## Debian / Ubuntu / Raspbian

To upgrade Tizonia and all its dependencies, simply re-run the installation
script.

If you prefer to do it manually, it is a two-step process. Run 'apt-get' as
usual to upgrade the Debian packages. Finally make sure that the various Python
dependencies are up-to-date.

<details><summary><b>Show details</b></summary>

```bash

# Step1: update Tizonia's Debian packages
$ sudo apt-get update && sudo apt-get upgrade

# Step2: update Tizonia's Python dependencies
# (Note that new versions of some of these Python dependencies are released often,
# so you should do this frequently, even if there isn't a new Tizonia release)

# For Tizonia v0.19.0 or newer: Python 3 dependencies
$ sudo -H pip3 install --upgrade gmusicapi soundcloud youtube-dl pafy pycountry titlecase pychromecast plexapi fuzzywuzzy eventlet python-Levenshtein && sudo -H pip3 install git+https://github.com/plamere/spotipy.git --upgrade

# For Tizonia v0.18.0 or older: Python 2 dependencies
$ sudo -H pip2 install --upgrade gmusicapi soundcloud youtube-dl pafy pycountry titlecase pychromecast plexapi fuzzywuzzy eventlet python-Levenshtein && sudo -H pip2 install git+https://github.com/plamere/spotipy.git --upgrade

```

</details>

&nbsp;&nbsp;

# Hall of Fame

If you are interested in participating, please read our [contribution
guidelines](CONTRIBUTING.md) and don't hesitate to ask (via the bug tracker,
[chat](https://gitter.im/tizonia/Lobby) or any other means!).

Here are some of our contributors:

[![](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/images/0)](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/links/0)[![](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/images/1)](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/links/1)[![](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/images/2)](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/links/2)[![](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/images/3)](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/links/3)[![](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/images/4)](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/links/4)[![](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/images/5)](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/links/5)[![](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/images/6)](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/links/6)[![](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/images/7)](https://sourcerer.io/fame/tizonia/tizonia/tizonia-openmax-il/links/7)


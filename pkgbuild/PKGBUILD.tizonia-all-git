# Maintainer: juanrubio

_githubname=tizonia-openmax-il
pkgname=tizonia-all-git
pkgver=0.21.0.r4.g920246f5
pkgrel=1
pkgdesc="Command-line cloud music player for Linux with support for Spotify, Google Play Music, YouTube, SoundCloud, Plex servers and Chromecast devices."
arch=('x86_64')
url="https://www.tizonia.org"
license=('LGPL')
makedepends=('git' 'boost' 'check' 'meson' 'samurai')
depends=(
    # official repositories:
    'libmad'
    'taglib'
    'mediainfo'
    'sdl'
    'sqlite'
    'lame'
    'faad2'
    'libcurl-gnutls'
    'libvpx'
    'mpg123'
    'opusfile'
    'libfishsound'
    'liboggz'
    'youtube-dl'
    'libpulse'
    'boost-libs'
    'hicolor-icon-theme'
    'python-eventlet'
    'python-levenshtein'

    # AUR:
    'log4c'
    'libspotify'
    'python-pafy'
    'python-gmusicapi'
    'python-soundcloud-git'
    'python-pychromecast-git'
    'python-plexapi'
    'python-fuzzywuzzy'
    'python-spotipy'
)
provides=('tizonia-all')
conflicts=('tizonia-all')
options=()
source=("${pkgname}"::git+https://github.com/tizonia/${_githubname}.git)
sha256sums=('SKIP')

pkgver() {
    cd "$pkgname"
    local _version="$(git tag | sort -Vr | head -n1 | sed 's/^v//')"
    local _revision="$(git rev-list v"${_version}"..HEAD --count)"
    local _shorthash="$(git rev-parse --short HEAD)"
    printf '%s.r%s.g%s' "$_version" "$_revision" "$_shorthash"
}

# prepare() {
  #command -v tizonia &> /dev/null \
  #    && { \
  #    echo >&2 "Please uninstall tizonia-all or tizonia-all-git before proceeding." ; \
  #    echo >&2 "See https://github.com/tizonia/tizonia-openmax-il/issues/485." ; \
  #    exit 1;
  #    }
#}

build() {
        CFLAGS='-O2 -s -DNDEBUG' \
        CXXFLAGS='-O2 -s -DNDEBUG -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security' \
        export SAMUFLAGS=-j4
        arch-meson "$pkgname" build
        samu -v -C build
}

package() {
    DESTDIR=$pkgdir samu -C build install
}

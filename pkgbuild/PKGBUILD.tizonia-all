# Maintainer: juanrubio

_githubname=tizonia-openmax-il
pkgname=tizonia-all
pkgver=0.11.0
pkgrel=1
pkgdesc="Command-line cloud music player for Linux with Spotify, Google Play Music, YouTube, SoundCloud, and Dirble support"
arch=('x86_64')
url="https://www.tizonia.org"
license=('LGPL')
depends=(
    # official repositories:
    'libmad'
    'sqlite'
    'libutil-linux'
    'taglib'
    'mediainfo'
    'sdl'
    'lame'
    'faad2'
    'libcurl-gnutls'
    'libvorbis'
    'libvpx'
    'mpg123'
    'opus'
    'opusfile'
    'libogg'
    'libfishsound'
    'flac'
    'liboggz'
    'libsndfile'
    'alsa-lib'
    'libpulse'
    'boost'
    'check'
    'python2-pafy'

    # AUR:
    'log4c'
    'libspotify'
    'python2-gmusicapi'
    'python2-soundcloud-git'
    'python2-youtube-dl-git'
    'python2-pychromecast-git'
)
source=("${_githubname}-${pkgver}.tar.gz"::"https://github.com/tizonia/${_githubname}/archive/v${pkgver}.tar.gz")
md5sums=('fe53ce96f527244bb3286bc4c7a632af')

prepare() {
  mkdir -p "$srcdir/path"
  # Tizonia expects Python 2
  ln -sf /usr/bin/python2 "$srcdir/path/python"
  ln -sf /usr/bin/python2-config "$srcdir/path/python-config"
}

build() {
    export PATH="$srcdir/path:$PATH"
    export PYTHON="/usr/bin/python2"
    cd "${_githubname}-${pkgver}"
    autoreconf -ifs
    ./configure \
        --prefix=/usr \
        --sysconfdir=/etc \
        --localstatedir=/var \
        --silent \
        --enable-silent-rules \
        CFLAGS='-O2 -s -DNDEBUG' \
        CXXFLAGS='-O2 -s -DNDEBUG -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security'
    make
}

package() {
    cd "${_githubname}-${pkgver}"
    make DESTDIR="$pkgdir/" install
}

# Maintainer: juanrubio

pkgname=python2-soundcloud-git
pkgver=r111.d77f6e5
pkgrel=1
pkgdesc="A Python wrapper around the Soundcloud API"
arch=("i686" "x86_64")
url="https://github.com/soundcloud/soundcloud-python"
license=("BSD")
depends=("python2-requests")
makedepends=("git" "python2-setuptools")
optdepends=("python2-simplejson")
provides=("python2-soundcloud")
source=("$pkgname"::"git://github.com/soundcloud/soundcloud-python")
md5sums=("SKIP")

pkgver() {
    cd "$srcdir/$pkgname"
    printf "r%s.%s" "$(git rev-list --count HEAD)" \
        "$(git rev-parse --short HEAD)"
}

build() {
    cd "$srcdir/$pkgname"
    python2 setup.py build
}

package() {
    cd "$srcdir/$pkgname"
    python2 setup.py install --root="$pkgdir" --optimize=1
}


# Ubuntu 24.04 amd64 build container

This directory defines the v1 baseline build environment for Ubuntu 24.04
`amd64`. It uses only Ubuntu archive packages listed in `apt-packages.txt` and
does not add external APT repositories or install Python packages outside the
Ubuntu archive.

## Build the image

From the repository root:

```bash
docker build \
  --platform linux/amd64 \
  -t tizonia-build:ubuntu-24.04 \
  -f ci/ubuntu-24.04/Dockerfile \
  ci/ubuntu-24.04
```

Documentation tooling is excluded by default. To include the Ubuntu-packaged
Sphinx and Doxygen dependencies for a docs-enabled build, add:

```bash
--build-arg TIZONIA_INSTALL_DOCS=true
```

## Enter the environment

From the repository root:

```bash
docker run --rm -it \
  --platform linux/amd64 \
  --user "$(id -u):$(id -g)" \
  -e HOME=/tmp \
  -v "$PWD":/work/tizonia-openmax-il \
  -w /work/tizonia-openmax-il \
  tizonia-build:ubuntu-24.04
```

## Run the baseline build

Inside the container:

```bash
meson setup build -Dlibspotify=false -Ddocs=false
ninja -C build -j1
```

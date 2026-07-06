# Ubuntu 24.04 v1 Build Container

This directory defines the Ubuntu 24.04 `amd64` baseline environment for v1
source builds. The image uses only Ubuntu archive packages from
`apt-packages.txt`; it does not configure external APT repositories or install
Python packages outside apt.

Build the image from the repository root:

```sh
docker build -f ci/ubuntu-24.04/Dockerfile -t tizonia-v1-ubuntu-24.04 .
```

Enter the environment with the current checkout mounted at the expected working
directory:

```sh
docker run --rm -it \
  -v "$PWD:/work/tizonia-openmax-il" \
  -w /work/tizonia-openmax-il \
  tizonia-v1-ubuntu-24.04
```

Run the baseline build inside the container:

```sh
rm -rf build
meson setup build -Dlibspotify=false -Ddocs=false
ninja -C build -j1
```

Documentation tooling is intentionally not installed in the baseline image. If
you are validating a docs-enabled build, install the optional documentation
packages listed at the end of `apt-packages.txt` before configuring with
`-Ddocs=true`.

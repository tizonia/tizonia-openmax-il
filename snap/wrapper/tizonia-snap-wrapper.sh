#!/bin/bash

case "$SNAP_ARCH" in
    "amd64") ARCH='x86_64-linux-gnu'
             ;;
    "i386") ARCH='i386-linux-gnu'
            ;;
    "armhf") ARCH="arm-linux-gnueabihf"
             ;;
    *)
        echo "Unsupported architecture for this app build"
        exit 1
        ;;
esac

# Pulseaudio export
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SNAP/usr/lib/$ARCH/pulseaudio

# Make PulseAudio socket available inside the snap-specific $XDG_RUNTIME_DIR
if [ -n "$XDG_RUNTIME_DIR" ]; then
    pulsenative="pulse/native"
    pulseaudio_sockpath="$XDG_RUNTIME_DIR/../$pulsenative"
    pulseaudio_snappath="$XDG_RUNTIME_DIR/$pulsenative"
    if [ -S "$pulseaudio_sockpath" ]; then
        if [ ! -e "$pulseaudio_snappath" ]; then
            mkdir -p -m 700 $(dirname "$pulseaudio_snappath")
            ln -s "$pulseaudio_sockpath" "$pulseaudio_snappath"
        fi
    fi
fi

# XDG Config
export XDG_CONFIG_DIRS=$SNAP/etc/xdg:$XDG_CONFIG_DIRS

# Tizonia's plugins directory
export TIZONIA_PLUGINS_DIR=$SNAP/lib/tizonia0-plugins12

$SNAP/bin/tizonia "$@"

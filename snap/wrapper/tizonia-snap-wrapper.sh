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

# XDG Config
export XDG_CONFIG_DIRS=$SNAP/etc/xdg:$XDG_CONFIG_DIRS

# Tizonia's plugins directory
export TIZONIA_PLUGINS_DIR=$SNAP/lib/tizonia0-plugins12

$SNAP/bin/tizonia "$@"

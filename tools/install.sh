#!/bin/bash
#
# Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#
# Script that installs Tizonia's debian packages and their dependencies.
#

# Run the install actions in a subshell
(
RELIDS=$(cat /etc/*-release)
if echo "$RELIDS" | grep raspbian; then
  DISTRO="raspbian" ; RELEASE="jessie"
elif echo "$RELIDS" | grep jessie; then
  DISTRO="debian" ; RELEASE="jessie"
elif echo "$RELIDS" | grep stretch; then
  DISTRO="debian" ; RELEASE="stretch"
elif echo "$RELIDS" | grep -E 'trusty|freya|qiana|rebecca|rafaela|rosa|sarah'; then
  # NOTE: Elementary OS 'freya' is based on trusty
  # NOTE: LinuxMint 'qiana' 'rebecca' 'rafaela' 'rosa' 'sarah' are all based on trusty
  DISTRO="ubuntu" ; RELEASE="trusty"
elif echo "$RELIDS" | grep vivid; then
  DISTRO="ubuntu" ; RELEASE="vivid"
elif echo "$RELIDS" | grep -E 'xenial|loki|sarah|serena'; then
  # NOTE: Elementary OS 'loki' is based on xenial
  # Linux Mint 'sarah' and 'serena' are based on xenial
  DISTRO="ubuntu" ; RELEASE="xenial"
else
  echo "Can't find a supported Debian or Ubuntu-based distribution."
  exit 1
fi

# Let's make sure these packages are already installed before trying to install anything else.
sudo apt-get -y --force-yes install python-dev curl apt-transport-https libffi-dev libssl-dev

# Add Mopidy's archive to APT's sources.list (required to install 'libspotify')
grep -q "apt.mopidy.com" /etc/apt/sources.list
if [[ "$?" -eq 1 ]]; then
    curl 'http://apt.mopidy.com/mopidy.gpg' | sudo apt-key add -
    echo "deb http://apt.mopidy.com/ stable main contrib non-free" | sudo tee -a /etc/apt/sources.list
fi

# Add Tizonia's archive to APT's sources.list
grep -q "dl.bintray.com/tizonia" /etc/apt/sources.list
if [[ "$?" -eq 1 ]]; then
    curl -k 'https://bintray.com/user/downloadSubjectPublicKey?username=tizonia' | sudo apt-key add -
    echo "deb https://dl.bintray.com/tizonia/$DISTRO $RELEASE main" | sudo tee -a /etc/apt/sources.list
fi

# Resynchronize APT's package index files
sudo apt-get update

# Python dependencies.
sudo apt-get -y install python-pip \
    && sudo -H pip install --upgrade \
            gmusicapi \
            soundcloud \
            youtube-dl \
            pafy

# Install 'libspotify'
if [[ "$?" -eq 0 ]]; then
    sudo apt-get -y install libspotify12
fi

# Install Tizonia
if [[ "$?" -eq 0 ]]; then
    sudo apt-get -y install tizonia-all
fi

# Copy Tizonia's config file to the user's config directory
if [[ "$?" -eq 0 ]]; then
    TIZ_CONFIG_DIR="$HOME/.config/tizonia"
    TIZ_CONFIG_FILE="$TIZ_CONFIG_DIR/tizonia.conf"
    if [[ ! -e "$TIZ_CONFIG_FILE" ]]; then
        mkdir -p "$TIZ_CONFIG_DIR"
        cp /etc/tizonia/tizonia.conf "$TIZ_CONFIG_FILE"
    fi
fi

# Simple test to verify that everything went well
which tizonia > /dev/null
if [[ "$?" -eq 0 ]]; then
    echo ; tizonia ; echo
    printf "Tizonia is now installed.\n\n"
    printf "Please add Spotify, Google Music, Soundcloud, and Dirble credentials to : $TIZ_CONFIG_FILE\n"
else
    echo "Oops. Something went wrong!"
    exit 1
fi

) # end running in a subshell

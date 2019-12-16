#!/bin/bash
#
# Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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

# Make sure the installation happens safely

echo "============================== NEWS =================================="
echo "Tizonia v0.19.0 has just been recently released (13/12/2019). However,"
echo "Debian packaging is still in progress and the new release is not yet"
echo "in the repos!. Please bear with us while we finalize the packaging."
echo "This message will be removed in the next few days, as soon as the"
echo "new binaries have been uploaded to Bintray. Please check back soon!."
echo "======================================================================"
exit 0

if [[ "$(id -u)" -eq 0 ]]; then
    if [[ ( -z "$1" ) || ( "$1" != "--safe" ) ]]; then
        echo "WARNING: Running as root; installation aborted."
        echo "Please add '--safe' to bypass this check and continue installing as root."
        exit 1
    else
        echo "WARNING: Running as root with --safe option; installation continues..."
    fi
fi

# Run the install actions in a subshell
(
RELIDS=$(cat /etc/*-release)
if echo "$RELIDS" | grep raspbian; then
    DISTRO="raspbian"
    if echo "$RELIDS" | grep buster; then
        RELEASE="buster"
    elif echo "$RELIDS" | grep stretch; then
        RELEASE="stretch"
    elif echo "$RELIDS" | grep jessie; then
        RELEASE="jessie"
    else
        echo "Can't find a supported Raspbian distribution."
        exit 1
    fi
elif echo "$RELIDS" | grep stretch; then
  DISTRO="debian" ; RELEASE="stretch"
elif echo "$RELIDS" | grep -E 'buster'; then
  DISTRO="debian" ; RELEASE="buster"
elif echo "$RELIDS" | grep -E 'bullseye|kali-rolling'; then
  # NOTE: Kali Linux is based on Debian Testing, which is currently codenamed 'Bullseye'
  DISTRO="debian" ; RELEASE="bullseye"
elif echo "$RELIDS" | grep -E 'trusty|freya|qiana|rebecca|rafaela|rosa'; then
  # NOTE: Elementary OS 'freya' is based on trusty
  # NOTE: LinuxMint 'qiana' 'rebecca' 'rafaela' 'rosa' are all based on trusty
  DISTRO="ubuntu" ; RELEASE="trusty"
elif echo "$RELIDS" | grep vivid; then
  DISTRO="ubuntu" ; RELEASE="vivid"
elif echo "$RELIDS" | grep -E 'xenial|loki|sarah|serena|sonya|sylvia'; then
  # NOTE: Elementary OS 'loki' is based on xenial
  # NOTE: Linux Mint 'sarah', 'serena', 'sonya' and 'sylvia' are based on xenial
  DISTRO="ubuntu" ; RELEASE="xenial"
elif echo "$RELIDS" | grep -E 'bionic|juno|hera|tara|tessa|tina'; then
  # NOTE: Elementary OS 'juno', and 'hera' are based on bionic
  # NOTE: Linux Mint 'tara'. 'tessa' and 'tina' are based on bionic
  # NOTE: Most of the time, binaries compiled on 18.04 will work on newer
  # releases, meaning you can try adding newer releases to the bionic conditional
  # (e.g. 'disco|bionic|juno|...'), to support installation on a newer system; 
  # however, do this 'at your own risk', as not all features will be guaranteed to work.
  DISTRO="ubuntu" ; RELEASE="bionic"
else
  echo "Can't find a supported Debian or Ubuntu-based distribution."
  exit 1
fi

# Let's make sure these packages are already installed before trying to install
# anything else.  Some of these packages are needed to make sure that Tizonia's
# Python dependencies are correctly installed from PIP.
sudo apt-get -y --force-yes install build-essential git curl python3-dev apt-transport-https libffi-dev libssl-dev libxml2-dev libxslt1-dev
if [[ "$?" -ne 0 ]]; then
    echo "Oops. Some important dependencies failed to install!."
    echo "Please re-run the install script."
    exit 1
fi

# Add Mopidy's archive to APT's sources.list (required to install 'libspotify')
grep -q "apt.mopidy.com" /etc/apt/sources.list
if [[ "$?" -eq 1 ]]; then
    curl 'http://apt.mopidy.com/mopidy.gpg' | sudo apt-key add -
    echo "deb http://apt.mopidy.com/ stable main contrib non-free" | sudo tee -a /etc/apt/sources.list
fi

# Add Tizonia's archive to APT's sources.list
grep -q "dl.bintray.com/tizonia" /etc/apt/sources.list
if [[ "$?" -eq 1 ]]; then
    echo "Setting up Tizonia's Bintray archive for $DISTRO:$RELEASE"
    curl -k 'https://bintray.com/user/downloadSubjectPublicKey?username=tizonia' | sudo apt-key add -
    echo "deb https://dl.bintray.com/tizonia/$DISTRO $RELEASE main" | sudo tee -a /etc/apt/sources.list
fi

# Resynchronize APT's package index files
sudo apt-get update

# Python dependencies (NOTE: using pip3 here, to make sure the Python 3
# versions of these packages are installed).
sudo apt-get -y install python3-pip \
    && sudo -H pip3 install --upgrade \
            gmusicapi \
            soundcloud \
            youtube-dl \
            pafy \
            pycountry \
            titlecase \
            pychromecast \
            plexapi \
            fuzzywuzzy \
            eventlet \
            python-Levenshtein \
    && sudo -H pip3 install git+https://github.com/plamere/spotipy.git --upgrade

# Install 'libspotify'
if [[ "$?" -eq 0 ]]; then
    sudo apt-get -y install libspotify12
fi

# Install Tizonia
if [[ "$?" -eq 0 ]]; then
    sudo apt-get -y install tizonia-all
fi

# Simple test to verify that everything went well
TIZ_CONFIG_FILE="$HOME/.config/tizonia/tizonia.conf"
which tizonia > /dev/null
if [[ "$?" -eq 0 ]]; then
    echo ; tizonia ; echo
    printf "Tizonia is now installed.\n\n"
    printf "Please add Spotify, Google Music, Soundcloud, Dirble, and Plex credentials to : $TIZ_CONFIG_FILE\n"
else
    echo "Oops. Something went wrong!"
    exit 1
fi

) # end running in a subshell

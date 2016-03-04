#!/bin/bash
#
# Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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

if cat /etc/*-release | grep raspbian; then
  DISTRO="raspbian" ; RELEASE="jessie"
elif cat /etc/*-release | grep jessie; then
  DISTRO="debian" ; RELEASE="jessie"
elif cat /etc/*-release | grep trusty; then
  DISTRO="ubuntu" ; RELEASE="trusty"
elif cat /etc/*-release | grep vivid; then
  DISTRO="ubuntu" ; RELEASE="vivid"
else
  echo "Can't find a supported debian-based distribution."
  exit 1
fi

# Make sure some required packages are already installed
sudo apt-get -y --force-yes install python-dev curl apt-transport-https

# To install libspotify deb packages, add Mopidy's archive to APT's
# sources.list
grep -q "apt.mopidy.com" /etc/apt/sources.list
if [ $? -eq 1 ]; then
    curl 'http://apt.mopidy.com/mopidy.gpg' | sudo apt-key add -
    echo "deb http://apt.mopidy.com/ stable main contrib non-free" | sudo tee -a /etc/apt/sources.list
fi

# Add Tizonia's archive to APT's sources.list
grep -q "dl.bintray.com/tizonia" /etc/apt/sources.list
if [ $? -eq 1 ]; then
    curl 'https://bintray.com/user/downloadSubjectPublicKey?username=tizonia' | sudo apt-key add -
    echo "deb https://dl.bintray.com/tizonia/$DISTRO $RELEASE main" | sudo tee -a /etc/apt/sources.list
fi

# Resynchronize APT's package index files
sudo apt-get update

# Using pip, install Simon Weber's gmusicapi and soundcloud's API wrapper
sudo apt-get -y install python-pip \
    && ( sudo pip install gmusicapi || sudo pip install gmusicapi ) \
    && sudo pip install soundcloud

# Install libspotify
sudo apt-get -y install libspotify12

# Install Tizonia
sudo apt-get -y install tizonia-all

# Copy Tizonia's config file to the user's config directory
if [ $? -eq 0 ]; then
    TIZ_CONFIG_DIR="$HOME/.config/tizonia"
    TIZ_CONFIG_FILE="$TIZ_CONFIG_DIR/tizonia.conf"
    if [ ! -e "$TIZ_CONFIG_FILE" ]; then
        mkdir "$TIZ_CONFIG_DIR"
        cp /etc/tizonia/tizonia.conf/tizonia.conf "$TIZ_CONFIG_FILE"
    fi
fi

# Simple test to verify that everything went well
which tizonia > /dev/null
if [ $? -eq 0 ]; then
    tizonia -v
    printf "\nTizonia is now installed.\n"
    printf "Please add Spotify, Google Music and Soundcloud credentials in : $TIZ_CONFIG_FILE\n"
else
    echo "Oops. Something went wrong!"
    exit 1
fi

exit 0

) # end running in a subshell 

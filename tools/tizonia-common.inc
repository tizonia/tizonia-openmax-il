#!/bin/bash
#
# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
# Constants and functions commonly used by Tizonia's tools.
#

# Constants

readonly E_BADARGS=85
readonly E_NOFILE=86
readonly E_BAD_CLONE=87
readonly CWD=$(pwd)

readonly RED=1
readonly GRN=2
readonly YEL=3
readonly BLU=4
readonly MAG=5
readonly CYA=6
readonly WHI=7

declare -Ar TIZ_PROJECT_DIRS=( \
    [tizdbus-c++]="3rdparty/dbus-cplusplus" \
    [tizilheaders]="include" \
    [tizgmusicproxy]="clients/gmusic/gmusicproxy" \
    [tizgmusic]="clients/gmusic/libtizgmusic" \
    [tizsoundcloudproxy]="clients/soundcloud/soundcloudproxy" \
    [tizsoundcloud]="clients/soundcloud/libtizsoundcloud" \
    [tizyoutubeproxy]="clients/youtube/youtubeproxy" \
    [tizyoutube]="clients/youtube/libtizyoutube" \
    [tizplexproxy]="clients/plex/plexproxy" \
    [tizplex]="clients/plex/libtizplex" \
    [tizchromecastproxy]="clients/chromecast/chromecastproxy" \
    [tizchromecast]="clients/chromecast/libtizchromecast" \
    [tizspotifyproxy]="clients/spotify/spotifyproxy" \
    [tizspotify]="clients/spotify/libtizspotify" \
    [tiztuneinproxy]="clients/tunein/tuneinproxy" \
    [tiztunein]="clients/tunein/libtiztunein" \
    [tizplatform]="libtizplatform" \
    [tizcastd]="cast/tizcastd" \
    [tizcastclient]="cast/libtizcastclient" \
    [tizrmd]="rm/tizrmd" \
    [tizrmproxy]="rm/libtizrmproxy" \
    [tizcore]="libtizcore" \
    [tizonia]="libtizonia" \
    [tizaacdec]="plugins/aac_decoder" \
    [tizchromecastrnd]="plugins/chromecast_renderer" \
    [tizfr]="plugins/file_reader" \
    [tizfw]="plugins/file_writer" \
    [tizflacdec]="plugins/flac_decoder" \
    [tizhttprnd]="plugins/http_renderer" \
    [tizhttpsrc]="plugins/http_source" \
    [tizmp3dec]="plugins/mp3_decoder" \
    [tizmp3enc]="plugins/mp3_encoder" \
    [tizmp3meta]="plugins/mp3_metadata" \
    [tizmpgdec]="plugins/mpeg_audio_decoder" \
    [tizoggdmux]="plugins/ogg_demuxer" \
    [tizopusdec]="plugins/opus_decoder" \
    [tizopusfiledec]="plugins/opusfile_decoder" \
    [tizpcmdec]="plugins/pcm_decoder" \
    [tizalsapcmrnd]="plugins/pcm_renderer_alsa" \
    [tizpulsepcmrnd]="plugins/pcm_renderer_pa" \
    [tizspotifysrc]="plugins/spotify_source" \
    [tizvorbisdec]="plugins/vorbis_decoder" \
    [tizvp8dec]="plugins/vp8_decoder" \
    [tizsdlivrnd]="plugins/yuv_renderer" \
    [tizwebmdmux]="plugins/webm_demuxer" \
    [tizonia-player]="player" \
    [tizonia-config]="config" \
)

readonly TIZ_PROJECTS=(${!TIZ_PROJECT_DIRS[@]})
readonly TIZ_PROJECTS_ORDERED=( \
    tizdbus-c++ \
    tizilheaders \
    tizgmusicproxy \
    tizgmusic \
    tizsoundcloudproxy \
    tizsoundcloud \
    tizyoutubeproxy \
    tizyoutube \
    tizplexproxy \
    tizplex \
    tizchromecastproxy \
    tizchromecast \
    tizspotifyproxy \
    tizspotify \
    tiztuneinproxy \
    tiztunein \
    tizplatform \
    tizcastd \
    tizcastclient \
    tizrmd \
    tizrmproxy \
    tizcore \
    tizonia \
    tizaacdec \
    tizchromecastrnd \
    tizfr \
    tizfw \
    tizflacdec \
    tizhttprnd \
    tizhttpsrc \
    tizmp3dec \
    tizmp3enc \
    tizmp3meta \
    tizmpgdec \
    tizoggdmux \
    tizopusdec \
    tizopusfiledec \
    tizpcmdec \
    tizalsapcmrnd \
    tizpulsepcmrnd \
    tizspotifysrc \
    tizvorbisdec \
    tizvp8dec \
    tizsdlivrnd \
    tizwebmdmux \
    tizonia-player \
    tizonia-config \
)

##################################################################
# Simple function to print with color
#
# Globals
#
# Arguments:
#
# Returns:
#   None
##################################################################
function pretty_print {
    echo "$(tput setaf $1)$2$(tput sgr 0)" 1>&2
}

##################################################################
# Simple function to print a banner
#
# Globals
#
# Arguments:
#
# Returns:
#   None
##################################################################
function print_banner {
    local msg="$1"
    local color="$2"
    local msg_sz=${#msg}
    local dashes=$(print_dashes "$msg_sz")
    if [[ "$color" == "" ]]; then
        color="$RED"
    fi
    echo
    pretty_print "$color" "$dashes"
    pretty_print "$color" "$msg"
    pretty_print "$color" "$dashes"
}

##################################################################
# Simple function to produce a string of "dashes" of a
# pre-determined size
#
# Globals
#   TIZONIA_DIR
# Arguments:
#   The length of the message
# Returns:
#   None
##################################################################
function print_dashes {
    local LEN=$1
    local CHAR='-'
    while (( $# > 0 ))
    do
        case $1 in
            [0-9]*) LEN=$1;;
            -c) shift
                CHAR=$1;;
            *) usagexit;;
        esac
        shift
    done

    if (( LEN > 4096 ))
    then
        echo "too large" >&2
        exit 3
    fi

    # build the string to the exact length
    DASHES=""
    for ((i=0; i<LEN; i++))
    do
        DASHES="${DASHES}${CHAR}"
    done
    echo "$DASHES"
}

##################################################################
# Simple function to print a banner with copyright and license info
#
# Arguments:
#   The tool's basename (i.e. $(basename "$0"))
# Returns:
#   None
##################################################################
function print_banner_and_license {
    pretty_print "$BLU" "$1. Copyright (C) 2020 Juan A. Rubio"
    pretty_print "$BLU" "This software is part of the Tizonia project <http://tizonia.org>."

    echo
    pretty_print "$BLU" "GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl.html>"
    pretty_print "$BLU" "This is free software: you are free to change and redistribute it."
    pretty_print "$BLU" "There is NO WARRANTY, to the extent permitted by law."
}

##################################################################
# Simple function to print a debug/error message with some extra info
#
# Globals
#   None
# Arguments:
#   None
# Returns:
#   None
##################################################################
function log_on_error {
    local exit_status="$1"
    local msg="$2"
    if [[ "$exit_status" -ne 0 ]]; then
        pretty_print "$RED" "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $2"
    fi
}

##################################################################
# Simple function to check if a string exists in an array
#
# Globals
#   none
# Arguments:
#   The search string is the first argument
#   The array is the second argument
# Returns:
#   0 if success, 1 on error
##################################################################
function exists_in_array {
    local e
    for e in "${@:2}"; do [[ "$e" == "$1" ]] && return 0; done
    return 1
}

##################################################################
# Simple function to print an error message an exit with error code 1
#
# Globals
#   None
# Arguments:
#   None
# Returns:
#   Always 1
##################################################################
function raise_error {
    pretty_print "$RED" "$@" 1>&2
    exit 1
}

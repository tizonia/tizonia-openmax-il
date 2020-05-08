# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
#
# This file is part of Tizonia
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""@package tizplexproxy
Simple Plex API proxy/wrapper.

Access Plex servers to retrieve audio track URLs and create a playback queue.

"""

import sys
import os
import logging
import random
import unicodedata
import re
import configparser
from plexapi.exceptions import NotFound
from plexapi.myplex import MyPlexAccount
from plexapi.server import PlexServer
from fuzzywuzzy import process
from fuzzywuzzy import fuzz
import imp

if sys.version[0] == "2":
    imp.reload(sys)
    sys.setdefaultencoding("utf-8")

# For use during debugging
# import pprint

FORMAT = (
    "[%(asctime)s] [%(levelname)5s] [%(thread)d] "
    "[%(module)s:%(funcName)s:%(lineno)d] - %(message)s"
)

logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get("TIZONIA_PLEXPROXY_DEBUG"):
    logging.basicConfig(format=FORMAT)
    from traceback import print_exception
else:
    logging.getLogger().addHandler(logging.NullHandler())


class ConfigColors:
    def __init__(self):
        self.config = configparser.ConfigParser()
        self.config.read(
            os.path.join(os.getenv("HOME"), ".config/tizonia/tizonia.conf")
        )
        active_theme = self.config.get(
            "color-themes", "active-theme", fallback="tizonia"
        )
        active_theme = active_theme + "."
        self.FAIL = (
            "\033["
            + self.config.get("color-themes", active_theme + "C08", fallback="91")
            .replace(",", ";")
            .split("#", 1)[0]
            .strip()
            + "m"
        )
        self.OKGREEN = (
            "\033["
            + self.config.get("color-themes", active_theme + "C09", fallback="92")
            .replace(",", ";")
            .split("#", 1)[0]
            .strip()
            + "m"
        )
        self.WARNING = (
            "\033["
            + self.config.get("color-themes", active_theme + "C10", fallback="93")
            .replace(",", ";")
            .split("#", 1)[0]
            .strip()
            + "m"
        )
        self.OKBLUE = (
            "\033["
            + self.config.get("color-themes", active_theme + "C11", fallback="94")
            .replace(",", ";")
            .split("#", 1)[0]
            .strip()
            + "m"
        )
        self.OKMAGENTA = (
            "\033["
            + self.config.get("color-themes", active_theme + "C12", fallback="95")
            .replace(",", ";")
            .split("#", 1)[0]
            .strip()
            + "m"
        )
        self.ENDC = "\033[0m"


_Colors = ConfigColors()


def pretty_print(color, msg=""):
    """Print message with color.

    """
    print(color + msg + _Colors.ENDC)


def print_msg(msg=""):
    """Print a normal message.

    """
    pretty_print(_Colors.OKGREEN + msg + _Colors.ENDC)


def print_nfo(msg=""):
    """Print an info message.

    """
    pretty_print(_Colors.OKBLUE + msg + _Colors.ENDC)


def print_adv(msg=""):
    """Print an advisory message.

    """
    pretty_print(_Colors.OKMAGENTA + msg + _Colors.ENDC)


def print_wrn(msg=""):
    """Print a warning message.

    """
    pretty_print(_Colors.WARNING + msg + _Colors.ENDC)


def print_err(msg=""):
    """Print an error message.

    """
    pretty_print(_Colors.FAIL + msg + _Colors.ENDC)


def exception_handler(exception_type, exception, traceback):
    """A simple handler that prints the exception message.

    """

    print_err("[Plex] (%s) : %s" % (exception_type.__name__, exception))

    if os.environ.get("TIZONIA_PLEXPROXY_DEBUG"):
        print_exception(exception_type, exception, traceback)


sys.excepthook = exception_handler


class TizEnumeration(set):
    """A simple enumeration class.

    """

    def __getattr__(self, name):
        if name in self:
            return name
        raise AttributeError


def to_ascii(msg):
    """Unicode to ascii helper.

    """

    if sys.version[0] == "2":
        return unicodedata.normalize("NFKD", str(msg)).encode("ASCII", "ignore")
    return msg


class TrackInfo(object):
    """ Class that represents a Plex track in the queue.

    """

    def __init__(self, track, artist, album):
        """ class members. """
        self.title = track.title
        self.artist = artist.title
        self.album = album.title
        self.year = album.year if album.year else 0
        if not track.duration:
            track.duration = 1
        self.duration = track.duration / 1000 if track.duration else 1
        m, s = divmod(self.duration, 60)
        h, m = divmod(m, 60)
        self.duration_str = ''
        if h > 0:
            self.duration_str = str(
                "{:d}h:{:d}m:{:02d}s".format(int(h), int(m), round(s))
            )
        elif m > 0:
            self.duration_str = str("{:d}m:{:02d}s".format(int(m), round(s)))
        else:
            self.duration_str = str("{:02d}s".format(round(s)))
        self.url = track.getStreamURL()
        self.thumb_url = track.thumbUrl
        self.art_url = track.artUrl
        media = track.media[0]
        if media:
            self.bitrate = media.bitrate if media.bitrate else 0
            self.codec = media.audioCodec
            self.channels = media.audioChannels
            part = media.parts[0]
            self.size = part.size if part else 0


class tizplexproxy(object):
    """A class that accesses Plex servers, retrieves track URLs and creates and
    manages a playback queue.

    """

    def __init__(self, base_url, token, section):
        self.base_url = base_url
        self.queue = list()
        self.queue_index = -1
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_track = None
        self._plex = PlexServer(base_url, token)
        self._music = self._plex.library.section(section)

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: current valid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)
        self._update_play_queue_order()

    def enqueue_audio_tracks(self, arg):
        """Search the Plex server for audio tracks and add them to the playback queue.

        :param arg: a search string

        """
        logging.info("arg : %s", arg)
        print_msg("[Plex] [Track search in server] : '{0}'. ".format(self.base_url))
        try:
            count = len(self.queue)

            try:
                tracks = self._music.searchTracks(title=arg)
                for track in tracks:
                    track_info = TrackInfo(track, track.artist(), track.album())
                    self._add_to_playback_queue(track_info)

            except (NotFound):
                pass

            if count == len(self.queue):
                tracks = self._music.search(libtype="track")
                for track in tracks:
                    track_name = track.title
                    if fuzz.partial_ratio(arg, track_name) > 60:
                        track_info = TrackInfo(track, track.artist(), track.album())
                        self._add_to_playback_queue(track_info)

            self._finalise_play_queue(count, arg)

        except ValueError:
            raise ValueError(str("Track not found : %s" % arg))

    def enqueue_audio_artist(self, arg):
        """Obtain an artist from the Plex server and add all the artist's audio tracks
        to the playback queue.

        :param arg: an artist search term

        """
        logging.info("arg : %s", arg)
        print_msg("[Plex] [Artist search in server] : '{0}'. ".format(self.base_url))
        try:
            count = len(self.queue)
            artist = None
            artist_name = ""

            try:
                artists = self._music.searchArtists(title=arg)
                for artist in artists:
                    artist_name = artist.title
                    print_wrn("[Plex] Playing '{0}'.".format(artist_name))
                    for album in artist.albums():
                        for track in album.tracks():
                            track_info = TrackInfo(track, artist, album)
                            self._add_to_playback_queue(track_info)

            except (NotFound):
                pass

            if count == len(self.queue):
                artist_dict = dict()
                artist_names = list()
                artists = self._music.search(libtype="artist")
                for art in artists:
                    artist_names.append(art.title)
                    artist_dict[art.title] = art

                if len(artist_names) > 1:
                    artist_name = process.extractOne(arg, artist_names)[0]
                    artist = artist_dict[artist_name]
                elif len(artist_names) == 1:
                    artist_name = artist_names[0]
                    artist = artist_dict[artist_name]

                if artist:
                    print_adv(
                        "[Plex] '{0}' not found. "
                        "Playing '{1}' instead.".format(arg, artist_name)
                    )
                    for album in artist.albums():
                        for track in album.tracks():
                            track_info = TrackInfo(track, artist, album)
                            self._add_to_playback_queue(track_info)

            self._finalise_play_queue(count, arg)

        except ValueError:
            raise ValueError(str("Artist not found : %s" % arg))

    def enqueue_audio_album(self, arg):
        """Obtain an album from the Plex server and add all its tracks to the playback
        queue.

        :param arg: an album search term

        """
        logging.info("arg : %s", arg)
        print_msg("[Plex] [Album search in server] : '{0}'. ".format(self.base_url))
        try:
            count = len(self.queue)
            album = None
            album_name = ""

            try:
                albums = self._music.searchAlbums(title=arg)
                for album in albums:
                    album_name = album.title
                    print_wrn("[Plex] Playing '{0}'.".format(album_name))
                    for track in album.tracks():
                        track_info = TrackInfo(track, track.artist(), album)
                        self._add_to_playback_queue(track_info)

            except (NotFound):
                pass

            if count == len(self.queue):
                album_dict = dict()
                album_names = list()
                albums = self._music.search(libtype="album")
                for alb in albums:
                    album_names.append(alb.title)
                    album_dict[alb.title] = alb

                if len(album_names) > 1:
                    album_name = process.extractOne(arg, album_names)[0]
                    album = album_dict[album_name]
                elif len(album_names) == 1:
                    album_name = album_names[0]
                    album = album_dict[album_name]

                if album:
                    print_adv(
                        "[Plex] '{0}' not found. "
                        "Playing '{1}' instead.".format(arg, album_name)
                    )
                    for track in album.tracks():
                        track_info = TrackInfo(track, album, album)
                        self._add_to_playback_queue(track_info)

            self._finalise_play_queue(count, arg)

        except ValueError:
            raise ValueError(str("Album not found : %s" % arg))

    def enqueue_audio_playlist(self, arg):
        """Add all audio tracks in a Plex playlist to the playback queue.

        :param arg: a playlist search term

        """
        logging.info("arg : %s", arg)
        print_msg("[Plex] [Playlist search in server] : '{0}'. ".format(self.base_url))
        try:
            count = len(self.queue)
            playlist_title = ""
            playlist = None

            try:
                playlist = self._plex.playlist(title=arg)
                if playlist:
                    playlist_title = playlist.title
                    print_wrn("[Plex] Playing '{0}'.".format(playlist_title))
                    for item in list(playlist.items()):
                        if item.TYPE == "track":
                            track = item
                            track_info = TrackInfo(track, track.artist(), track.album())
                            self._add_to_playback_queue(track_info)
                        if count == len(self.queue):
                            print_wrn(
                                "[Plex] '{0}' No audio tracks found.".format(
                                    playlist_title
                                )
                            )
                            raise ValueError

            except (NotFound):
                pass

            if count == len(self.queue):
                playlist_dict = dict()
                playlist_titles = list()
                playlists = self._plex.playlists()
                for pl in playlists:
                    playlist_titles.append(pl.title)
                    playlist_dict[pl.title] = pl

                if len(playlist_titles) > 1:
                    playlist_title = process.extractOne(arg, playlist_titles)[0]
                    playlist = playlist_dict[playlist_title]
                elif len(playlist_titles) == 1:
                    playlist_title = playlist_titles[0]
                    playlist = playlist_dict[playlist_title]

                if playlist:
                    print_adv(
                        "[Plex] '{0}' not found. "
                        "Playing '{1}' instead.".format(arg, playlist_title)
                    )
                    for item in list(playlist.items()):
                        if item.TYPE == "track":
                            track = item
                            track_info = TrackInfo(track, track.artist(), track.album())
                            self._add_to_playback_queue(track_info)
                        if count == len(self.queue):
                            print_wrn(
                                "[Plex] '{0}' No audio tracks found.".format(
                                    playlist_title
                                )
                            )

            self._finalise_play_queue(count, arg)

        except (ValueError, NotFound):
            raise ValueError(
                str("Playlist not found or no audio tracks in playlist : %s" % arg)
            )

    def current_audio_track_title(self):
        """ Retrieve the current track's title.

        """
        logging.info("current_audio_track_title")
        track = self.now_playing_track
        title = ""
        if track:
            title = to_ascii(track.title)
        return title

    def current_audio_track_artist(self):
        """ Retrieve the current track's artist.

        """
        logging.info("current_audio_track_artist")
        track = self.now_playing_track
        artist = ""
        if track:
            artist = to_ascii(track.artist)
        return artist

    def current_audio_track_album(self):
        """ Retrieve the current track's album.

        """
        logging.info("current_audio_track_album")
        track = self.now_playing_track
        album = ""
        if track and track.album:
            album = to_ascii(track.album)
        return album

    def current_audio_track_year(self):
        """ Retrieve the current track's publication year.

        """
        logging.info("current_audio_track_year")
        track = self.now_playing_track
        year = 0
        if track:
            year = track.year
        return year

    def current_audio_track_file_size(self):
        """ Retrieve the current track's file size.

        """
        logging.info("current_audio_track_file_size")
        track = self.now_playing_track
        size = 0
        if track:
            size = track.size
        return size

    def current_audio_track_duration(self):
        """ Retrieve the current track's duration.

        """
        logging.info("current_audio_track_duration")
        track = self.now_playing_track
        duration = 0
        if track:
            duration = track.duration
        return duration

    def current_audio_track_bitrate(self):
        """ Retrieve the current track's bitrate.

        """
        logging.info("current_audio_track_bitrate")
        track = self.now_playing_track
        bitrate = 0
        if track:
            bitrate = track.bitrate
        return bitrate

    def current_audio_track_codec(self):
        """ Retrieve the current track's codec.

        """
        logging.info("current_audio_track_codec")
        track = self.now_playing_track
        codec = ""
        if track:
            codec = to_ascii(track.codec)
        return codec

    def current_audio_track_album_art(self):
        """ Retrieve the current track's album_art.

        """
        logging.info("current_audio_track_album_art")
        track = self.now_playing_track
        album_art = ""
        if track and track.thumb_url:
            album_art = to_ascii(track.thumb_url)
        return album_art

    def current_audio_track_queue_index_and_queue_length(self):
        """ Retrieve index in the queue (starting from 1) of the current track and the
        length of the playback queue.

        """
        logging.info("current_audio_track_queue_index_and_queue_length")
        return self.play_queue_order[self.queue_index] + 1, len(self.queue)

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1

    def print_queue(self):
        """ Print the contents of the playback queue.

        """

        for i in range(0, len(self.queue)):
            track = self.queue[self.play_queue_order[i]]
            order_num = str("#{:0{}d}".format(i + 1, len(str(len(self.queue)))))
            info_str = str(
                "[Plex] [Track] [{0}] '{1}' [{2}] ({3})".format(
                    order_num,
                    to_ascii(track.title),
                    to_ascii(track.artist),
                    to_ascii(track.duration_str),
                )
            )
            print_nfo(info_str + ".")

        print_nfo("[Plex] [Tracks in queue] '{0}'.".format(len(self.queue)))

    def remove_current_url(self):
        """Remove the currently active url from the playback queue.

        """
        logging.info("")
        if len(self.queue) and self.queue_index:
            track = self.queue[self.queue_index]
            print_nfo(
                "[Plex] [Track] '{0}' removed.".format(to_ascii(track["i"].title))
            )
            del self.queue[self.queue_index]
            self.queue_index -= 1
            if self.queue_index < 0:
                self.queue_index = 0
            self._update_play_queue_order()

    def next_url(self):
        """ Retrieve the url of the next track in the playback queue.

        """
        logging.info("")
        try:
            if len(self.queue):
                self.queue_index += 1
                if (self.queue_index < len(self.queue)) and (self.queue_index >= 0):
                    next_track = self.queue[self.play_queue_order[self.queue_index]]
                    return self._retrieve_track_url(next_track)
                else:
                    self.queue_index = -1
                    return self.next_url()
            else:
                return ""
        except (KeyError, AttributeError):
            # TODO: We don't remove this for now
            # del self.queue[self.queue_index]
            logging.info("exception")
            return self.next_url()

    def prev_url(self):
        """ Retrieve the url of the previous track in the playback queue.

        """
        logging.info("")
        try:
            if len(self.queue):
                self.queue_index -= 1
                if (self.queue_index < len(self.queue)) and (self.queue_index >= 0):
                    prev_track = self.queue[self.play_queue_order[self.queue_index]]
                    return self._retrieve_track_url(prev_track)
                else:
                    self.queue_index = len(self.queue)
                    return self.prev_url()
            else:
                return ""
        except (KeyError, AttributeError):
            # TODO: We don't remove this for now
            # del self.queue[self.queue_index]
            logging.info("exception")
            return self.prev_url()

    def get_url(self, position=None):
        """Retrieve the url on a particular position in the playback queue. If no
        position is given, the url at the current position of the playback is returned.

        """
        logging.info("get_url {}".format(position if position else "-1"))
        try:
            if len(self.queue):
                queue_pos = self.play_queue_order[self.queue_index]
                if position and position > 0 and position <= len(self.queue):
                    self.queue_index = position - 1
                    queue_pos = self.play_queue_order[self.queue_index]
                    logging.info("get_url : self.queue_index {}".format(self.queue_index))
                logging.info(
                    "get_url : play_queue_order {}".format(
                        self.play_queue_order[self.queue_index]
                    )
                )
                track = self.queue[queue_pos]
                return self._retrieve_track_url(track)
            else:
                return ""
        except (KeyError, AttributeError):
            # TODO: We don't remove this for now
            # del self.queue[self.queue_index]
            logging.info("exception")
            return ""

    def _update_play_queue_order(self):
        """ Update the queue playback order.

        A sequential order is applied if the current play mode is "NORMAL" or a
        random order if current play mode is "SHUFFLE"

        """
        total_tracks = len(self.queue)
        if total_tracks:
            if not len(self.play_queue_order):
                # Create a sequential play order, if empty
                self.play_queue_order = list(range(total_tracks))
            if self.current_play_mode == self.play_modes.SHUFFLE:
                random.shuffle(self.play_queue_order)

    def _retrieve_track_url(self, track):
        """ Retrieve a track url

        """
        try:
            self.now_playing_track = track
            return track.url

        except AttributeError:
            logging.info("Could not retrieve the track url!")
            raise

    def _add_to_playback_queue(self, track):
        """ Add to the playback queue. """

        if not track:
            return

        self.queue.append(track)

    def _finalise_play_queue(self, count, arg):
        """ Helper function to grou the various actions needed to ready play
        queue.

        """

        if count == len(self.queue):
            logging.info("no tracks found arg : %s", arg)
            raise ValueError
        self._update_play_queue_order()
        self.print_queue()


if __name__ == "__main__":
    tizplexproxy()

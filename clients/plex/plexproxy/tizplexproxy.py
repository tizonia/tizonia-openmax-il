# Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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

from __future__ import print_function, unicode_literals

import sys
import os
import logging
import random
import unicodedata
import re
from plexapi.exceptions import NotFound
from plexapi.myplex import MyPlexAccount
from plexapi.server import PlexServer

# For use during debugging
# import pprint
# from traceback import print_exception

FORMAT = '[%(asctime)s] [%(levelname)5s] [%(thread)d] ' \
         '[%(module)s:%(funcName)s:%(lineno)d] - %(message)s'

logging.captureWarnings(True)
logging.getLogger().addHandler(logging.NullHandler())
logging.basicConfig(format=FORMAT)
logging.getLogger().setLevel(logging.DEBUG)

class _Colors:
    """A trivial class that defines various ANSI color codes.

    """
    BOLD = '\033[1m'
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

# This code is here for debugging purposes
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
    del traceback # unused
    # print_exception(exception_type, exception, traceback)

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

    return unicodedata.normalize('NFKD', unicode(msg)).encode('ASCII', 'ignore')

class TrackInfo(object):
    """ Class that represents a Plex track in the queue.

    """

    def __init__(self, track, artist, album):
        """ class members. """
        self.title = track.title
        self.artist = artist.title
        self.album = album.title
        self.year = album.year if album.year else 0;
        self.duration = track.duration / 1000 if track.duration else 0;
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

    def __init__(self, base_url, token):
        self.queue = list()
        self.queue_index = -1
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_track = None
        self._plex = PlexServer(base_url, token)
        self._music = self._plex.library.section('Music')

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: current valid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)
        self.__update_play_queue_order()

    def enqueue_audio_tracks(self, arg):
        """Search the Plex server for audio tracks and add them to the playback queue.

        :param arg: a search string

        """
        logging.info('arg : %s', arg)
        try:
            count = len(self.queue)

            tracks = self._music.searchTracks(title=arg)
            for track in tracks:
                track_info = TrackInfo(track, track.artist(), track.album())
                self.add_to_playback_queue(track_info)

            if count == len(self.queue):
                raise ValueError
            self.__update_play_queue_order()

        except ValueError:
            raise ValueError(str("Track not found : %s" % arg))

    def enqueue_audio_artist(self, arg, feelinglucky=True):
        """Obtain an artist from the Plex server and add all the artist's audio tracks
        to the playback queue.

        :param arg: an artist search term

        :param feelinglucky: If True, it will perform another Plex search to find
        alternatives if the original artist cannot be found.

        """
        logging.info('arg : %s', arg)
        try:
            count = len(self.queue)

            artists = self._music.searchArtists(title=arg)
            for artist in artists:
                for album in artist.albums():
                    for track in album.tracks():
                        track_info = TrackInfo(track, artist, album)
                        self.add_to_playback_queue(track_info)

            if count == len(self.queue):
                raise ValueError
            self.__update_play_queue_order()

        except ValueError:
            raise ValueError(str("Artist not found : %s" % arg))

    def enqueue_audio_album(self, arg, feelinglucky=True):
        """Obtain an album from the Plex server and add all its tracks to the playback
        queue.

        :param arg: a Plex video id

        :param feelinglucky: If True, it will perform another Plex search to
        find alternatives if the original album cannot be found.

        """
        logging.info('arg : %s', arg)
        try:
            count = len(self.queue)

            albums = self._music.searchAlbums(title=arg)
            for album in albums:
                for track in album.tracks():
                    track_info = TrackInfo(track, track.artist(), album)
                    self.add_to_playback_queue(track_info)

            if count == len(self.queue):
                raise ValueError
            self.__update_play_queue_order()

        except ValueError:
            raise ValueError(str("Album not found : %s" % arg))


    def enqueue_audio_playlist(self, arg):
        """Add all audio tracks in a Plex playlist to the playback queue.

        :param arg: a Plex playlist id

        """
        logging.info('arg : %s', arg)
        try:
            count = len(self.queue)

            playlists = self._plex.playlist(title=arg)
            for playlist in playlists:
                for item in playlist.items():
                    if item.TYPE == 'track':
                        track = item
                        track_info = TrackInfo(track, track.artist(), \
                                               track.album())
                        self.add_to_playback_queue(track_info)

            if count == len(self.queue):
                raise ValueError
            self.__update_play_queue_order()

        except (ValueError, NotFound):
            raise ValueError(str("Playlist not found : %s" % arg))

    def current_audio_track_title(self):
        """ Retrieve the current track's title.

        """
        track = self.now_playing_track
        title = ''
        if track:
            title = to_ascii(track.title).encode("utf-8")
        return title

    def current_audio_track_artist(self):
        """ Retrieve the current track's artist.

        """
        track = self.now_playing_track
        artist = ''
        if track:
            artist = to_ascii(track.artist).encode("utf-8")
        return artist

    def current_audio_track_album(self):
        """ Retrieve the current track's album.

        """
        track = self.now_playing_track
        album = ''
        if track:
            album = to_ascii(track.album).encode("utf-8")
        return album

    def current_audio_track_year(self):
        """ Retrieve the current track's publication year.

        """
        track = self.now_playing_track
        year = 0
        if track:
            year = track.year
        return year

    def current_audio_track_file_size(self):
        """ Retrieve the current track's file size.

        """
        track = self.now_playing_track
        size = 0
        if track:
            size = track.size
        return size

    def current_audio_track_duration(self):
        """ Retrieve the current track's duration.

        """
        track = self.now_playing_track
        duration = 0
        if track:
            duration = track.duration
        return duration

    def current_audio_track_bitrate(self):
        """ Retrieve the current track's bitrate.

        """
        track = self.now_playing_track
        bitrate = 0
        if track:
            bitrate = track.bitrate
        return bitrate

    def current_audio_track_codec(self):
        """ Retrieve the current track's codec.

        """
        track = self.now_playing_track
        codec = ''
        if track:
            codec = to_ascii(track.codec).encode("utf-8")
        return codec

    def current_audio_track_album_art(self):
        """ Retrieve the current track's album_art.

        """
        track = self.now_playing_track
        album_art = ''
        if track:
            album_art = to_ascii(track.thumb_url).encode("utf-8")
        return album_art

    def current_audio_track_queue_index_and_queue_length(self):
        """ Retrieve index in the queue (starting from 1) of the current track and the
        length of the playback queue.

        """
        return self.play_queue_order[self.queue_index] + 1, len(self.queue)

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1

    def remove_current_url(self):
        """Remove the currently active url from the playback queue.

        """
        logging.info("")
        if len(self.queue) and self.queue_index:
            track = self.queue[self.queue_index]
            print_nfo("[Plex] [Track] '{0}' removed." \
                      .format(to_ascii(track['i'].title).encode("utf-8")))
            del self.queue[self.queue_index]
            self.queue_index -= 1
            if self.queue_index < 0:
                self.queue_index = 0
            self.__update_play_queue_order()

    def next_url(self):
        """ Retrieve the url of the next track in the playback queue.

        """
        logging.info("")
        try:
            if len(self.queue):
                self.queue_index += 1
                if (self.queue_index < len(self.queue)) \
                   and (self.queue_index >= 0):
                    next_track = self.queue[self.play_queue_order \
                                            [self.queue_index]]
                    return self.__retrieve_track_url(next_track)
                else:
                    self.queue_index = -1
                    return self.next_url()
            else:
                return ''
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
                if (self.queue_index < len(self.queue)) \
                   and (self.queue_index >= 0):
                    prev_track = self.queue[self.play_queue_order \
                                            [self.queue_index]]
                    return self.__retrieve_track_url(prev_track)
                else:
                    self.queue_index = len(self.queue)
                    return self.prev_url()
            else:
                return ''
        except (KeyError, AttributeError):
            # TODO: We don't remove this for now
            # del self.queue[self.queue_index]
            logging.info("exception")
            return self.prev_url()

    def __update_play_queue_order(self):
        """ Update the queue playback order.

        A sequential order is applied if the current play mode is "NORMAL" or a
        random order if current play mode is "SHUFFLE"

        """
        total_tracks = len(self.queue)
        if total_tracks:
            if not len(self.play_queue_order):
                # Create a sequential play order, if empty
                self.play_queue_order = range(total_tracks)
            if self.current_play_mode == self.play_modes.SHUFFLE:
                random.shuffle(self.play_queue_order)
            print_nfo("[Plex] [Tracks in queue] '{0}'." \
                      .format(total_tracks))

    def __retrieve_track_url(self, track):
        """ Retrieve a track url

        """
        try:
            self.now_playing_track = track
            return track.url.encode("utf-8")

        except AttributeError:
            logging.info("Could not retrieve the track url!")
            raise

    def add_to_playback_queue(self, track):
        """ Add to the playback queue. """

        print_nfo("[Plex] [Track] '{0}' [{1}]." \
                  .format(to_ascii(track.title).encode("utf-8"), \
                          to_ascii(track.codec)))
        queue_index = len(self.queue)
        self.queue.append(track)

if __name__ == "__main__":
    tizplexproxy()

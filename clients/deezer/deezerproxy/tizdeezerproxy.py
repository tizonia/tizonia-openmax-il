# Copyright (C) 2017 Aratelia Limited - Juan A. Rubio
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

"""@package tizdeezerproxy
Simple Deezer proxy/wrapper.

Access Deezer using a user account to retrieve track URLs and create a play
queue for streaming.

"""

from __future__ import unicode_literals

import os
import sys
import logging
import random
import collections
import unicodedata
from requests.exceptions import HTTPError
from operator import itemgetter

from tizmopidydeezer import DeezerClient

# For use during debugging
import pprint

logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get('TIZONIA_DEEZERPROXY_DEBUG'):
    from traceback import print_exception
else:
    logging.getLogger().addHandler(logging.NullHandler())

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

def pretty_print(color, msg=""):
    """Print message with color.

    """
    print color + msg + _Colors.ENDC

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
    """A simple exception handler that prints the excetion message.

    """

    print_err("[Deezer] (%s) : %s" % (exception_type.__name__, exception))
    if os.environ.get('TIZONIA_DEEZERPROXY_DEBUG'):
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

    return unicodedata.normalize('NFKD', unicode(msg)).encode('ASCII', 'ignore')

class tizdeezerproxy(object):
    """A class that logs into a Deezer account, retrieves track URLs
    on behalf of the user and creates and manages a playback queue.

    """
    def __init__(self, user_id):
        self.__api = DeezerClient(user_id)
        self.queue = list()
        self.queue_index = -1
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_track = None
        self.now_playing_stream = None

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: curren tvalid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)
        self.__update_play_queue_order()

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1

    def current_track_title_and_artist(self):
        """ Retrieve the current track's title and artist name.

        """
        logging.info("current_track_title_and_artist")
        title = ''
        artist = ''
        track = self.now_playing_track
        if track:
            title = to_ascii(track.name)
            artist = ''
            if track.artists and len(track.artists):
                artist = to_ascii(track.artists[0].name)
            logging.info("Now playing %s by %s", title, artist)
        return title, artist

    def enqueue_album(self, arg):
        """ Search the user's library for albums with a given name and adds
        them to the playback queue.

        """
        try:
            logging.info("Album search %s", arg)
            [artists, albums, tracks] = self.__api.search(arg)

            album = None
            tentative_album = None

            for alb in albums:
                iterartists = iter(alb.artists)
                album_name = alb.name
                artist_name = next(iterartists).name
                for art in iterartists:
                    artist_name += ', ' + art.name

                print_nfo("[Deezer] [Album] '{0} by {1}'." \
                          .format(to_ascii(album_name), \
                                  to_ascii(artist_name)))

                if not album:
                    if arg.lower() == album_name.lower():
                        album = alb
                    if not tentative_album:
                        if arg.lower() in album_name.lower():
                            tentative_album = alb

            if not album and not tentative_album:
                raise KeyError("Album not found : {0}".format(arg))

            if not album and tentative_album:
                album = tentative_album

            album_id = album.uri[len('deezer:album:'):]
            tracks = self.__api.lookup_album(album_id)
            for track in tracks:
                print_nfo("[Deezer] [Album track] '{0}'." \
                          .format(to_ascii(track.name)))

            if not tracks or not len(tracks):
                raise KeyError

            self.__enqueue_tracks(tracks)
            print_wrn("[Deezer] Playing '{0}'." \
                      .format(to_ascii(album.name)))
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))

    def stream_current_track(self):
        """  Return coroutine with seeking capabilities: some_stream.send(30000)

        """
        logging.info("stream_current_track %s - %s", \
                     self.now_playing_track.name, self.now_playing_track.uri)
        data = next(self.now_playing_stream)
        data_len = 0
        if data:
            data_len = len(data)
        return data, data_len

    def __enqueue_tracks(self, tracks):
        """ Add tracks to the playback queue

        """
        count = 0
        for track in tracks:
            self.queue.append(track)
            count += 1
        return count

    def next_track(self):
        """ Retrieve the next track in the playback queue.

        """
        logging.info("next_track START")

        if len(self.queue):
            self.queue_index += 1
            if (self.queue_index < len(self.queue)) \
               and (self.queue_index >= 0):
                next_track = self.queue[self.play_queue_order[self.queue_index]]
                uri = self.__retrieve_track_uri(next_track)
                logging.info("next_track END")
                return to_ascii(uri)
            else:
                self.queue_index = -1
                return self.next_track()
        else:
            return ''

    def prev_track(self):
        """ Retrieve the previous track in the playback queue.

        """
        if len(self.queue):
            self.queue_index -= 1
            if (self.queue_index < len(self.queue)) \
               and (self.queue_index >= 0):
                prev_track = self.queue[self.play_queue_order[self.queue_index]]
                return self.__retrieve_track_uri(prev_track)
            else:
                self.queue_index = len(self.queue)
                return self.prev_track()
        else:
            return ''

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
            print_nfo("[Google Play Music] [Tracks in queue] '{0}'." \
                      .format(total_tracks))

    def __retrieve_track_uri(self, track):
        """ Retrieve a track uri

        """
        logging.info("__retrieve_track_uri START")

        try:
            track_url = track.uri
            track_id = track.uri[len('deezer:track:'):]
            self.now_playing_track = track
            if self.now_playing_stream:
                self.now_playing_stream.close()
            self.now_playing_stream = self.__api.stream(track_id)
            next(self.now_playing_stream)
            return track_url
        except AttributeError:
            logging.info("Could not retrieve the track url!")
            raise

if __name__ == "__main__":
    tizdeezerproxy()

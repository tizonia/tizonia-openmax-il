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
# import pprint

logging.captureWarnings(True)

if os.environ.get('TIZONIA_DEEZERPROXY_DEBUG'):
    from traceback import print_exception
    logging.getLogger().setLevel(logging.DEBUG)
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
        self.now_playing_track_data = None

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
        track_data = self.now_playing_track_data
        if track_data:
            title = to_ascii(track_data['SNG_TITLE'])
            artist = to_ascii(track_data['ART_NAME'])
            logging.info("Now playing %s by %s", title, artist)
        return title, artist

    def current_track_album_and_duration(self):
        """ Retrieve the current track's album and duration.

        """
        logging.info("current_track_album_and_duration")
        album_name = ''
        duration = 0
        track_data = self.now_playing_track_data
        if track_data:
            album_name = to_ascii(track_data['ALB_TITLE'])
            duration = to_ascii(track_data['DURATION'])
        logging.info("album %s duration %s", album_name, duration)
        return album_name, int(duration)

    def current_track_file_size(self):
        """ Retrieve the current track's file size.

        """
        size = 0
        track_data = self.now_playing_track_data
        if track_data:
            size = to_ascii(track_data['FILESIZE_MP3_320'])
        logging.info("file size %s", size)
        return int(size)

    def enqueue_tracks(self, arg):
        """Search for tracks with a given name and adds them to the playback queue.

        """
        try:
            logging.info("Tracks search %s", arg)
            [artists, albums, tracks] = self.__api.search(arg)

            if not len(tracks):
                # Ok, nothing came out of the searc. Try something else, at
                # least there will be some noise.
                print_wrn("[Deezer] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(to_ascii(arg)))
                [artists, albums, tracks] = self.__api.search("Top")

            for track in tracks:
                print_nfo("[Deezer] [Track] '{0}'." \
                          .format(to_ascii(track.name)))

            if not tracks or not len(tracks):
                raise KeyError

            self.__enqueue_tracks(tracks)
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Track not found : {0}".format(arg))

    def enqueue_album(self, arg):
        """Search for albums with a given name and adds them to the playback queue.

        """
        try:
            logging.info("Album search %s", arg)
            [artists, albums, tracks] = self.__api.search(arg)

            if not len(albums):
                # Ok, nothing came out of the searc. Try something else, at
                # least there will be some noise.
                [artists, albums, tracks] = self.__api.search("Top Hits")

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

            if not album and tentative_album:
                album = tentative_album

            if not album and not tentative_album:
                print_wrn("[Deezer] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(to_ascii(arg)))
                random.seed()
                album = random.choice(albums)

            if not album:
                raise KeyError("Album not found : {0}".format(arg))

            album_id = album.uri[len('deezer:album:'):]
            tracks = self.__api.lookup_album(album_id)

            if not tracks or not len(tracks):
                raise KeyError

            print_wrn("[Deezer] [Album] '{0}'." \
                      .format(to_ascii(album.name)))

            for track in tracks:
                print_nfo("[Deezer] [Album track] '{0}'." \
                          .format(to_ascii(track.name)))

            self.__enqueue_tracks(tracks)
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))

    def enqueue_artist(self, arg):
        """ Search for artists with a given name and adds
        them to the playback queue.

        """
        try:
            logging.info("Artist search %s", arg)
            [artists, albums, tracks] = self.__api.search(arg)

            if not len(artists):
                # Ok, nothing came out of the searc. Try something else, at
                # least there will be some noise.
                [artists, albums, tracks] = self.__api.search("Best")

            artist = None
            tentative_artist = None

            for art in artists:
                artist_name = art.name
                print_nfo("[Deezer] [Artist] '{0}'." \
                          .format(to_ascii(artist_name)))

                if not artist:
                    if arg.lower() == artist_name.lower():
                        artist = art
                    if not tentative_artist:
                        if arg.lower() in artist_name.lower():
                            tentative_artist = art

            if not artist and tentative_artist:
                artist = tentative_artist

            if not artist and not tentative_artist:
                print_wrn("[Deezer] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(to_ascii(arg)))
                random.seed()
                artist = random.choice(artists)

            if not artist:
                raise KeyError("Artist not found : {0}".format(arg))

            artist_id = artist.uri[len('deezer:artist:'):]
            tracks = self.__api.lookup_artist(artist_id)
            for track in tracks:
                print_nfo("[Deezer] [Artist track] '{0}'." \
                          .format(to_ascii(track.name)))

            if not tracks or not len(tracks):
                raise KeyError

            self.__enqueue_tracks(tracks)
            print_wrn("[Deezer] Playing '{0}'." \
                      .format(to_ascii(artist.name)))
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Artist not found : {0}".format(arg))

    def enqueue_mix(self, arg):
        """ Search for mixes with a given name and adds
        them to the playback queue.

        """
        try:
            logging.info("Mix search %s", arg)
            mixes = self.__api.browse_mixes()

            mix = None
            tentative_mix = None

            for m in mixes:
                mix_name = m.name
                print_nfo("[Deezer] [Mix] '{0}'." \
                          .format(to_ascii(mix_name)))

                if not mix:
                    if arg.lower() == mix_name.lower():
                        mix = m
                    if not tentative_mix:
                        if arg.lower() in mix_name.lower():
                            tentative_mix = m

            if not mix and tentative_mix:
                mix = tentative_mix

            if not mix and not tentative_mix:
                print_wrn("[Deezer] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(to_ascii(arg)))
                random.seed()
                mix = random.choice(mixes)

            if not mix:
                raise KeyError("Mix not found : {0}".format(arg))

            mix_id = mix.uri[len('deezer:radio:'):]
            tracks = self.__api.lookup_radio(mix_id)
            for track in tracks:
                print_nfo("[Deezer] [Mix track] '{0}'." \
                          .format(to_ascii(track.name)))

            if not tracks or not len(tracks):
                raise KeyError

            self.__enqueue_tracks(tracks)
            print_wrn("[Deezer] Playing '{0}'." \
                      .format(to_ascii(mix.name)))
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Mix not found : {0}".format(arg))

    # This does not work yet
    def enqueue_playlist(self, arg):
        """ Search for playlists with a given name and adds
        them to the playback queue.

        """
        try:
            logging.info("Playlist search %s", arg)
            playlists = self.__api.browse_playlists()
            pprint.pprint(playlists)

            playlist = None
            tentative_playlist = None

            for m in playlists:
                playlist_name = m.name
                print_nfo("[Deezer] [Playlist] '{0}'." \
                          .format(to_ascii(playlist_name)))

                if not playlist:
                    if arg.lower() == playlist_name.lower():
                        playlist = m
                    if not tentative_playlist:
                        if arg.lower() in playlist_name.lower():
                            tentative_playlist = m

            if not playlist and tentative_playlist:
                playlist = tentative_playlist

            if not playlist and not tentative_playlist:
                print_wrn("[Deezer] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(to_ascii(arg)))
                random.seed()
                playlist = random.choice(playlists)

            if not playlist:
                raise KeyError("Playlist not found : {0}".format(arg))

            playlist_id = playlist.uri[len('deezer:radio:'):]
            tracks = self.__api.lookup_playlist(playlist_id)
            for track in tracks:
                print_nfo("[Deezer] [Playlist track] '{0}'." \
                          .format(to_ascii(track.name)))

            if not tracks or not len(tracks):
                raise KeyError

            self.__enqueue_tracks(tracks)
            print_wrn("[Deezer] Playing '{0}'." \
                      .format(to_ascii(playlist.name)))
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Playlist not found : {0}".format(arg))

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
                logging.info("next_track uri %s END", to_ascii(uri))
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
                uri = self.__retrieve_track_uri(prev_track)
                logging.info("prev_track uri %s END", to_ascii(uri))
                return to_ascii(uri)
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
            print_nfo("[Deezer] [Tracks in queue] '{0}'." \
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
            self.now_playing_stream = self.__stream_track(track_id)
            next(self.now_playing_stream)
            return track_url
        except AttributeError:
            logging.info("Could not retrieve the track url!")
            raise

    def __stream_track(self, track_id):
        """ Return coroutine with seeking capabilities: some_stream.send(30000)

        """
        logging.info("__stream_track")

        track_data = self.__api.get_track(track_id)
        self.now_playing_track_data = track_data
        track_cipher = self.__api.get_track_cipher(track_data['SNG_ID'])
        track_url = self.__api.get_track_url(track_data)
        return self.__api._stream(track_cipher, track_url)

        self.__api.stream(track_id)

if __name__ == "__main__":
    tizdeezerproxy()

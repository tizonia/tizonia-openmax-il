# Copyright (C) 2017 Aratelia Limited - Juan A. Rubio
#
# Portions Copyright (C) 2014 Dan Nixon
# (see https://github.com/DanNixon/PlayMusicCL)
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


"""@package tizgmusicproxy
Simple Google Play Music proxy/wrapper.

Access a Google Music account to retrieve song URLs and create a play queue for
streaming. With ideas from Dan Nixon's command-line client, which in turn uses
Simon Weber's 'gmusicapi' Python module. For further information:

- https://github.com/DanNixon/PlayMusicCL
- https://github.com/simon-weber/Unofficial-Google-Music-API

"""

import os
import sys
import logging
import random
import unicodedata
import pickle
from operator import itemgetter
from gmusicapi import Mobileclient
from gmusicapi.exceptions import CallFailure
from requests.structures import CaseInsensitiveDict

# For use during debugging
# import pprint


logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get('TIZONIA_GMUSICPROXY_DEBUG'):
    from traceback import print_exception
else:
    logging.getLogger().addHandler(logging.NullHandler())

MAX_TRACKS = 200

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
    print_err("[Google Play Music] (%s) : %s" \
              % (exception_type.__name__, exception))

    if os.environ.get('TIZONIA_GMUSICPROXY_DEBUG'):
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

class tizgmusicproxy(object):
    """A class for logging into a Google Play Music account and retrieving song
    URLs.

    """

    all_songs_album_title = "All Songs"
    thumbs_up_playlist_name = "Thumbs Up"

    # pylint: disable=too-many-instance-attributes,too-many-public-methods
    def __init__(self, email, password, device_id):
        self.__gmusic = Mobileclient()
        self.__email = email
        self.__device_id = device_id
        self.logged_in = False
        self.queue = list()
        self.queue_index = -1
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_song = None

        userdir = os.path.expanduser('~')
        tizconfig = os.path.join(userdir, ".config/tizonia/." + email + ".auth_token")
        auth_token = ""
        if os.path.isfile(tizconfig):
            with open(tizconfig, "r") as f:
                auth_token = pickle.load(f)
                if auth_token:
                    # 'Keep track of the auth token' workaround. See:
                    # https://github.com/diraimondo/gmusicproxy/issues/34#issuecomment-147359198
                    print_msg("[Google Play Music] [Authenticating] : " \
                              "'with cached auth token'")
                    self.__gmusic.android_id = device_id
                    self.__gmusic.session._authtoken = auth_token
                    self.__gmusic.session.is_authenticated = True
                    try:
                        self.__gmusic.get_registered_devices()
                    except CallFailure:
                        # The token has expired. Reset the client object
                        print_wrn("[Google Play Music] [Authenticating] : " \
                                  "'auth token expired'")
                        self.__gmusic = Mobileclient()
                        auth_token = ""

        if not auth_token:
            attempts = 0
            print_nfo("[Google Play Music] [Authenticating] : " \
                      "'with user credentials'")
            while not self.logged_in and attempts < 3:
                self.logged_in = self.__gmusic.login(email, password, device_id)
                attempts += 1

            with open(tizconfig, "a+") as f:
                f.truncate()
                pickle.dump(self.__gmusic.session._authtoken, f)

        self.library = CaseInsensitiveDict()
        self.song_map = CaseInsensitiveDict()
        self.playlists = CaseInsensitiveDict()
        self.stations = CaseInsensitiveDict()

    def logout(self):
        """ Reset the session to an unauthenticated, default state.

        """
        self.__gmusic.logout()

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: curren tvalid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)
        self.__update_play_queue_order()

    def current_song_title_and_artist(self):
        """ Retrieve the current track's title and artist name.

        """
        logging.info("current_song_title_and_artist")
        song = self.now_playing_song
        if song:
            title = to_ascii(song.get('title'))
            artist = to_ascii(song.get('artist'))
            logging.info("Now playing %s by %s", title, artist)
            return artist, title
        else:
            return '', ''

    def current_song_album_and_duration(self):
        """ Retrieve the current track's album and duration.

        """
        logging.info("current_song_album_and_duration")
        song = self.now_playing_song
        if song:
            album = to_ascii(song.get('album'))
            duration = to_ascii \
                       (song.get('durationMillis'))
            logging.info("album %s duration %s", album, duration)
            return album, int(duration)
        else:
            return '', 0

    def current_track_and_album_total(self):
        """Return the current track number and the total number of tracks in the
        album, if known.

        """
        logging.info("current_track_and_album_total")
        song = self.now_playing_song
        track = 0
        total = 0
        if song:
            try:
                track = song['trackNumber']
                total = song['totalTrackCount']
                logging.info("track number %s total tracks %s", track, total)
            except KeyError:
                logging.info("trackNumber or totalTrackCount : not found")
        else:
            logging.info("current_song_track_number_"
                         "and_total_tracks : not found")
        return track, total

    def current_song_year(self):
        """ Return the current track's year of publication.

        """
        logging.info("current_song_year")
        song = self.now_playing_song
        year = 0
        if song:
            try:
                year = song['year']
                logging.info("track year %s", year)
            except KeyError:
                logging.info("year : not found")
        else:
            logging.info("current_song_year : not found")
        return year

    def current_song_genre(self):
        """ Return the current track's genre.

        """
        logging.info("current_song_genre")
        song = self.now_playing_song
        if song:
            genre = to_ascii(song.get('genre'))
            logging.info("genre %s", genre)
            return genre
        else:
            return ''

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1

    def enqueue_tracks(self, arg):
        """ Search the user's library for tracks and add
        them to the playback queue.

        :param arg: a track search term
        """
        try:
            songs = self.__gmusic.get_all_songs()

            track_hits = list ()
            for song in songs:
                song_title = song['title']
                if arg.lower() in song_title.lower():
                    track_hits.append(song)
                    print_nfo("[Google Play Music] [Track] '{0}'." \
                              .format(to_ascii(song_title)))

            if not len(track_hits):
                print_wrn("[Google Play Music] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(arg.encode('utf-8')))
                random.seed()
                track_hits = random.sample(songs, MAX_TRACKS)
                for hit in track_hits:
                    song_title = hit['title']
                    print_nfo("[Google Play Music] [Track] '{0}'." \
                              .format(to_ascii(song_title)))

            if not len(track_hits):
                raise KeyError

            tracks_added = self.__enqueue_tracks(track_hits)
            logging.info("Added %d tracks from %s to queue", \
                         tracks_added, arg)

            self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Track not found : {0}".format(arg))

    def enqueue_artist(self, arg):
        """ Search the user's library for tracks from the given artist and add
        them to the playback queue.

        :param arg: an artist
        """
        try:
            self.__update_local_library()
            artist = None
            artist_dict = None
            if arg not in self.library.keys():
                for name, art in self.library.iteritems():
                    if arg.lower() in name.lower():
                        artist = name
                        artist_dict = art
                        if arg.lower() != name.lower():
                            print_wrn("[Google Play Music] '{0}' not found. " \
                                      "Playing '{1}' instead." \
                                      .format(arg.encode('utf-8'), \
                                              name.encode('utf-8')))
                        break
                if not artist:
                    # Play some random artist from the library
                    random.seed()
                    artist = random.choice(self.library.keys())
                    artist_dict = self.library[artist]
                    print_wrn("[Google Play Music] '{0}' not found. "\
                              "Feeling lucky?." \
                              .format(arg.encode('utf-8')))
            else:
                artist = arg
                artist_dict = self.library[arg]
            tracks_added = 0
            for album in artist_dict:
                tracks_added += self.__enqueue_tracks(artist_dict[album])
            print_wrn("[Google Play Music] Playing '{0}'." \
                      .format(to_ascii(artist)))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Artist not found : {0}".format(arg))

    def enqueue_album(self, arg):
        """ Search the user's library for albums with a given name and add
        them to the playback queue.

        """
        try:
            self.__update_local_library()
            album = None
            artist = None
            tentative_album = None
            tentative_artist = None
            for library_artist in self.library:
                for artist_album in self.library[library_artist]:
                    print_nfo("[Google Play Music] [Album] '{0}'." \
                              .format(to_ascii(artist_album)))
                    if not album:
                        if arg.lower() == artist_album.lower():
                            album = artist_album
                            artist = library_artist
                            break
                    if not tentative_album:
                        if arg.lower() in artist_album.lower():
                            tentative_album = artist_album
                            tentative_artist = library_artist
                if album:
                    break

            if not album and tentative_album:
                album = tentative_album
                artist = tentative_artist
                print_wrn("[Google Play Music] '{0}' not found. " \
                          "Playing '{1}' instead." \
                          .format(arg.encode('utf-8'), \
                          album.encode('utf-8')))
            if not album:
                # Play some random album from the library
                random.seed()
                artist = random.choice(self.library.keys())
                album = random.choice(self.library[artist].keys())
                print_wrn("[Google Play Music] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(arg.encode('utf-8')))

            if not album:
                raise KeyError("Album not found : {0}".format(arg))

            self.__enqueue_tracks(self.library[artist][album])
            print_wrn("[Google Play Music] Playing '{0}'." \
                      .format(to_ascii(album)))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))

    def enqueue_playlist(self, arg):
        """Search the user's library for playlists with a given name
        and add the tracks of the first match to the playback queue.

        Requires Unlimited subscription.

        """
        try:
            self.__update_local_library()
            self.__update_playlists()
            self.__update_playlists_unlimited()
            playlist = None
            playlist_name = None
            for name, plist in self.playlists.items():
                print_nfo("[Google Play Music] [Playlist] '{0}'." \
                          .format(to_ascii(name)))
            if arg not in self.playlists.keys():
                for name, plist in self.playlists.iteritems():
                    if arg.lower() in name.lower():
                        playlist = plist
                        playlist_name = name
                        if arg.lower() != name.lower():
                            print_wrn("[Google Play Music] '{0}' not found. " \
                                      "Playing '{1}' instead." \
                                      .format(arg.encode('utf-8'), \
                                              to_ascii(name)))
                            break
            else:
                playlist_name = arg
                playlist = self.playlists[arg]

            random.seed()
            x = 0
            while (not playlist or not len(playlist)) and x < 3:
                x += 1
                # Play some random playlist from the library
                playlist_name = random.choice(self.playlists.keys())
                playlist = self.playlists[playlist_name]
                print_wrn("[Google Play Music] '{0}' not found or found empty. "\
                          "Feeling lucky?." \
                          .format(arg.encode('utf-8')))

            if not len(playlist):
                raise KeyError

            self.__enqueue_tracks(playlist)
            print_wrn("[Google Play Music] Playing '{0}'." \
                      .format(to_ascii(playlist_name)))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Playlist not found or found empty : {0}".format(arg))

    def enqueue_podcast(self, arg):
        """Search Google Play Music for a podcast series and add its tracks to the
        playback queue ().

        Requires Unlimited subscription.

        """
        print_msg("[Google Play Music] [Retrieving podcasts] : '{0}'. " \
                  .format(self.__email))

        try:

            self.__enqueue_podcast(arg)

            if not len(self.queue):
                raise KeyError

            logging.info("Added %d episodes from '%s' to queue", \
                         len(self.queue), arg)
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Podcast not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_station_unlimited(self, arg):
        """Search the user's library for a station with a given name
        and add its tracks to the playback queue.

        Requires Unlimited subscription.

        """
        try:
            # First try to find a suitable station in the user's library
            self.__enqueue_user_station_unlimited(arg)

            if not len(self.queue):
                # If no suitable station is found in the user's library, then
                # search google play unlimited for a potential match.
                self.__enqueue_station_unlimited(arg)

            if not len(self.queue):
                raise KeyError

        except KeyError:
            raise KeyError("Station not found : {0}".format(arg))

    def enqueue_genre_unlimited(self, arg):
        """Search Unlimited for a genre with a given name and add its
        tracks to the playback queue.

        Requires Unlimited subscription.

        """
        print_msg("[Google Play Music] [Retrieving genres] : '{0}'. " \
                  .format(self.__email))

        try:
            all_genres = list()
            root_genres = self.__gmusic.get_genres()
            second_tier_genres = list()
            for root_genre in root_genres:
                second_tier_genres += self.__gmusic.get_genres(root_genre['id'])
            all_genres += root_genres
            all_genres += second_tier_genres
            for genre in all_genres:
                print_nfo("[Google Play Music] [Genre] '{0}'." \
                          .format(to_ascii(genre['name'])))
            genre = dict()
            if arg not in all_genres:
                genre = next((g for g in all_genres \
                              if arg.lower() in to_ascii(g['name']).lower()), \
                             None)

            tracks_added = 0
            while not tracks_added:
                if not genre and len(all_genres):
                    # Play some random genre from the search results
                    random.seed()
                    genre = random.choice(all_genres)
                    print_wrn("[Google Play Music] '{0}' not found. "\
                              "Feeling lucky?." \
                              .format(arg.encode('utf-8')))

                genre_name = genre['name']
                genre_id = genre['id']
                station_id = self.__gmusic.create_station(genre_name, \
                                                          None, None, None, genre_id)
                num_tracks = MAX_TRACKS
                tracks = self.__gmusic.get_station_tracks(station_id, num_tracks)
                tracks_added = self.__enqueue_tracks(tracks)
                logging.info("Added %d tracks from %s to queue", tracks_added, genre_name)
                if not tracks_added:
                    # This will produce another iteration in the loop
                    genre = None

            print_wrn("[Google Play Music] Playing '{0}'." \
                      .format(to_ascii(genre['name'])))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Genre not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_situation_unlimited(self, arg):
        """Search Unlimited for a situation with a given name and add its
        tracks to the playback queue.

        Requires Unlimited subscription.

        """
        print_msg("[Google Play Music] [Retrieving situations] : '{0}'. " \
                  .format(self.__email))

        try:

            self.__enqueue_situation_unlimited(arg)

            if not len(self.queue):
                raise KeyError

            logging.info("Added %d tracks from %s to queue", \
                         len(self.queue), arg)

        except KeyError:
            raise KeyError("Situation not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_artist_unlimited(self, arg):
        """Search Unlimited for an artist and add the artist's 200 top tracks to the
        playback queue.

        Requires Unlimited subscription.

        """
        try:
            artist = self.__gmusic_search(arg, 'artist')

            include_albums = False
            max_top_tracks = MAX_TRACKS
            max_rel_artist = 0
            artist_tracks = dict()
            if artist:
                artist_tracks = self.__gmusic.get_artist_info \
                                (artist['artist']['artistId'],
                                 include_albums, max_top_tracks,
                                 max_rel_artist)['topTracks']

            if not artist_tracks:
                raise KeyError

            for track in artist_tracks:
                song_title = track['title']
                print_nfo("[Google Play Music] [Track] '{0}'." \
                          .format(to_ascii(song_title)))

            tracks_added = self.__enqueue_tracks(artist_tracks)
            logging.info("Added %d tracks from %s to queue", \
                         tracks_added, arg)
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Artist not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_album_unlimited(self, arg):
        """Search Unlimited for an album and add its tracks to the
        playback queue.

        Requires Unlimited subscription.

        """
        try:
            album = self.__gmusic_search(arg, 'album')
            album_tracks = dict()
            if album:
                album_tracks = self.__gmusic.get_album_info \
                               (album['album']['albumId'])['tracks']
            if not album_tracks:
                raise KeyError

            print_wrn("[Google Play Music] Playing '{0}'." \
                      .format((album['album']['name']).encode('utf-8')))

            tracks_added = self.__enqueue_tracks(album_tracks)
            logging.info("Added %d tracks from %s to queue", \
                         tracks_added, arg)
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_tracks_unlimited(self, arg):
        """ Search Unlimited for a track name and add all the matching tracks
        to the playback queue.

        Requires Unlimited subscription.

        """
        print_msg("[Google Play Music] [Retrieving library] : '{0}'. " \
                  .format(self.__email))

        try:
            max_results = MAX_TRACKS
            track_hits = self.__gmusic.search(arg, max_results)['song_hits']
            if not len(track_hits):
                # Do another search with an empty string
                track_hits = self.__gmusic.search("", max_results)['song_hits']
                print_wrn("[Google Play Music] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(arg.encode('utf-8')))

            tracks = list()
            for hit in track_hits:
                tracks.append(hit['track'])
            tracks_added = self.__enqueue_tracks(tracks)
            logging.info("Added %d tracks from %s to queue", \
                         tracks_added, arg)
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Playlist not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_playlist_unlimited(self, arg):
        """Search Unlimited for a playlist name and add all its tracks to the
        playback queue.

        Requires Unlimited subscription.

        """
        print_msg("[Google Play Music] [Retrieving playlists] : '{0}'. " \
                  .format(self.__email))

        try:
            playlist_tracks = list()

            playlist_hits = self.__gmusic_search(arg, 'playlist')
            if playlist_hits:
                playlist = playlist_hits['playlist']
                playlist_contents = self.__gmusic.get_shared_playlist_contents(playlist['shareToken'])
            else:
                raise KeyError

            print_nfo("[Google Play Music] [Playlist] '{}'." \
                      .format(playlist['name']).encode('utf-8'))

            for item in playlist_contents:
                print_nfo("[Google Play Music] [Playlist Track] '{} by {} (Album: {}, {})'." \
                          .format((item['track']['title']).encode('utf-8'),
                                  (item['track']['artist']).encode('utf-8'),
                                  (item['track']['album']).encode('utf-8'),
                                  (item['track']['year'])))
                track = item['track']
                playlist_tracks.append(track)

            if not playlist_tracks:
                raise KeyError

            tracks_added = self.__enqueue_tracks(playlist_tracks)
            logging.info("Added %d tracks from %s to queue", \
                         tracks_added, arg)
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Playlist not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_promoted_tracks_unlimited(self):
        """ Retrieve the url of the next track in the playback queue.

        """
        try:
            tracks = self.__gmusic.get_promoted_songs()
            count = 0
            for track in tracks:
                store_track = self.__gmusic.get_track_info(track['storeId'])
                if u'id' not in store_track.keys():
                    store_track[u'id'] = store_track['storeId']
                self.queue.append(store_track)
                count += 1
            if count == 0:
                print_wrn("[Google Play Music] Operation requires " \
                          "an Unlimited subscription.")
            logging.info("Added %d Unlimited promoted tracks to queue", \
                         count)
            self.__update_play_queue_order()
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def next_url(self):
        """ Retrieve the url of the next track in the playback queue.

        """
        if len(self.queue):
            self.queue_index += 1
            if (self.queue_index < len(self.queue)) \
               and (self.queue_index >= 0):
                next_song = self.queue[self.play_queue_order[self.queue_index]]
                return self.__retrieve_track_url(next_song)
            else:
                self.queue_index = -1
                return self.next_url()
        else:
            return ''

    def prev_url(self):
        """ Retrieve the url of the previous track in the playback queue.

        """
        if len(self.queue):
            self.queue_index -= 1
            if (self.queue_index < len(self.queue)) \
               and (self.queue_index >= 0):
                prev_song = self.queue[self.play_queue_order[self.queue_index]]
                return self.__retrieve_track_url(prev_song)
            else:
                self.queue_index = len(self.queue)
                return self.prev_url()
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

    def __retrieve_track_url(self, song):
        """ Retrieve a song url

        """
        if song.get('episodeId'):
            song_url = self.__gmusic.get_podcast_episode_stream_url(song['episodeId'], self.__device_id)
        else:
            song_url = self.__gmusic.get_stream_url(song['id'], self.__device_id)

        try:
            self.now_playing_song = song
            return song_url
        except AttributeError:
            logging.info("Could not retrieve the song url!")
            raise

    def __update_local_library(self):
        """ Retrieve the songs and albums from the user's library

        """
        print_msg("[Google Play Music] [Retrieving library] : '{0}'. " \
                  .format(self.__email))

        songs = self.__gmusic.get_all_songs()
        self.playlists[self.thumbs_up_playlist_name] = list()

        # Retrieve the user's song library
        for song in songs:
            if "rating" in song and song['rating'] == "5":
                self.playlists[self.thumbs_up_playlist_name].append(song)

            song_id = song['id']
            song_artist = song['artist']
            song_album = song['album']

            self.song_map[song_id] = song

            if song_artist == "":
                song_artist = "Unknown Artist"

            if song_album == "":
                song_album = "Unknown Album"

            if song_artist not in self.library:
                self.library[song_artist] = CaseInsensitiveDict()
                self.library[song_artist][self.all_songs_album_title] = list()

            if song_album not in self.library[song_artist]:
                self.library[song_artist][song_album] = list()

            self.library[song_artist][song_album].append(song)
            self.library[song_artist][self.all_songs_album_title].append(song)

        # Sort albums by track number
        for artist in self.library.keys():
            logging.info("Artist : %s", to_ascii(artist))
            for album in self.library[artist].keys():
                logging.info("   Album : %s", to_ascii(album))
                if album == self.all_songs_album_title:
                    sorted_album = sorted(self.library[artist][album],
                                          key=lambda k: k['title'])
                else:
                    sorted_album = sorted(self.library[artist][album],
                                          key=lambda k: k.get('trackNumber',
                                                              0))
                self.library[artist][album] = sorted_album

    def __update_stations_unlimited(self):
        """ Retrieve stations (Unlimited)

        """
        self.stations.clear()
        stations = self.__gmusic.get_all_stations()
        self.stations[u"I'm Feeling Lucky"] = 'IFL'
        for station in stations:
            station_name = station['name']
            logging.info("station name : %s", to_ascii(station_name))
            self.stations[station_name] = station['id']

    def __enqueue_user_station_unlimited(self, arg):
        """ Enqueue a user station (Unlimited)

        """
        print_msg("[Google Play Music] [Station search "\
                  "in user's library] : '{0}'. " \
                  .format(self.__email))
        self.__update_stations_unlimited()
        station_name = arg
        station_id = None
        for name, st_id in self.stations.iteritems():
            print_nfo("[Google Play Music] [Station] '{0}'." \
                      .format(to_ascii(name)))
        if arg not in self.stations.keys():
            for name, st_id in self.stations.iteritems():
                if arg.lower() in name.lower():
                    station_id = st_id
                    station_name = name
                    break
        else:
            station_id = self.stations[arg]

        num_tracks = MAX_TRACKS
        tracks = list()
        if station_id:
            try:
                tracks = self.__gmusic.get_station_tracks(station_id, \
                                                          num_tracks)
            except KeyError:
                raise RuntimeError("Operation requires an "
                                   "Unlimited subscription.")
            tracks_added = self.__enqueue_tracks(tracks)
            if tracks_added:
                if arg.lower() != station_name.lower():
                    print_wrn("[Google Play Music] '{0}' not found. " \
                              "Playing '{1}' instead." \
                              .format(arg.encode('utf-8'), name.encode('utf-8')))
                logging.info("Added %d tracks from %s to queue", tracks_added, arg)
                self.__update_play_queue_order()
            else:
                print_wrn("[Google Play Music] '{0}' has no tracks. " \
                          .format(station_name))

        if not len(self.queue):
            print_wrn("[Google Play Music] '{0}' " \
                      "not found in the user's library. " \
                      .format(arg.encode('utf-8')))

    def __enqueue_station_unlimited(self, arg, max_results=MAX_TRACKS, quiet=False):
        """Search for a station and enqueue all of its tracks (Unlimited)

        """
        if not quiet:
            print_msg("[Google Play Music] [Station search in "\
                      "Google Play Music] : '{0}'. " \
                      .format(arg.encode('utf-8')))
        try:
            station_name = arg
            station_id = None
            station = self.__gmusic_search(arg, 'station', max_results, quiet)

            if station:
                station = station['station']
                station_name = station['name']
                seed = station['seed']
                seed_type = seed['seedType']
                track_id = seed['trackId'] if seed_type == u'2' else None
                artist_id = seed['artistId'] if seed_type == u'3' else None
                album_id = seed['albumId'] if seed_type == u'4' else None
                genre_id = seed['genreId'] if seed_type == u'5' else None
                playlist_token = seed['playlistShareToken'] if seed_type == u'8' else None
                curated_station_id = seed['curatedStationId'] if seed_type == u'9' else None
                num_tracks = max_results
                tracks = list()
                try:
                    station_id \
                        = self.__gmusic.create_station(station_name, \
                                                       track_id, \
                                                       artist_id, \
                                                       album_id, \
                                                       genre_id, \
                                                       playlist_token, \
                                                       curated_station_id)
                    tracks \
                        = self.__gmusic.get_station_tracks(station_id, \
                                                           num_tracks)
                except KeyError:
                    raise RuntimeError("Operation requires an "
                                       "Unlimited subscription.")
                tracks_added = self.__enqueue_tracks(tracks)
                if tracks_added:
                    if not quiet:
                        print_wrn("[Google Play Music] [Station] : '{0}'." \
                                  .format(station_name.encode('utf-8')))
                    logging.info("Added %d tracks from %s to queue", \
                                 tracks_added, arg.encode('utf-8'))
                    self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Station not found : {0}".format(arg))

    def __enqueue_situation_unlimited(self, arg):
        """Search for a situation and enqueue all of its tracks (Unlimited)

        """
        print_msg("[Google Play Music] [Situation search in "\
                  "Google Play Music] : '{0}'. " \
                  .format(arg.encode('utf-8')))
        try:
            situation_hits = self.__gmusic.search(arg)['situation_hits']

            # If the search didn't return results, just do another search with
            # an empty string
            if not len(situation_hits):
                situation_hits = self.__gmusic.search("")['situation_hits']
                print_wrn("[Google Play Music] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(arg.encode('utf-8')))

            # Try to find a "best result", if one exists
            situation = next((hit for hit in situation_hits \
                              if 'best_result' in hit.keys() \
                              and hit['best_result'] == True), None)

            num_tracks = MAX_TRACKS

            # If there is no best result, then get a selection of tracks from
            # each situation. At least we'll play some music.
            if not situation and len(situation_hits):
                max_results = num_tracks / len(situation_hits)
                for hit in situation_hits:
                    situation = hit['situation']
                    print_nfo("[Google Play Music] [Situation] '{0} : {1}'." \
                              .format((hit['situation']['title']).encode('utf-8'),
                                      (hit['situation']['description']).encode('utf-8')))
                    self.__enqueue_station_unlimited(situation['title'], max_results, True)
            elif situation:
                # There is at list one sitution, enqueue its tracks.
                situation = situation['situation']
                max_results = num_tracks
                self.__enqueue_station_unlimited(situation['title'], max_results, True)

            if not situation:
                raise KeyError

        except KeyError:
           raise KeyError("Situation not found : {0}".format(arg))

    def __enqueue_podcast(self, arg):
        """Search for a podcast series and enqueue all of its tracks.

        """
        print_msg("[Google Play Music] [Podcast search in "\
                  "Google Play Music] : '{0}'. " \
                  .format(arg.encode('utf-8')))
        try:
            podcast_hits = self.__gmusic_search(arg, 'podcast', 10, quiet=False)

            if not podcast_hits:
                print_wrn("[Google Play Music] [Podcast] 'Search returned zero results'.")
                print_wrn("[Google Play Music] [Podcast] 'Are you in a supported region "
                          "(currently only US and Canada) ?'")

            # Use the first podcast retrieved. At least we'll play something.
            podcast = dict ()
            if podcast_hits and len(podcast_hits):
                podcast = podcast_hits['series']

            episodes_added = 0
            if podcast:
                # There is a podcast, enqueue its episodes.
                print_nfo("[Google Play Music] [Podcast] 'Playing '{0}' by {1}'." \
                          .format((podcast['title']).encode('utf-8'),
                                  (podcast['author']).encode('utf-8')))
                print_nfo("[Google Play Music] [Podcast] '{0}'." \
                          .format((podcast['description'][0:150]).encode('utf-8')))
                series = self.__gmusic.get_podcast_series_info(podcast['seriesId'])
                episodes = series['episodes']
                for episode in episodes:
                    print_nfo("[Google Play Music] [Podcast Episode] '{0} : {1}'." \
                              .format((episode['title']).encode('utf-8'),
                                      (episode['description'][0:80]).encode('utf-8')))
                episodes_added = self.__enqueue_tracks(episodes)

            if not podcast or not episodes_added:
                raise KeyError

        except KeyError:
            raise KeyError("Podcast not found or no episodes found: {0}".format(arg))

    def __enqueue_tracks(self, tracks):
        """ Add tracks to the playback queue

        """
        count = 0
        for track in tracks:
            if u'id' not in track.keys() and track.get('storeId'):
                track[u'id'] = track['storeId']
            self.queue.append(track)
            count += 1
        return count

    def __update_playlists(self):
        """ Retrieve the user's playlists

        """
        plists = self.__gmusic.get_all_user_playlist_contents()
        for plist in plists:
            plist_name = plist.get('name')
            tracks = plist.get('tracks')
            if plist_name and tracks:
                logging.info("playlist name : %s", to_ascii(plist_name))
                tracks.sort(key=itemgetter('creationTimestamp'))
                self.playlists[plist_name] = list()
                for track in tracks:
                    song_id = track.get('trackId')
                    if song_id:
                        song = self.song_map.get(song_id)
                        if song:
                            self.playlists[plist_name].append(song)

    def __update_playlists_unlimited(self):
        """ Retrieve shared playlists (Unlimited)

        """
        plists_subscribed_to = [p for p in self.__gmusic.get_all_playlists() \
                                if p.get('type') == 'SHARED']
        for plist in plists_subscribed_to:
            share_tok = plist['shareToken']
            playlist_items \
                = self.__gmusic.get_shared_playlist_contents(share_tok)
            plist_name = plist['name']
            logging.info("shared playlist name : %s", to_ascii(plist_name))
            self.playlists[plist_name] = list()
            for item in playlist_items:
                try:
                    song = item['track']
                    song['id'] = item['trackId']
                    self.playlists[plist_name].append(song)
                except IndexError:
                    pass

    def __gmusic_search(self, query, query_type, max_results=MAX_TRACKS, quiet=False):
        """ Search Google Play (Unlimited)

        """

        search_results = self.__gmusic.search(query, max_results)[query_type + '_hits']

        # This is a workaround. Some podcast results come without these two
        # keys in the dictionary
        if query_type == "podcast" and len(search_results) \
           and not search_results[0].get('navigational_result'):
            for res in search_results:
                res[u'best_result'] = False
                res[u'navigational_result'] = False
                res[query_type] = res['series']

        result = ''
        if query_type != "playlist":
            result = next((hit for hit in search_results \
                           if 'best_result' in hit.keys() \
                           and hit['best_result'] == True), None)

        if not result and len(search_results):
            secondary_hit = None
            for hit in search_results:
                name = ''
                if hit[query_type].get('name'):
                    name = hit[query_type].get('name')
                elif hit[query_type].get('title'):
                    name = hit[query_type].get('title')
                if not quiet:
                    print_nfo("[Google Play Music] [{0}] '{1}'." \
                              .format(query_type.capitalize(),
                                      (name).encode('utf-8')))
                if query.lower() == \
                   to_ascii(name).lower():
                    result = hit
                    break
                if query.lower() in \
                   to_ascii(name).lower():
                    secondary_hit = hit
            if not result and secondary_hit:
                result = secondary_hit

        if not result and not len(search_results):
            # Do another search with an empty string
            search_results = self.__gmusic.search("")[query_type + '_hits']

        if not result and len(search_results):
            # Play some random result from the search results
            random.seed()
            result = random.choice(search_results)
            if not quiet:
                print_wrn("[Google Play Music] '{0}' not found. "\
                          "Feeling lucky?." \
                          .format(query.encode('utf-8')))

        return result

if __name__ == "__main__":
    tizgmusicproxy()

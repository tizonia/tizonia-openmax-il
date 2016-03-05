# Copyright (C) 2016 Aratelia Limited - Juan A. Rubio
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
# from traceback import print_exception

logging.captureWarnings(True)
logging.getLogger().addHandler(logging.NullHandler())
logging.getLogger().setLevel(logging.INFO)

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
    del traceback # unused
    #print_exception(exception_type, exception, traceback)

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
        if song is not None:
            title = to_ascii(self.now_playing_song.get('title'))
            artist = to_ascii(self.now_playing_song.get('artist'))
            logging.info("Now playing {0} by {1}".format(title, artist))
            return artist, title
        else:
            return '', ''

    def current_song_album_and_duration(self):
        """ Retrieve the current track's album and duration.

        """
        logging.info("current_song_album_and_duration")
        song = self.now_playing_song
        if song is not None:
            album = to_ascii(self.now_playing_song.get('album'))
            duration = to_ascii \
                       (self.now_playing_song.get('durationMillis'))
            logging.info("album {0} duration {1}".format(album, duration))
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
        if song is not None:
            try:
                track = self.now_playing_song['trackNumber']
                total = self.now_playing_song['totalTrackCount']
                logging.info("track number {0} total tracks {1}"
                             .format(track, total))
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
        if song is not None:
            try:
                year = song['year']
                logging.info("track year {0}".format(year))
            except KeyError:
                logging.info("year : not found")
        else:
            logging.info("current_song_year : not found")
        return year

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1

    def enqueue_artist(self, arg):
        """ Search the user's library for tracks from the given artist and adds
        them to the playback queue.

        :param arg: an artist
        """
        try:
            self.__update_local_library()
            artist = None
            if not arg in self.library.keys():
                for name, art in self.library.iteritems():
                    if arg.lower() in name.lower():
                        artist = art
                        print_wrn("[Google Play Music] '{0}' not found. " \
                                  "Playing '{1}' instead." \
                                  .format(arg, name))
                        break
                if not artist:
                    raise KeyError("Artist not found : {0}".format(arg))
            else:
                artist = self.library[arg]
            tracks_added = 0
            for album in artist:
                tracks_added += self.__enqueue_tracks(artist[album])
            logging.info("Added {0} tracks by {1} to queue" \
                         .format(tracks_added, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Artist not found : {0}".format(arg))

    def enqueue_album(self, arg):
        """ Search the user's library for albums with a given name and adds
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

            if not album:
                album = tentative_album
                artist = tentative_artist
                print_wrn("[Google Play Music] '{0}' not found. " \
                          "Playing '{1}' instead." \
                          .format(arg, album))
            if not album:
                raise KeyError("Album not found : {0}".format(arg))
            tracks_added = self.__enqueue_tracks(self.library[artist][album])
            logging.info("Added {0} tracks from {1} by " \
                         "{2} to queue" \
                         .format(tracks_added, album, artist))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))

    def enqueue_playlist(self, arg):
        """Search the user's library for playlists with a given name
        and adds the tracks of the first match to the playback queue.

        Requires Unlimited subscription.

        """
        try:
            self.__update_local_library()
            self.__update_playlists()
            self.__update_playlists_unlimited()
            playlist = None
            for name, plist in self.playlists.items():
                print_nfo("[Google Play Music] [Playlist] '{0}'." \
                          .format(to_ascii(name)))
            if not arg in self.playlists.keys():
                for name, plist in self.playlists.iteritems():
                    if arg.lower() in name.lower():
                        playlist = plist
                        print_wrn("[Google Play Music] '{0}' not found. " \
                                  "Playing '{1}' instead." \
                                  .format(arg, to_ascii(name)))
                        break
                if not playlist:
                    raise KeyError
            else:
                playlist = self.playlists[arg]

            tracks_added = self.__enqueue_tracks(playlist)
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(tracks_added, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Playlist not found : {0}".format(arg))

    def enqueue_station_unlimited(self, arg):
        """Search the user's library for a station with a given name
        and adds its tracks to the playback queue.

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
        """Search Unlimited for a genre with a given name and adds its
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
            if not arg in all_genres:
                genre = next((g for g in all_genres \
                              if arg.lower() in to_ascii(g['name']).lower()), \
                             None)
                if genre:
                    print_wrn("[Google Play Music] '{0}' not found. " \
                              "Playing '{1}' instead." \
                              .format(arg, to_ascii(genre['name'])))
                else:
                    raise KeyError("Genre not found : {0}".format(arg))
            genre_name = genre['name']
            genre_id = genre['id']
            station_id = self.__gmusic.create_station(genre_name, \
                                                   None, None, None, genre_id)
            num_tracks = 200
            tracks = self.__gmusic.get_station_tracks(station_id, num_tracks)
            tracks_added = self.__enqueue_tracks(tracks)
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(tracks_added, genre_name))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Genre not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_artist_unlimited(self, arg):
        """Search Unlimited for an artist and adds the artist's 200 top tracks to the
        playback queue.

        Requires Unlimited subscription.

        """
        try:
            artist_hits = self.__gmusic.search_all_access(arg)['artist_hits']
            artist = next((hit for hit in artist_hits \
                           if 'best_result' in hit.keys()), None)
            if not artist and len(artist_hits):
                for hit in artist_hits:
                    print_nfo("[Google Play Music] [Artist] '{0}'." \
                              .format((hit['artist']['name']).encode('utf-8')))
                artist = artist_hits[0]
                print_wrn("[Google Play Music] Playing '{0}'." \
                          .format((artist['artist']['name']).encode('utf-8')))

            include_albums = False
            max_top_tracks = 200
            max_rel_artist = 0
            artist_tracks = dict()
            if artist:
                artist_tracks = self.__gmusic.get_artist_info \
                                (artist['artist']['artistId'],
                                 include_albums, max_top_tracks,
                                 max_rel_artist)['topTracks']
            if not artist_tracks:
                raise KeyError

            tracks_added = self.__enqueue_tracks(artist_tracks)
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(tracks_added, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Artist not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_album_unlimited(self, arg):
        """Search Unlimited for an album and adds its tracks to the
        playback queue.

        Requires Unlimited subscription.

        """
        try:
            album_hits = self.__gmusic.search_all_access(arg)['album_hits']
            album = next((hit for hit in album_hits \
                          if 'best_result' in hit.keys()), None)

            if not album and len(album_hits):
                for hit in album_hits:
                    print_nfo("[Google Play Music] [Album] '{0}'." \
                              .format((hit['album']['name']).encode('utf-8')))
                album = album_hits[0]
                print_wrn("[Google Play Music] Playing '{0}'." \
                          .format((album['album']['name']).encode('utf-8')))

            album_tracks = dict()
            if album:
                album_tracks = self.__gmusic.get_album_info \
                               (album['album']['albumId'])['tracks']
            if not album_tracks:
                raise KeyError

            tracks_added = self.__enqueue_tracks(album_tracks)
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(tracks_added, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_tracks_unlimited(self, arg):
        """ Search Unlimited for a track name and adds all the matching tracks
        to the playback queue.

        Requires Unlimited subscription.

        """
        print_msg("[Google Play Music] [Retrieving library] : '{0}'. " \
                  .format(self.__email))

        try:
            track_hits = self.__gmusic.search_all_access(arg)['song_hits']
            tracks = list()
            for hit in track_hits:
                tracks.append(hit['track'])
            tracks_added = self.__enqueue_tracks(tracks)
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(tracks_added, arg))
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
                if not u'id' in store_track.keys():
                    store_track[u'id'] = store_track['nid']
                self.queue.append(store_track)
                count += 1
            if count == 0:
                print_wrn("[Google Play Music] Operation requires " \
                          "an Unlimited subscription.")
            logging.info("Added {0} Unlimited promoted tracks to queue" \
                         .format(count))
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
        logging.info("__retrieve_track_url : {0}".format(song['id']))
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

            if not song_artist in self.library:
                self.library[song_artist] = CaseInsensitiveDict()
                self.library[song_artist][self.all_songs_album_title] = list()

            if not song_album in self.library[song_artist]:
                self.library[song_artist][song_album] = list()

            self.library[song_artist][song_album].append(song)
            self.library[song_artist][self.all_songs_album_title].append(song)

        # Sort albums by track number
        for artist in self.library.keys():
            logging.info("Artist : {0}".format(to_ascii(artist)))
            for album in self.library[artist].keys():
                logging.info("   Album : {0}".format(to_ascii(album)))
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
            logging.info("station name : {0}" \
                         .format(to_ascii(station_name)))
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
        if not arg in self.stations.keys():
            for name, st_id in self.stations.iteritems():
                if arg.lower() in name.lower():
                    station_id = st_id
                    station_name = name
                    break
        else:
            station_id = self.stations[arg]

        num_tracks = 200
        tracks = dict()
        if station_id:
            try:
                tracks = self.__gmusic.get_station_tracks(station_id, \
                                                          num_tracks)
            except KeyError:
                raise RuntimeError("Operation requires an "
                                   "Unlimited subscription.")
            tracks_added = self.__enqueue_tracks(tracks)
            if tracks_added:
                if arg != station_name:
                    print_wrn("[Google Play Music] '{0}' not found. " \
                              "Playing '{1}' instead." \
                              .format(arg, name))
                logging.info("Added {0} tracks from {1} to queue" \
                             .format(tracks_added, arg))
                self.__update_play_queue_order()
            else:
                print_wrn("[Google Play Music] '{0}' has no tracks. " \
                          .format(station_name))

        if not len(self.queue):
            print_wrn("[Google Play Music] '{0}' unable to find a " \
                      "suitable station in user's library. " \
                      .format(arg))

    def __enqueue_station_unlimited(self, arg):
        """Search for a station and enqueue all of its tracks (Unlimited)

        """
        print_msg("[Google Play Music] [Station search in "\
                  "Google Play Music Unlimited] : '{0}'. " \
                  .format(arg))
        try:
            station_name = arg
            station_id = None
            station_hits = self.__gmusic.search_all_access(arg)['station_hits']
            station = next((hit for hit in station_hits \
                            if 'best_result' in hit.keys()), None)

            if not station and len(station_hits):
                for hit in station_hits:
                    print_nfo("[Google Play Music] [Station] '{0}'." \
                              .format((hit['station']['name']).encode('utf-8')))
                    if not station:
                        if arg.lower() in \
                           to_ascii(hit['station']['name']).lower():
                            station = hit
                if not station:
                    # Play some random station from the search results
                    random.seed()
                    station_index = random.randint(0, len(station_hits))
                    station = station_hits[station_index]

            if station:
                station = station['station']
                station_name = station['name']
                seed = station['seed']
                seed_type = seed['seedType']
                track_id = seed['trackId'] if seed_type == u'2' else None
                artist_id = seed['artistId'] if seed_type == u'3' else None
                album_id = seed['albumId'] if seed_type == u'4' else None
                genre_id = seed['genreId'] if seed_type == u'5' else None
                num_tracks = 200
                tracks = dict()
                try:
                    station_id \
                        = self.__gmusic.create_station(station_name, \
                                                       track_id, \
                                                       artist_id, \
                                                       album_id, \
                                                       genre_id)
                    tracks \
                        = self.__gmusic.get_station_tracks(station_id, \
                                                           num_tracks)
                except KeyError:
                    raise RuntimeError("Operation requires an "
                                       "Unlimited subscription.")
                tracks_added = self.__enqueue_tracks(tracks)
                if tracks_added:
                    print_wrn("[Google Play Music] [Station] : '{0}'." \
                              .format(station_name.encode('utf-8')))
                    logging.info("Added {0} tracks from {1} to queue" \
                                 .format(tracks_added, arg))
                    self.__update_play_queue_order()

        except KeyError:
            raise KeyError("Station not found : {0}".format(arg))

    def __enqueue_tracks(self, tracks):
        """ Add tracks to the playback queue

        """
        count = 0
        for track in tracks:
            if not u'id' in track.keys():
                track[u'id'] = track['nid']
            self.queue.append(track)
            count += 1
        return count

    def __update_playlists(self):
        """ Retrieve the user's playlists

        """
        plists = self.__gmusic.get_all_user_playlist_contents()
        for plist in plists:
            plist_name = plist['name']
            logging.info("playlist name : {0}".format(to_ascii(plist_name)))
            tracks = plist['tracks']
            tracks.sort(key=itemgetter('creationTimestamp'))
            self.playlists[plist_name] = list()
            for track in tracks:
                try:
                    song = self.song_map[track['trackId']]
                    self.playlists[plist_name].append(song)
                except IndexError:
                    pass

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
            logging.info("shared playlist name : {0}" \
                         .format(to_ascii(plist_name)))
            self.playlists[plist_name] = list()
            for item in playlist_items:
                try:
                    song = item['track']
                    song['id'] = item['trackId']
                    self.playlists[plist_name].append(song)
                except IndexError:
                    pass

if __name__ == "__main__":
    tizgmusicproxy()

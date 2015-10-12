# Copyright (C) 2015 Aratelia Limited - Juan A. Rubio
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

"""Simple Google PLay Music proxy class.

Access a user's Google Music account to retrieve song URLs to be used for
streaming. With ideas from Dan Nixon's command-line client, which in turn uses
Simon Weber's 'gmusicapi' Python module. For further information:

- https://github.com/DanNixon/PlayMusicCL
- https://github.com/simon-weber/Unofficial-Google-Music-API

"""

import sys
import logging
import random
from operator import itemgetter
from gmusicapi import Mobileclient
from gmusicapi.exceptions import CallFailure
from requests.structures import CaseInsensitiveDict
#import pprint

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
    del exception_type # unused
    del traceback # unused
    print_err("[Google Play Music] %s" % (exception))

sys.excepthook = exception_handler

class TizEnumeration(set):
    """A simple enumeration class.

    """
    def __getattr__(self, name):
        if name in self:
            return name
        raise AttributeError

class tizgmusicproxy(object):
    """A class for logging into a Google Play Music account and retrieving song
    URLs.

    """

    all_songs_album_title = "All Songs"
    thumbs_up_playlist_name = "Thumbs Up"

    def __init__(self, email, password, device_id):
        self.__api = Mobileclient()
        self.logged_in = False
        self.__email = email
        self.__device_id = device_id
        self.queue = list()
        self.queue_index = -1
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_song = None

        attempts = 0
        while not self.logged_in and attempts < 3:
            self.logged_in = self.__api.login(email, password, device_id)
            attempts += 1

        self.library = CaseInsensitiveDict()
        self.song_map = CaseInsensitiveDict()
        self.playlists = CaseInsensitiveDict()
        self.stations = CaseInsensitiveDict()

    def logout(self):
        """ Reset the session to an unauthenticated, default state.

        """
        self.__api.logout()

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
            title = self.now_playing_song['title']
            artist = self.now_playing_song['artist']
            logging.info("Now playing {0} by {1}".format(title.encode("utf-8"),
                                                   artist.encode("utf-8")))
            return artist.encode("utf-8"), title.encode("utf-8")
        else:
            return '', ''

    def current_song_album_and_duration(self):
        """ Retrieve the current track's album and duration.

        """
        logging.info("current_song_album_and_duration")
        song = self.now_playing_song
        if song is not None:
            album = self.now_playing_song['album']
            duration = self.now_playing_song['durationMillis']
            logging.info("album {0} duration {1}".format(album.encode("utf-8"),
                                                   duration.encode("utf-8")))
            return album.encode("utf-8"), int(duration)
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

        No All Access search is performed.

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
            count = 0
            for album in artist:
                for song in artist[album]:
                    self.queue.append(song)
                    count += 1
            logging.info("Added {0} tracks by {1} to queue".format(count, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Artist not found : {0}".format(arg))

    def enqueue_album(self, arg):
        """ Search the user's library for albums with a given name and adds
        them to the playback queue.

        No All Access search is performed.

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
                              .format(artist_album.encode("utf-8")))
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
            count = 0
            for song in self.library[artist][album]:
                self.queue.append(song)
                count += 1
                logging.info("Added {0} tracks from {1} by " \
                             "{2} to queue" \
                             .format(count, album.encode("utf-8"), \
                                     artist.encode("utf-8")))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))

    def enqueue_playlist(self, arg):
        """Search the user's library for playlists with a given name and adds the
        tracks of the first match to the playback queue.

        An All Access search is performed.

        """
        try:
            self.__update_local_library()
            self.__update_playlists()
            self.__update_playlists_all_access()
            playlist = None
            for name, plist in self.playlists.items():
                print_nfo("[Google Play Music] [Playlist] '{0}'." \
                          .format(name.encode("utf-8")))
            if not arg in self.playlists.keys():
                for name, plist in self.playlists.iteritems():
                    if arg.lower() in name.lower():
                        playlist = plist
                        print_wrn("[Google Play Music] '{0}' not found. " \
                                  "Playing '{1}' instead." \
                                  .format(arg, name))
                        break
                if not playlist:
                    raise KeyError("Playlist not found : {0}".format(arg))
            else:
                playlist = self.playlists[arg]
            count = 0
            for song in playlist:
                self.queue.append(song)
                count += 1
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(count, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Playlist not found : {0}".format(arg))

    def enqueue_station_all_access(self, arg):
        """Search the user's library for a station with a given name
        and adds its tracks to the playback queue.

        An All Access search is performed.

        """
        print_msg("[Google Play Music] [Retrieving stations] : '{0}'. " \
                  "This could take some time.".format(self.__email))

        try:
            self.__update_stations_all_access()
            for name, st_id in self.stations.iteritems():
                print_nfo("[Google Play Music] [Station] '{0}'." \
                          .format(name.encode("utf-8")))
            station_id = None
            if not arg in self.stations.keys():
                for name, st_id in self.stations.iteritems():
                    if arg.lower() in name.lower():
                        station_id = st_id
                        print_wrn("[Google Play Music] '{0}' not found. " \
                                  "Playing '{1}' instead." \
                                  .format(arg, name.encode("utf-8")))
                        break
                if not station_id:
                    raise KeyError("Station not found : {0}".format(arg))
            else:
                station_id = self.stations[arg]
            num_tracks = 200
            try:
                tracks = self.__api.get_station_tracks(station_id, num_tracks)
            except KeyError:
                raise RuntimeError("Operation requires an "
                                   "All Access subscription.")
            count = 0
            for track in tracks:
                if not u'id' in track.keys():
                    track[u'id'] = track['nid']
                self.queue.append(track)
                count += 1
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(count, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Station not found : {0}".format(arg))

    def enqueue_genre_all_access(self, arg):
        """Search All Access for a genre with a given name and adds its
        tracks to the playback queue.

        An All Access search is performed.

        """
        print_msg("[Google Play Music] [Retrieving genres] : '{0}'. " \
                  "This could take some time.".format(self.__email))

        try:
            all_genres = list()
            root_genres = self.__api.get_genres()
            all_genres += root_genres
            count = 0
            for root_genre in root_genres:
                all_genres += self.__api.get_genres(root_genre['id'])
            for genre in all_genres:
                print_nfo("[Google Play Music] [Genre] '{0}'." \
                          .format(genre['name'].encode("utf-8")))
            genre = dict()
            if not arg in all_genres:
                genre = next((g for g in all_genres \
                              if arg.lower() in g['name'].lower()), None)
                if genre:
                    print_wrn("[Google Play Music] '{0}' not found. " \
                              "Playing '{1}' instead." \
                              .format(arg, genre['name'].encode("utf-8")))
                else:
                    raise KeyError("Genre not found : {0}".format(arg))
            genre_name = genre['name']
            genre_id = genre['id']
            station_id = self.__api.create_station(genre_name, \
                                                   None, None, None, genre_id)
            num_tracks = 200
            tracks = self.__api.get_station_tracks(station_id, num_tracks)
            count = 0
            for track in tracks:
                if not u'id' in track.keys():
                    track[u'id'] = track['nid']
                self.queue.append(track)
                count += 1
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(count, genre_name))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Genre not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an All Access subscription.")

    def enqueue_artist_all_access(self, arg):
        """Search All Access for an artist and adds the artist's 50 top tracks
        to the playback queue.

        An All Access search is performed.

        """
        try:
            artist_hits = self.__api.search_all_access(arg)['artist_hits']
            artist = next((hit for hit in artist_hits \
                           if 'best_result' in hit.keys()), None)
            if not artist:
                artist = artist_hits[0]
                print_wrn("[Google Play Music] '{0}' not found. " \
                          "Playing '{1}' instead." \
                          .format(arg, artist['artist']['name']))
            include_albums = False
            max_top_tracks = 50
            max_rel_artist = 0
            artist_tracks = self.__api.get_artist_info \
                            (artist['artist']['artistId'],
                             include_albums, max_top_tracks,
                             max_rel_artist)['topTracks']
            count = 0
            for track in artist_tracks:
                if not u'id' in track.keys():
                    track[u'id'] = track['nid']
                self.queue.append(track)
                count += 1
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(count, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Artist not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an All Access subscription.")

    def enqueue_album_all_access(self, arg):
        """Search All Access for an album and adds its tracks to the
        playback queue.

        An All Access search is performed.

        """
        try:
            album_hits = self.__api.search_all_access(arg)['album_hits']
            album = next((hit for hit in album_hits \
                          if 'best_result' in hit.keys()), None)
            if not album:
                album = album_hits[0]
                print_wrn("[Google Play Music] '{0}' not found. " \
                          "Playing '{1}' instead." \
                          .format(arg, album['album']['name']))
            album_tracks = self.__api.get_album_info\
                           (album['album']['albumId'])['tracks']
            count = 0
            for track in album_tracks:
                if not u'id' in track.keys():
                    track[u'id'] = track['nid']
                self.queue.append(track)
                count += 1
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(count, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an All Access subscription.")

    def enqueue_tracks_all_access(self, arg):
        """ Search All Access for a track name and adds all the matching tracks
        to the playback queue.

        An All Access search is performed.

        """
        print_msg("[Google Play Music] [Retrieving library] : '{0}'. " \
                  "This could take some time.".format(self.__email))

        try:
            track_hits = self.__api.search_all_access(arg)['song_hits']
            count = 0
            for item in track_hits:
                track = item['track']
                if not u'id' in track.keys():
                    track[u'id'] = track['nid']
                self.queue.append(track)
                count += 1
            logging.info("Added {0} tracks from {1} to queue" \
                         .format(count, arg))
            self.__update_play_queue_order()
        except KeyError:
            raise KeyError("Playlist not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an All Access subscription.")

    def enqueue_promoted_tracks_all_access(self):
        """ Retrieve the url of the next track in the playback queue.

        """
        try:
            tracks = self.__api.get_promoted_songs()
            count = 0
            for track in tracks:
                store_track = self.__api.get_track_info(track['storeId'])
                if not u'id' in store_track.keys():
                    store_track[u'id'] = store_track['nid']
                self.queue.append(store_track)
                count += 1
            if count == 0:
                print_wrn("[Google Play Music] Operation requires " \
                          "an All Access subscription.")
            logging.info("Added {0} All Access promoted tracks to queue" \
                         .format(count))
            self.__update_play_queue_order()
        except CallFailure:
            raise RuntimeError("Operation requires an All Access subscription.")

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
        if len(self.queue):
            if not len(self.play_queue_order):
                # Create a sequential play order, if empty
                self.play_queue_order = range(len(self.queue))
            if self.current_play_mode == self.play_modes.SHUFFLE:
                random.shuffle(self.play_queue_order)

    def __retrieve_track_url(self, song):
        """ Retrieve a song url

        """
        logging.info("__retrieve_track_url : {0}".format(song['id']))
        song_url = self.__api.get_stream_url(song['id'], self.__device_id)
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
                  "This could take some time.".format(self.__email))

        songs = self.__api.get_all_songs()
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
            logging.info("Artist : {0}".format(artist.encode("utf-8")))
            for album in self.library[artist].keys():
                logging.info("   Album : {0}".format(album.encode("utf-8")))
                if album == self.all_songs_album_title:
                    sorted_album = sorted(self.library[artist][album],
                                          key=lambda k: k['title'])
                else:
                    sorted_album = sorted(self.library[artist][album],
                                          key=lambda k: k.get('trackNumber',
                                                              0))
                self.library[artist][album] = sorted_album

    def __update_stations_all_access(self):
        """ Retrieve stations (All Access)

        """
        self.stations.clear()
        stations = self.__api.get_all_stations()
        self.stations[u"I'm Feeling Lucky"] = 'IFL'
        for station in stations:
            station_name = station['name']
            logging.info("station name : {0}" \
                         .format(station_name.encode("utf-8")))
            self.stations[station_name] = station['id']

    def __update_playlists(self):
        """ Retrieve the user's playlists

        """
        plists = self.__api.get_all_user_playlist_contents()
        for plist in plists:
            plist_name = plist['name']
            logging.info("playlist name : {0}" \
                         .format(plist_name.encode("utf-8")))
            tracks = plist['tracks']
            tracks.sort(key=itemgetter('creationTimestamp'))
            self.playlists[plist_name] = list()
            for track in tracks:
                try:
                    song = self.song_map[track['trackId']]
                    self.playlists[plist_name].append(song)
                except IndexError:
                    pass

    def __update_playlists_all_access(self):
        """ Retrieve shared playlists (All Access)

        """
        plists_subscribed_to = [p for p in self.__api.get_all_playlists() \
                                if p.get('type') == 'SHARED']
        for plist in plists_subscribed_to:
            share_tok = plist['shareToken']
            playlist_items = self.__api.get_shared_playlist_contents(share_tok)
            plist_name = plist['name']
            logging.info("shared playlist name : {0}" \
                         .format(plist_name.encode("utf-8")))
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

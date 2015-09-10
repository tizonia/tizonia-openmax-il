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

"""Simple Google Music proxy class.

Access a user's Google Music account to retrieve song URLs to be used for
streaming. With ideas from Dan Nixon's command-line client, which is in turn
based on Simon Weber's 'Unofficial Google Music API' Python module. For more
information:

- https://github.com/DanNixon/PlayMusicCL
- https://github.com/simon-weber/Unofficial-Google-Music-API

"""

import thread
import random
import sys
import os
import logging
import unicodedata as ud
from gmusicapi import Mobileclient
from requests.structures import CaseInsensitiveDict

logging.captureWarnings(True)
logging.getLogger().addHandler(logging.NullHandler())
logging.getLogger().setLevel(logging.INFO)

class tizgmusicproxy(object):
    """A class for accessing a Google Music account to retrieve song URLs.
    """

    all_songs_album_title = "All Songs"
    thumbs_up_playlist_name = "Thumbs Up"

    def __init__(self, email, password, device_id):
        self.__api = Mobileclient()
        self.logged_in = False
        self.__device_id = device_id
        self.queue = list()
        self.queue_index = -1
        self.play_mode = 0
        self.now_playing_song = None

        attempts = 0
        while not self.logged_in and attempts < 3:
            self.logged_in = self.__api.login(email, password, device_id)
            attempts += 1

        self.playlists = CaseInsensitiveDict()
        self.library = CaseInsensitiveDict()

    def logout(self):
        self.__api.logout()

    def update_local_lib(self):
        songs = self.__api.get_all_songs()
        self.playlists[self.thumbs_up_playlist_name] = list()

        # Get main library
        song_map = dict()
        for song in songs:
            if "rating" in song and song["rating"] == "5":
                self.playlists[self.thumbs_up_playlist_name].append(song)

            song_id = song["id"]
            song_artist = song["artist"]
            song_album = song["album"]

            song_map[song_id] = song

            if song_artist == "":
                song_artist = "Unknown Artist"

            if song_album == "":
                song_album = "Unknown Album"

            if not (song_artist in self.library):
                self.library[song_artist] = dict()
                self.library[song_artist][self.all_songs_album_title] = list()

            if not (song_album in self.library[song_artist]):
                self.library[song_artist][song_album] = list()

            self.library[song_artist][song_album].append(song)
            self.library[song_artist][self.all_songs_album_title].append(song)

        # Sort albums by track number
        for artist in self.library.keys():
            logging.info ("Artist : {0}".format(artist.encode("utf-8")))
            for album in self.library[artist].keys():
                logging.info ("   Album : {0}".format(album.encode("utf-8")))
                if album == self.all_songs_album_title:
                    sorted_album = sorted(self.library[artist][album],
                                          key=lambda k: k['title'])
                else:
                    sorted_album = sorted(self.library[artist][album],
                                          key=lambda k: k.get('trackNumber',
                                                              0))
                self.library[artist][album] = sorted_album

        # Get all user playlists
        plists = self.__api.get_all_user_playlist_contents()
        for plist in plists:
            plist_name = plist["name"]
            logging.info ("playlist name : {0}".format(plist_name.encode("utf-8")))
            self.playlists[plist_name] = list()
            for track in plist["tracks"]:
                try:
                    song = song_map[track["trackId"]]
                    self.playlists[plist_name].append(song)
                except IndexError:
                    pass

        # Get shared playlists (All Access)
        plists_subscribed_to = [p for p in self.__api.get_all_playlists() if p.get('type') == 'SHARED']
        for plist in plists_subscribed_to:
            share_tok = plist['shareToken']
            playlist_items = self.__api.get_shared_playlist_contents(share_tok)
            plist_name = plist["name"]
            logging.info ("shared playlist name : {0}".format(plist_name.encode("utf-8")))
            self.playlists[plist_name] = list()
            for item in playlist_items:
                try:
                    song = item['track']
                    song['id'] = item['trackId']
                    self.playlists[plist_name].append(song)
                except IndexError:
                    pass

    def current_song_title_and_artist(self):
        logging.info ("current_song_title_and_artist")
        song         = self.now_playing_song
        if song is not None:
            title    = self.now_playing_song["title"]
            artist   = self.now_playing_song["artist"]
            logging.info ("Now playing {0} by {1}".format(title.encode("utf-8"),
                                                   artist.encode("utf-8")))
            return artist.encode("utf-8"), title.encode("utf-8")
        else:
            return '', ''

    def current_song_album_and_duration(self):
        logging.info ("current_song_album_and_duration")
        song = self.now_playing_song
        if song is not None:
            album = self.now_playing_song["album"]
            duration = self.now_playing_song["durationMillis"]
            logging.info ("album {0} duration {1}".format(album.encode("utf-8"),
                                                   duration.encode("utf-8")))
            return album.encode("utf-8"), int(duration)
        else:
            return '', 0

    def current_song_track_number_and_total_tracks(self):
        logging.info ("current_song_track_number_and_total_tracks")
        song = self.now_playing_song
        if song is not None:
            try:
                track = self.now_playing_song["trackNumber"]
                total = self.now_playing_song["totalTrackCount"]
                logging.info ("track number {0} total tracks {1}".format(track, total))
                return track, total
            except KeyError:
                logging.info ("trackNumber or totalTrackCount : not found")
                return 0, 0
        else:
            logging.info ("current_song_track_number_and_total_tracks : not found")
            return 0, 0

    def clear_queue(self):
        self.queue = list()
        self.queue_index = -1

    def enqueue_artist(self, arg):
        try:
            artist = self.library[arg]
            count = 0
            for album in artist:
                for song in artist[album]:
                    self.queue.append(song)
                    count += 1
            logging.info ("Added {0} tracks by {1} to queue".format(count, arg))
        except KeyError:
            logging.info ("Cannot find {0}".format(arg))
            raise

    def enqueue_album(self, arg):
        try:
            for artist in self.library:
                for album in self.library[artist]:
                    logging.info ("enqueue album : {0} | {1}".format(
                        artist.encode("utf-8"),
                        album.encode("utf-8")))
                    if album.lower() == arg.lower():
                        count = 0
                        for song in self.library[artist][album]:
                            self.queue.append(song)
                            count += 1
                        logging.info ("Added {0} tracks from {1} by "
                        "{2} to queue".format(count, album.encode("utf-8"),
                                              artist.encode("utf-8")))
        except KeyError:
            logging.info ("Cannot find {0}".format(arg))
            raise

    def enqueue_playlist(self, arg):
        try:
            for playlist in self.playlists:
                logging.info ("playlist {0}".format(playlist.encode("utf-8")))
            playlist = self.playlists[arg]
            count = 0
            for song in playlist:
                self.queue.append(song)
                count += 1
            logging.info ("Added {0} tracks from {1} to queue".format(count, arg))
        except KeyError:
            logging.info ("Cannot find {0}".format(arg))
            raise

    def next_url(self):
        logging.info ("next_url")
        if len(self.queue):
            self.queue_index += 1
            if (self.queue_index < len(self.queue)) \
               and (self.queue_index >= 0):
                next_song = self.queue[self.queue_index]
                return self.__get_song_url(next_song)
            else:
                self.queue_index = -1
                return self.next_url()
        else:
            return ''

    def prev_url(self):
        if len(self.queue):
            self.queue_index -= 1
            if (self.queue_index < len(self.queue)) \
               and (self.queue_index >= 0):
                prev_song = self.queue[self.queue_index]
                return self.__get_song_url(prev_song)
            else:
                self.queue_index = len(self.queue)
                return self.prev_url()
        else:
            return ''

    def __get_song_url(self, song):
        song_url = self.__api.get_stream_url(song["id"], self.__device_id)
        try:
            self.now_playing_song = song
            return song_url
        except AttributeError:
            logging.info ("Could not retrieve song url!")
            raise

if __name__ == "__main__":
    tizgmusicproxy()

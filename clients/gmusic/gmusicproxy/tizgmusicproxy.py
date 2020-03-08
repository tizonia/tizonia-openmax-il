# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
import configparser
from datetime import datetime
from operator import itemgetter
from gmusicapi import Mobileclient
from gmusicapi.exceptions import CallFailure
from requests.structures import CaseInsensitiveDict
from fuzzywuzzy import process
from fuzzywuzzy import fuzz

# For use during debugging
# from pprint import pprint

FORMAT = (
    "[%(asctime)s] [%(levelname)5s] [%(thread)d] "
    "[%(module)s:%(funcName)s:%(lineno)d] - %(message)s"
)

logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get("TIZONIA_GMUSICPROXY_DEBUG"):
    logging.basicConfig(format=FORMAT)
    from traceback import print_exception
else:
    logging.getLogger().addHandler(logging.NullHandler())

MAX_TRACKS_NONE = None
MAX_TRACKS = 100


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
    print_err("[Google Play Music] (%s) : %s" % (exception_type.__name__, exception))

    if os.environ.get("TIZONIA_GMUSICPROXY_DEBUG"):
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


class tizgmusicproxy(object):
    """A class for logging into a Google Play Music account and retrieving song
    URLs.

    """

    all_songs_album_title = "All Songs"
    thumbs_up_playlist_name = "Thumbs Up"

    # pylint: disable=too-many-instance-attributes,too-many-public-methods
    def __init__(self, email, password, device_id):
        self.gmusic = Mobileclient()
        self.email = email
        self.device_id = device_id
        self.logged_in = False
        self.queue = list()
        self.queue_index = -1
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_song = None

        userdir = os.path.expanduser("~")
        tizconfig = os.path.join(userdir, ".config/tizonia/." + email + ".auth_token")
        auth_token = ""
        if os.path.isfile(tizconfig):
            with open(tizconfig, "rb") as f:
                auth_token = pickle.load(f)
                if auth_token:
                    # 'Keep track of the auth token' workaround. See:
                    # https://github.com/diraimondo/gmusicproxy/issues/34#issuecomment-147359198
                    print_msg(
                        "[Google Play Music] [Authenticating] : "
                        "'with cached auth token'"
                    )
                    self.gmusic.android_id = device_id
                    self.gmusic.session._authtoken = auth_token
                    self.gmusic.session.is_authenticated = True
                    try:
                        self.gmusic.get_registered_devices()
                    except CallFailure:
                        # The token has expired. Reset the client object
                        print_wrn(
                            "[Google Play Music] [Authenticating] : "
                            "'auth token expired'"
                        )
                        self.gmusic = Mobileclient()
                        auth_token = ""

        if not auth_token:
            attempts = 0
            print_nfo(
                "[Google Play Music] [Authenticating] : " "'with user credentials'"
            )
            while not self.logged_in and attempts < 3:
                self.logged_in = self.gmusic.login(email, password, device_id)
                attempts += 1

            with open(tizconfig, "ab+") as f:
                f.truncate()
                pickle.dump(self.gmusic.session._authtoken, f)

        self.library = CaseInsensitiveDict()
        self.song_map = CaseInsensitiveDict()
        self.playlists = CaseInsensitiveDict()
        self.stations = CaseInsensitiveDict()

    def logout(self):
        """ Reset the session to an unauthenticated, default state.

        """
        self.gmusic.logout()

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: curren tvalid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)
        self._update_play_queue_order(print_queue=False)

    def current_song_title_and_artist(self):
        """ Retrieve the current track's title and artist name.

        """
        logging.info("current_song_title_and_artist")
        song = self.now_playing_song
        if song:
            title = to_ascii(song.get("title")) if song.get("title") else ""
            artist = to_ascii(song.get("artist")) if song.get("artist") else ""
            if "" == artist:
                # try author instead
                artist = to_ascii(song.get("author")) if song.get("author") else ""
            if "" == artist:
                artist = "Unknown"
            logging.info("Now playing %s by %s", title, artist)
            return artist, title
        else:
            return "", ""

    def current_song_album_and_duration(self):
        """ Retrieve the current track's album and duration.

        """
        logging.info("current_song_album_and_duration")
        song = self.now_playing_song
        if song:
            album = to_ascii(song.get("album")) if song.get("album") else ""
            if "" == album:
                # try seriesTitle instead
                album = (
                    to_ascii(song.get("seriesTitle")) if song.get("seriesTitle") else ""
                )
            duration = (
                to_ascii(song.get("durationMillis"))
                if song.get("durationMillis")
                else ""
            )
            if "" == album:
                album = "Unknown"
            logging.info("album %s duration %s", album, duration)
            return album, int(duration)
        else:
            return "", 0

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
                track = song["trackNumber"]
                total = song["totalTrackCount"]
                logging.info("track number %s total tracks %s", track, total)
            except KeyError:
                logging.info("trackNumber or totalTrackCount : not found")
        else:
            logging.info("current_song_track_number_" "and_total_tracks : not found")
        return track, total

    def current_song_year(self):
        """ Return the current track's year of publication.

        """
        logging.info("current_song_year")
        song = self.now_playing_song
        year = "0"
        if song:
            year = str(song["year"]) if song.get("year") else "0"
            if "0" == year:
                # try publicationTimestampMillis
                year = (
                    song["publicationTimestampMillis"]
                    if song.get("publicationTimestampMillis")
                    else "0"
                )
                if "0" != year:
                    year = datetime.fromtimestamp(int(year) / 1000.0).strftime(
                        "%Y-%m-%d"
                    )
        logging.info("track year %s", year)
        return year

    def current_song_genre(self):
        """ Return the current track's genre.

        """
        logging.info("current_song_genre")
        song = self.now_playing_song
        if song:
            genre = to_ascii(song.get("genre")) if song.get("genre") else ""
            if "" == genre and song.get("seriesId"):
                genre = "podcast"
            logging.info("genre %s", genre)
            return genre
        else:
            return ""

    def current_song_album_art(self):
        """ Return the current track's album art image.

        """
        logging.info("current_song_art")
        song = self.now_playing_song
        url = ""
        if song and song.get("albumArtRef"):
            artref = song.get("albumArtRef")
            if artref and len(artref) > 0:
                url = to_ascii(artref[0].get("url"))
                logging.info("url %s", url)
                return url
        if song and song.get("art"):
            art = song.get("art")
            url = art[0].get("url")
        return url

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1

    def enqueue_library(self):
        """ Add the user library to the playback queue.

        """
        try:
            songs = self.gmusic.get_all_songs()
            self._enqueue_tracks(songs)
            self._update_play_queue_order(print_queue=False)

        except KeyError:
            raise KeyError("Library not found")

    def enqueue_tracks(self, arg):
        """ Search the user's library for tracks and add
        them to the playback queue.

        :param arg: a track search term
        """
        try:
            songs = self.gmusic.get_all_songs()

            track_hits = list()
            for song in songs:
                song_title = song["title"]
                if fuzz.partial_ratio(arg, song_title) > 60:
                    track_hits.append(song)

            if not len(track_hits):
                print_adv(
                    "[Google Play Music] '{0}' not found. "
                    "Feeling lucky?.".format(to_ascii(arg))
                )
                random.seed()
                max_tracks = min(len(songs), MAX_TRACKS)
                track_hits = random.sample(songs, max_tracks)
                for hit in track_hits:
                    song_title = hit["title"]
                    song_artist = hit["artist"]

            if not len(track_hits):
                raise KeyError

            self._enqueue_tracks(track_hits)
            self._update_play_queue_order()

        except KeyError:
            raise KeyError("Track not found : {0}".format(arg))

    def enqueue_artist(self, arg):
        """ Search the user's library for tracks from the given artist and add
        them to the playback queue.

        :param arg: an artist
        """
        try:
            self._update_local_library()
            artist = None
            artist_dict = None
            if arg not in list(self.library.keys()):
                artist_dicts = dict()
                artist_names = list()
                for name, art in self.library.items():
                    if fuzz.ratio(arg, name) > 50:
                        artist_names.append(name)
                        artist_dicts[name] = art

                if len(artist_names) > 1:
                    artist = process.extractOne(arg, artist_names)[0]
                    artist_dict = artist_dicts[artist]
                elif len(artist_names) == 1:
                    artist = artist_names[0]
                    artist_dict = artist_dicts[artist]

                if artist:
                    if arg.lower() != name.lower():
                        print_wrn(
                            "[Google Play Music] '{0}' not found. "
                            "Playing '{1}' instead.".format(
                                to_ascii(arg), to_ascii(artist)
                            )
                        )

                if not artist:
                    # Play some random artist from the library
                    random.seed()
                    artist = random.choice(list(self.library.keys()))
                    artist_dict = self.library[artist]
                    print_adv(
                        "[Google Play Music] '{0}' not found. "
                        "Feeling lucky?.".format(to_ascii(arg))
                    )
            else:
                artist = arg
                artist_dict = self.library[arg]
            for album in artist_dict:
                self._enqueue_tracks(artist_dict[album])
            print_wrn("[Google Play Music] Playing '{0}'.".format(to_ascii(artist)))

            self._update_play_queue_order()

        except KeyError:
            raise KeyError("Artist not found : {0}".format(arg))

    def enqueue_album(self, arg):
        """ Search the user's library for albums with a given name and add
        them to the playback queue.

        """
        try:
            self._update_local_library()
            album = None
            artist = None
            choice_albums = list()
            choice_artists = dict()
            for library_artist in self.library:
                for artist_album in self.library[library_artist]:
                    print_nfo(
                        "[Google Play Music] [Album] '{0} ({1})'.".format(
                            to_ascii(artist_album), to_ascii(library_artist)
                        )
                    )
                    choice_albums.append(artist_album)
                    choice_artists[artist_album] = library_artist

            album = process.extractOne(arg, choice_albums)[0]
            artist = choice_artists[album]

            if not album:
                # Play some random album from the library
                random.seed()
                artist = random.choice(list(self.library.keys()))
                album = random.choice(list(self.library[artist].keys()))
                print_adv(
                    "[Google Play Music] '{0}' not found. "
                    "Feeling lucky?.".format(to_ascii(arg))
                )

            if not album:
                raise KeyError("Album not found : {0}".format(arg))

            self._enqueue_tracks(self.library[artist][album])
            print_wrn(
                "[Google Play Music] Playing '{0} ({1})'.".format(
                    to_ascii(album), to_ascii(artist)
                )
            )

            self._update_play_queue_order()

        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))

    def enqueue_playlist(self, arg):
        """Search the user's library for playlists with a given name
        and add the tracks of the first match to the playback queue.

        Requires Unlimited subscription.

        """
        try:
            self._update_local_library()
            self._update_playlists_unlimited()
            self._update_playlists()
            playlist = None
            playlist_name = None
            for name, plist in list(self.playlists.items()):
                print_nfo(
                    "[Google Play Music] [Playlist] '{0}' ({1} tracks).".format(
                        to_ascii(name), len(plist)
                    )
                )
            if arg not in list(self.playlists.keys()):
                playlist_dict = dict()
                playlist_names = list()
                for name, plist in self.playlists.items():
                    if fuzz.ratio(arg, name) > 50:
                        playlist_dict[name] = plist
                        playlist_names.append(name)

                if len(playlist_names) > 1:
                    playlist_name = process.extractOne(arg, playlist_names)[0]
                    playlist = playlist_dict[playlist_name]
                elif len(playlist_names) == 1:
                    playlist_name = playlist_names[0]
                    playlist = playlist_dict[playlist_name]

                if playlist_name:
                    if arg.lower() != playlist_name.lower():
                        print_wrn(
                            "[Google Play Music] '{0}' not found. "
                            "Playing '{1}' instead.".format(
                                to_ascii(arg), to_ascii(playlist_name)
                            )
                        )

            else:
                playlist_name = arg
                playlist = self.playlists[arg]

            if not playlist and len(list(self.playlists.keys())) > 0:
                print_adv(
                    "[Google Play Music] '{0}' not found (or is empty). "
                    "Feeling lucky?.".format(to_ascii(arg))
                )
                random.seed()
                x = 0
                playlists_copy = self.playlists.copy()
                while (
                    (not playlist or not len(playlist))
                    and x < 3
                    and len(playlists_copy)
                ):
                    x += 1
                    # Play some random playlist from the library
                    playlist_name = random.choice(list(playlists_copy.keys()))
                    playlist = self.playlists[playlist_name]
                    if not playlist or not len(playlist):
                        del playlists_copy[playlist_name]
                        print_wrn(
                            "[Google Play Music] '{0}' is empty. ".format(
                                to_ascii(playlist_name)
                            )
                        )

            if not len(playlist):
                raise KeyError

            self._enqueue_tracks(playlist)
            print_wrn(
                "[Google Play Music] Playing '{0}'.".format(to_ascii(playlist_name))
            )

            self._update_play_queue_order()

        except KeyError:
            raise KeyError("Playlist not found or is empty : {0}".format(arg))

    def enqueue_podcast(self, arg):
        """Search Google Play Music for a podcast series and add its tracks to the
        playback queue ().

        Requires Unlimited subscription.

        """
        print_msg(
            "[Google Play Music] [Retrieving podcasts] : '{0}'. ".format(self.email)
        )

        try:

            self._enqueue_podcast(arg)

            if not len(self.queue):
                raise KeyError

            logging.info("Added %d episodes from '%s' to queue", len(self.queue), arg)
            self._update_play_queue_order(print_queue=False)

        except KeyError:
            raise KeyError("Podcast not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_station(self, arg):
        """Search for a free station using a given track, album or artist name and add
        its tracks to the playback queue.

        Available in the free tier.

        """
        try:
            # First search for a station id
            max_results = MAX_TRACKS
            station_hits = self.gmusic.search(arg, max_results)["station_hits"]
            station_name = ""
            station_seed = None
            station_id = ""
            station_tracks = dict()
            if station_hits and len(station_hits):
                station_seeds = dict()
                station_names = list()
                for hit in station_hits:
                    station = hit["station"]
                    station_name = station["name"]
                    print_nfo(
                        "[Google Play Music] [Station] '{0}'.".format(station_name)
                    )
                    if fuzz.partial_ratio(arg, station_name) > 70:
                        station_seeds[station_name] = station["seed"]
                        station_names.append(station_name)

                if len(station_names) > 1:
                    station_name = process.extractOne(arg, station_names)[0]
                    station_seed = station_seeds[station_name]
                elif len(station_names) == 1:
                    station_name = station_names[0]
                    station_seed = station_seeds[station_name]

                if station_seed:
                    if station_seed["seedType"] == "2":
                        station_id = self.gmusic.create_station(
                            station_name, track_id=station_seed["trackId"]
                        )
                    if station_seed["seedType"] == "3":
                        station_id = self.gmusic.create_station(
                            station_name, artist_id=station_seed["artistId"]
                        )
                    if station_seed["seedType"] == "4":
                        station_id = self.gmusic.create_station(
                            station_name, album_id=station_seed["albumId"]
                        )
                    if station_seed["seedType"] == "9":
                        station_id = self.gmusic.create_station(
                            station_name,
                            curated_station_id=station_seed["curatedStationId"],
                        )
                if station_id:
                    station_info = self.gmusic.get_station_info(station_id)
                    session_token = (
                        station_info["sessionToken"]
                        if station_info.get("sessionToken")
                        else None
                    )
                    station_tracks = station_info["tracks"]

                if not station_tracks:
                    raise KeyError

                print_wrn("[Google Play Music] Playing '{0}'.".format(station_name))

                for track in station_tracks:
                    track["sessionToken"] = session_token

                self._enqueue_tracks(station_tracks)
                self._update_play_queue_order()

            if not len(self.queue):
                raise KeyError

        except KeyError:
            raise KeyError(
                "Station not found : '{0}' "
                "(NOTE: Free stations are currently available "
                "only in the U.S., Canada, and India).".format(arg)
            )

    def enqueue_station_unlimited(self, arg):
        """Search the user's library for a station with a given name
        and add its tracks to the playback queue.

        Requires Unlimited subscription.

        """
        try:
            # If no suitable station is found in the user's library, then
            # search google play unlimited for a potential match.
            self._enqueue_station_unlimited(arg)

            if not len(self.queue):
                raise KeyError

        except KeyError:
            raise KeyError("Station not found : {0}".format(arg))

    def enqueue_genre_unlimited(self, arg):
        """Search Unlimited for a genre with a given name and add its
        tracks to the playback queue.

        Requires Unlimited subscription.

        """
        print_msg(
            "[Google Play Music] [Retrieving genres] : '{0}'. ".format(self.email)
        )

        try:
            all_genres = list()
            root_genres = self.gmusic.get_genres()
            second_tier_genres = list()
            for root_genre in root_genres:
                second_tier_genres += self.gmusic.get_genres(root_genre["id"])
            all_genres += root_genres
            all_genres += second_tier_genres
            choices = dict()
            choice_names = list()
            for g in all_genres:
                print_nfo(
                    "[Google Play Music] [Genre] '{0}'.".format(to_ascii(g["name"]))
                )
                choices[g["name"]] = g
                choice_names.append(g["name"])

            choice_name = process.extractOne(arg, choice_names)[0]
            genre = choices[choice_name]

            tracks_added = 0
            while not tracks_added:
                if not genre and len(all_genres):
                    # Play some random genre from the search results
                    random.seed()
                    genre = random.choice(all_genres)
                    print_adv(
                        "[Google Play Music] '{0}' not found. "
                        "Feeling lucky?.".format(to_ascii(arg))
                    )

                genre_name = genre["name"]
                genre_id = genre["id"]
                station_id = self.gmusic.create_station(
                    genre_name, None, None, None, genre_id
                )
                num_tracks = MAX_TRACKS
                tracks = self.gmusic.get_station_tracks(station_id, num_tracks)
                tracks_added = self._enqueue_tracks(tracks)
                logging.info(
                    "Added %d tracks from %s to queue", tracks_added, genre_name
                )
                if not tracks_added:
                    print_wrn(
                        "[Google Play Music] '{0}' No tracks found. "
                        "Trying something else.".format(to_ascii(genre_name))
                    )
                    del choices[genre_name]
                    choice_names.remove(genre_name)
                    choice_name = process.extractOne(arg, choice_names)[0]
                    genre = choices[choice_name]

            print_wrn(
                "[Google Play Music] Playing '{0}'.".format(to_ascii(genre["name"]))
            )

            self._update_play_queue_order()

        except KeyError:
            raise KeyError("Genre not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_situation_unlimited(self, arg, additional_keywords):
        """Search Unlimited for a situation (a.k.a. activity) with a given name and add its
        tracks to the playback queue.

        Requires Unlimited subscription.

        """
        print_msg(
            "[Google Play Music] [Retrieving activities] : '{0}'. ".format(self.email)
        )

        try:

            self._enqueue_situation_unlimited(arg, additional_keywords)

            if not len(self.queue):
                raise KeyError

            logging.info("Added %d tracks from %s to queue", len(self.queue), arg)

        except KeyError:
            raise KeyError("Activity not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_artist_unlimited(self, arg):
        """Search Unlimited for an artist and add the artist's top tracks to the
        playback queue.

        Requires Unlimited subscription.

        """
        try:
            artist = self._gmusic_search(arg, "artist")

            include_albums = False
            max_top_tracks = 1000
            max_rel_artist = 0
            artist_tracks = dict()
            if artist:
                artist_dict = self.gmusic.get_artist_info(
                    artist["artist"]["artistId"],
                    include_albums,
                    max_top_tracks,
                    max_rel_artist,
                )
                if artist_dict.get("topTracks"):
                    artist_tracks = artist_dict["topTracks"]
                else:
                    raise RuntimeError(
                        "Artist search returned no tracks : {0}".format(arg)
                    )

            if not artist_tracks:
                raise KeyError

            self._enqueue_tracks(artist_tracks)
            self._update_play_queue_order()

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
            album = self._gmusic_search(arg, "album")
            album_tracks = dict()
            if album:
                album_tracks = self.gmusic.get_album_info(album["album"]["albumId"])[
                    "tracks"
                ]
            if not album_tracks:
                raise KeyError

            print_wrn(
                "[Google Play Music] Playing '{0} ({1})'.".format(
                    (album["album"]["name"]), (album["album"]["artist"])
                )
            )

            self._enqueue_tracks(album_tracks)
            self._update_play_queue_order()

        except KeyError:
            raise KeyError("Album not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_tracks_unlimited(self, arg):
        """ Search Unlimited for a track name and add all the matching tracks
        to the playback queue.

        Requires Unlimited subscription.

        """
        print_msg(
            "[Google Play Music] [Retrieving library] : '{0}'. ".format(self.email)
        )

        try:
            max_results = MAX_TRACKS
            track_hits = self.gmusic.search(arg, max_results)["song_hits"]
            if not len(track_hits):
                # Do another search with an empty string
                track_hits = self.gmusic.search("", max_results)["song_hits"]
                print_adv(
                    "[Google Play Music] '{0}' not found. "
                    "Feeling lucky?.".format(to_ascii(arg))
                )

            tracks = list()
            for hit in track_hits:
                tracks.append(hit["track"])

            self._enqueue_tracks(tracks)
            self._update_play_queue_order()

        except KeyError:
            raise KeyError("Playlist not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_playlist_unlimited(self, arg):
        """Search Unlimited for a playlist name and add all its tracks to the
        playback queue.

        Requires Unlimited subscription.

        """
        print_msg(
            "[Google Play Music] [Retrieving playlists] : '{0}'. ".format(self.email)
        )

        try:
            playlist_tracks = list()

            playlist_hits = self._gmusic_search(arg, "playlist")
            if playlist_hits:
                playlist = playlist_hits["playlist"]
                playlist_contents = self.gmusic.get_shared_playlist_contents(
                    playlist["shareToken"]
                )
            else:
                raise KeyError

            print_wrn(
                "[Google Play Music] Playing '{0}' by '{1}'.".format(
                    playlist["name"],
                    playlist["ownerName"] if playlist.get("ownerName") else "n/a",
                )
            )

            for item in playlist_contents:
                track = item["track"]
                print_nfo(
                    "[Google Play Music] [Track] '{} by {} (Album: {}, {})'.".format(
                        (track["title"]),
                        (track["artist"]),
                        (track["album"]),
                        (track["year"]),
                    )
                )
                playlist_tracks.append(track)

            if not playlist_tracks:
                raise KeyError

            self._enqueue_tracks(playlist_tracks)
            self._update_play_queue_order(print_queue=False)

        except KeyError:
            raise KeyError("Playlist not found : {0}".format(arg))
        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def enqueue_promoted_tracks_unlimited(self):
        """ Retrieve the url of the next track in the playback queue.

        """
        try:
            tracks = self.gmusic.get_promoted_songs()
            count = 0
            for track in tracks:
                store_track = self.gmusic.get_track_info(track["storeId"])
                if "id" not in list(store_track.keys()):
                    store_track["id"] = store_track["storeId"]
                self.queue.append(store_track)
                count += 1
            if count == 0:
                print_wrn(
                    "[Google Play Music] Operation requires "
                    "an Unlimited subscription."
                )

            self._print_play_queue()
            self._update_play_queue_order()

        except CallFailure:
            raise RuntimeError("Operation requires an Unlimited subscription.")

    def next_url(self, count=0):
        """ Retrieve the url of the next track in the playback queue.

        """
        if len(self.queue):
            self.queue_index += 1
            if (self.queue_index < len(self.queue)) and (self.queue_index >= 0):
                next_song = self.queue[self.play_queue_order[self.queue_index]]
                url = self._retrieve_track_url(next_song)
                if url:
                    return url
                else:
                    if count < len(self.queue) and count < sys.getrecursionlimit():
                        logging.info(
                            "Trying item # {0} in the queue!".format(count + 1)
                        )
                        return self.next_url(count + 1)
                    else:
                        raise RuntimeError("Unable to play any songs from the queue.")
            else:
                self.queue_index = -1
                logging.info("Trying item # {0} in the queue!".format(count + 1))
                return self.next_url(count + 1)
        else:
            return ""

    def prev_url(self):
        """ Retrieve the url of the previous track in the playback queue.

        """
        if len(self.queue):
            self.queue_index -= 1
            if (self.queue_index < len(self.queue)) and (self.queue_index >= 0):
                prev_song = self.queue[self.play_queue_order[self.queue_index]]
                return self._retrieve_track_url(prev_song)
            else:
                self.queue_index = len(self.queue)
                return self.prev_url()
        else:
            return ""

    def _update_play_queue_order(self, print_queue=True):
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

            if print_queue:
                self._print_play_queue()

            print_nfo(
                "[Google Play Music] [Tracks in queue] '{0}'.".format(total_tracks)
            )

    def _print_play_queue(self):
        """ Print the play queue in playback order.

        """
        for index in self.play_queue_order:
            track = self.queue[index]
            print_nfo(
                "[Google Play Music] [Track] '{0}' by '{1}'.".format(
                    to_ascii(track["title"]), to_ascii(track["artist"])
                )
            )

    def _retrieve_track_url(self, song):
        """ Retrieve a song url

        """
        try:
            if song.get("episodeId"):
                song_url = self.gmusic.get_podcast_episode_stream_url(
                    song["episodeId"], self.device_id
                )
            elif song.get("wentryid"):
                song_url = self.gmusic.get_station_track_stream_url(
                    song["id"], song["wentryid"], song["sessionToken"], self.device_id
                )
            else:
                song_url = self.gmusic.get_stream_url(song["id"], self.device_id)

            self.now_playing_song = song
            return song_url

        except AttributeError:
            logging.info(
                "AttributeError: [{0}] Could not retrieve the song url!".format(
                    song["title"] if song.get("title") else ""
                )
            )
            raise
        except CallFailure:
            title = song["title"] if song.get("title") else ""
            logging.info(
                "AttributeError: [{0}] Could not retrieve the song url!".format(title)
            )
            print_wrn(
                "[Google Play Music] : [{0}] 'Could not retrieve the song url'".format(
                    title
                )
            )

    def _update_local_library(self):
        """ Retrieve the songs and albums from the user's library

        """
        print_msg(
            "[Google Play Music] [Retrieving library] : '{0}'. ".format(self.email)
        )

        songs = self.gmusic.get_all_songs()
        self.playlists[self.thumbs_up_playlist_name] = list()

        # Retrieve the user's song library
        for song in songs:
            if "rating" in song and song["rating"] == "5":
                self.playlists[self.thumbs_up_playlist_name].append(song)

            song_id = song["id"]
            song_artist = song["artist"]
            song_album = song["album"]

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
        for artist in list(self.library.keys()):
            logging.info("Artist : %s", to_ascii(artist))
            for album in list(self.library[artist].keys()):
                logging.info("   Album : %s", to_ascii(album))
                if album == self.all_songs_album_title:
                    sorted_album = sorted(
                        self.library[artist][album], key=lambda k: k["title"]
                    )
                else:
                    sorted_album = sorted(
                        self.library[artist][album],
                        key=lambda k: k.get("trackNumber", 0),
                    )
                self.library[artist][album] = sorted_album

    def _update_stations_unlimited(self):
        """ Retrieve stations (Unlimited)

        """
        self.stations.clear()
        stations = self.gmusic.get_all_stations()
        self.stations["I'm Feeling Lucky"] = "IFL"
        for station in stations:
            station_name = station["name"]
            logging.info("station name : %s", to_ascii(station_name))
            self.stations[station_name] = station["id"]

    def _enqueue_station_unlimited(self, arg, max_results=MAX_TRACKS, quiet=False):
        """Search for a station and enqueue all of its tracks (Unlimited)

        """
        if not quiet:
            print_msg(
                "[Google Play Music] [Station search in "
                "Google Play Music] : '{0}'. ".format(to_ascii(arg))
            )
        try:
            station_name = arg
            station_id = None
            station = self._gmusic_search(arg, "station", max_results, quiet)
            if station:
                station = station["station"]
                station_name = station["name"]
                seed = station["seed"]
                seed_type = seed["seedType"]
                track_id = seed["trackId"] if seed_type == "2" else None
                artist_id = seed["artistId"] if seed_type == "3" else None
                album_id = seed["albumId"] if seed_type == "4" else None
                genre_id = seed["genreId"] if seed_type == "5" else None
                playlist_token = (
                    seed["playlistShareToken"] if seed_type == "8" else None
                )
                curated_station_id = (
                    seed["curatedStationId"] if seed_type == "9" else None
                )
                num_tracks = max_results
                tracks = list()
                try:
                    station_id = self.gmusic.create_station(
                        station_name,
                        track_id,
                        artist_id,
                        album_id,
                        genre_id,
                        playlist_token,
                        curated_station_id,
                    )
                    tracks = self.gmusic.get_station_tracks(station_id, num_tracks)
                except KeyError:
                    raise RuntimeError(
                        "Operation requires an " "Unlimited subscription."
                    )
                tracks_added = self._enqueue_tracks(tracks)
                if tracks_added:
                    if not quiet:
                        print_wrn(
                            "[Google Play Music] [Station] : '{0}'.".format(
                                station_name
                            )
                        )
                    logging.info(
                        "Added %d tracks from %s to queue", tracks_added, to_ascii(arg)
                    )
                    self._update_play_queue_order()

        except KeyError:
            raise KeyError("Station not found : {0}".format(arg))

    def _enqueue_situation_unlimited(self, arg, additional_keywords):
        """Search for a situation and enqueue all of its tracks (Unlimited)

        """
        print_msg(
            "[Google Play Music] [Activity search in "
            "Google Play Music] : '{0}'. ".format(to_ascii(to_ascii(arg)))
        )
        if additional_keywords:
            print_msg(
                "[Google Play Music] [Activity search in "
                "Google Play Music] : 'Additional keywords : {0}'. ".format(
                    to_ascii(additional_keywords)
                )
            )
        try:
            situation = None
            situation_title = ""
            situation_desc = None
            situation_id = None

            results = self.gmusic.search(arg)
            #             situation_hits = results['situation_hits']

            #             for hit in situation_hits:
            #                 pprint (hit)
            #                 situation = hit['situation']
            #                 situation_title = situation['title'] if situation.get('title') else ''
            #                 situation_desc = situation['description'] if situation.get('description') else 'None'
            #                 if situation_desc:
            #                     print_nfo("[Google Play Music] [{0}] '{1} : {2}'." \
            #                               .format(arg, situation_title,
            #                                       situation_desc))

            # {u'cluster': [{u'category': u'1', u'id': u'search_situation', u'type': u'7'}],
            #  u'situation': {u'description': u'We\u2019ll play you great songs that the whole office can enjoy.',
            #                 u'id': u'Nba3ghyph5zhpclslcakv6ffxwm',
            #                 u'imageUrl': u'http://lh3.googleusercontent.com/rDxm2lzeuxsFLoT0W4cSEMv20aiUjFU6dLTnmGLTtD-Lz0kSrN_cJH1zMGB_rHkmhr6aGTV0kQ',
            #                 u'title': u'Office radio',
            #                 u'wideImageUrl': u'http://lh3.googleusercontent.com/Ma7Uy3T6rMSHADWneS4ybErNlkNu1AV0jAl9gZpkj6Ms52ivGsGA6caCjiIdlDC9DgKi1VWZ2GI=s1080'},
            #  u'type': u'7'}
            # [Google Play Music] [office radio] 'Office radio : We will play you great songs that the whole office can enjoy.'.
            # {u'cluster': [{u'category': u'1', u'id': u'search_situation', u'type': u'7'}],
            #  u'situation': {u'description': u'We\u2019ll play you some spacious, mind freeing music to help you concentrate and focus.',
            #                 u'id': u'Nlhbc6nnqlrrm2hsgsrffcmwoea',
            #                 u'imageUrl': u'http://lh3.googleusercontent.com/kAXyBVLZzefY_VxtRqKgP4mqi3y_arnBnTj_YbAtRrhUh5t0RljrOcekzEBh2qtCBmx6fOOV',
            #                 u'title': u'Deep focus',
            #                 u'wideImageUrl': u'http://lh3.googleusercontent.com/sNGG9qdeQ1aYc_0Cq3XrICPT-Rqnjt4wYOOMYWvwJOov_ZciPZfruOMhHkQmJdqEvD_veDUdaTM=s1080'},
            #  u'type': u'7'}
            # [Google Play Music] [office radio] 'Deep focus : We will play you some spacious, mind freeing music to help you concentrate and focus.'.

            situation_hits = results["station_hits"]
            # If there is no best result, then get a selection of tracks from
            # each situation. At least we'll play some music.
            if len(situation_hits):
                situation_dict = dict()
                situation_titles = list()
                for hit in situation_hits:
                    if hit and hit.get("station"):
                        situation = hit["station"]
                        situation_title = (
                            situation["name"] if situation.get("name") else ""
                        )
                        situation_desc = (
                            situation["description"]
                            if situation.get("description")
                            else None
                        )
                        if situation_desc:
                            if not additional_keywords:
                                print_nfo(
                                    "[Google Play Music] [{0}] '{1} : {2}'.".format(
                                        arg, situation_title, situation_desc
                                    )
                                )
                            else:
                                print_nfo(
                                    "[Google Play Music] [{0} - {1}] '{2} : {3}'.".format(
                                        arg,
                                        additional_keywords
                                        if additional_keywords
                                        else "(no keywords)",
                                        situation_title,
                                        situation_desc,
                                    )
                                )
                            if (
                                fuzz.partial_ratio(additional_keywords, situation_title)
                                > 50
                            ):
                                situation_titles.append(situation_title)
                                situation_dict[situation_title] = situation

                if len(situation_titles) > 1:
                    situation_title = process.extractOne(
                        additional_keywords, situation_titles
                    )[0]
                    situation = situation_dict[situation_title]
                elif len(situation_titles) == 1:
                    situation_title = situation_titles[0]
                    situation = situation_dict[situation_title]

            if situation:
                print_wrn(
                    "[Google Play Music] Playing '{0}'.".format(
                        to_ascii(situation_title)
                    )
                )
                self._enqueue_station_unlimited_v2(situation)

            if not situation:
                raise KeyError

        except KeyError:
            raise KeyError(
                "Activity not found : {0} - {1}".format(arg, additional_keywords)
            )

    def _enqueue_station_unlimited_v2(self, station):
        """Enqueue all tracks of a given station (Unlimited)

        """
        try:
            station_id = None
            if station:
                station_name = station["name"]
                seed = station["seed"]
                seed_type = seed["seedType"]
                track_id = seed["trackId"] if seed_type == "2" else None
                artist_id = seed["artistId"] if seed_type == "3" else None
                album_id = seed["albumId"] if seed_type == "4" else None
                genre_id = seed["genreId"] if seed_type == "5" else None
                playlist_token = (
                    seed["playlistShareToken"] if seed_type == "8" else None
                )
                curated_station_id = (
                    seed["curatedStationId"] if seed_type == "9" else None
                )
                num_tracks = MAX_TRACKS
                tracks = list()
                try:
                    station_id = self.gmusic.create_station(
                        station_name,
                        track_id,
                        artist_id,
                        album_id,
                        genre_id,
                        playlist_token,
                        curated_station_id,
                    )
                    tracks = self.gmusic.get_station_tracks(station_id, num_tracks)
                except KeyError:
                    raise RuntimeError(
                        "Operation requires an " "Unlimited subscription."
                    )
                tracks_added = self._enqueue_tracks(tracks)
                if tracks_added:
                    print_wrn(
                        "[Google Play Music] [Station] : '{0}'.".format(station_name)
                    )
                    logging.info(
                        "Added %d tracks from %s to queue",
                        tracks_added,
                        to_ascii(station_name),
                    )
                    self._update_play_queue_order()

        except KeyError:
            raise KeyError("Station not found")

    def _enqueue_podcast(self, arg):
        """Search for a podcast series and enqueue all of its tracks.

        """
        print_msg(
            "[Google Play Music] [Podcast search in "
            "Google Play Music] : '{0}'. ".format(to_ascii(arg))
        )
        try:
            podcast_hits = self._gmusic_search(arg, "podcast", 10, quiet=False)

            if not podcast_hits:
                print_wrn(
                    "[Google Play Music] [Podcast] 'Search returned zero results'."
                )
                print_wrn(
                    "[Google Play Music] [Podcast] 'Are you in a supported region "
                    "(currently only US and Canada) ?'"
                )

            # Use the first podcast retrieved. At least we'll play something.
            podcast = dict()
            if podcast_hits and len(podcast_hits):
                podcast = podcast_hits["series"]

            episodes_added = 0
            if podcast:
                # There is a podcast, enqueue its episodes.
                print_wrn(
                    "[Google Play Music] [Podcast] 'Playing '{0}' by {1}'.".format(
                        (podcast["title"]), (podcast["author"])
                    )
                )
                print_nfo(
                    "[Google Play Music] [Podcast] '{0}'.".format(
                        (podcast["description"][0:150])
                    )
                )
                series = self.gmusic.get_podcast_series_info(podcast["seriesId"])
                episodes = series["episodes"]
                for episode in episodes:
                    print_nfo(
                        "[Google Play Music] [Podcast Episode] '{0} : {1}'.".format(
                            (episode["title"]), (episode["description"][0:80])
                        )
                    )
                episodes_added = self._enqueue_tracks(episodes)

            if not podcast or not episodes_added:
                raise KeyError

        except KeyError:
            raise KeyError("Podcast not found or no episodes found: {0}".format(arg))

    def _enqueue_tracks(self, tracks):
        """ Add tracks to the playback queue

        """
        count = 0
        for track in tracks:
            if "id" not in list(track.keys()) and track.get("storeId"):
                track["id"] = track["storeId"]
            self.queue.append(track)
            count += 1
        return count

    def _update_playlists(self):
        """ Retrieve the user's playlists

        """
        logging.info("_update_playlists : updating users playlists")
        plists = self.gmusic.get_all_user_playlist_contents()
        for plist in plists:
            plist_name = plist.get("name")
            tracks = plist.get("tracks")
            if plist_name and len(tracks):
                logging.info(
                    "_update_playlists : playlist name : %s - num tracks %d",
                    to_ascii(plist_name),
                    len(tracks),
                )
                tracks.sort(key=itemgetter("creationTimestamp"))
                self.playlists[plist_name] = list()
                for track in tracks:
                    song_id = track.get("trackId")
                    if song_id:
                        song = self.song_map.get(song_id)
                        if song:
                            self.playlists[plist_name].append(song)
                        else:
                            song = track["track"]
                            song["id"] = track["trackId"]
                            self.playlists[plist_name].append(song)

            if not len(tracks):
                # Append a null item
                self.playlists[plist_name] = list()

    def _update_playlists_unlimited(self):
        """ Retrieve shared playlists (Unlimited)

        """
        plists_subscribed_to = [
            p for p in self.gmusic.get_all_playlists() if p.get("type") == "SHARED"
        ]
        for plist in plists_subscribed_to:
            share_tok = plist["shareToken"]
            playlist_items = self.gmusic.get_shared_playlist_contents(share_tok)
            plist_name = plist["name"]
            logging.info("shared playlist name : %s", to_ascii(plist_name))
            self.playlists[plist_name] = list()
            for item in playlist_items:
                try:
                    song = item["track"]
                    song["id"] = item["trackId"]
                    self.playlists[plist_name].append(song)
                except IndexError:
                    pass

    def _gmusic_search(self, query, query_type, max_results=MAX_TRACKS, quiet=False):
        """ Search Google Play (Unlimited)

        """

        search_results = self.gmusic.search(query, max_results)[query_type + "_hits"]

        # This is a workaround. Some podcast results come without these two
        # keys in the dictionary
        if (
            query_type == "podcast"
            and len(search_results)
            and not search_results[0].get("navigational_result")
        ):
            for res in search_results:
                res["best_result"] = False
                res["navigational_result"] = False
                res[query_type] = res["series"]

        result = ""
        if query_type != "playlist":
            result = next(
                (
                    hit
                    for hit in search_results
                    if "best_result" in list(hit.keys()) and hit["best_result"] == True
                ),
                None,
            )

        if not result and len(search_results):
            choices = dict()
            choice_names = list()
            for hit in search_results:
                name = ""
                if hit[query_type].get("name"):
                    name = hit[query_type].get("name")
                elif hit[query_type].get("title"):
                    name = hit[query_type].get("title")
                if not quiet:
                    if query_type == "album":
                        print_nfo(
                            "[Google Play Music] [{0}] '{1} ({2})'.".format(
                                query_type.capitalize(), name, hit["album"]["artist"]
                            )
                        )
                    elif query_type == "playlist":
                        playlist = hit["playlist"]
                        if playlist.get("ownerName"):
                            print_nfo(
                                "[Google Play Music] [{0}] '{1}' by '{2}'.".format(
                                    query_type.capitalize(), name, playlist["ownerName"]
                                )
                            )
                        else:
                            print_nfo(
                                "[Google Play Music] [{0}] '{1}'.".format(
                                    query_type.capitalize(), name
                                )
                            )
                    else:
                        print_nfo(
                            "[Google Play Music] [{0}] '{1}'.".format(
                                query_type.capitalize(), (name)
                            )
                        )
                choices[name] = hit
                choice_names.append(name)

            if len(choice_names) > 1:
                choice_name = process.extractOne(query, choice_names)[0]
                result = choices[choice_name]
            elif len(choice_names) == 1:
                choice_name = choice_names[0]
                result = choices[choice_name]

        if not result and not len(search_results):
            # Do another search with an empty string
            search_results = self.gmusic.search("")[query_type + "_hits"]

        if not result and len(search_results):
            # Play some random result from the search results
            random.seed()
            result = random.choice(search_results)
            if not quiet:
                print_adv(
                    "[Google Play Music] '{0}' not found. "
                    "Feeling lucky?.".format(query)
                )

        return result


if __name__ == "__main__":
    tizgmusicproxy()

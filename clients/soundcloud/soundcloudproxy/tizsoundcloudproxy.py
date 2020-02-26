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

"""@package tizsoundcloudproxy
Simple SoundCloud API proxy/wrapper.

Access SoundCloud using a user account to retrieve track URLs and create a play
queue for streaming.

"""

import os
import sys
import logging
import random
import soundcloud
import collections
import unicodedata
from requests.exceptions import HTTPError
from operator import itemgetter
from fuzzywuzzy import process
import imp
import configparser

if sys.version[0] == "2":
    imp.reload(sys)
    sys.setdefaultencoding("utf-8")

# For use during debugging
# from pprint import pprint

FORMAT = (
    "[%(asctime)s] [%(levelname)5s] [%(thread)d] "
    "[%(module)s:%(funcName)s:%(lineno)d] - %(message)s"
)

logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get("TIZONIA_SOUNDCLOUDPROXY_DEBUG"):
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

    print_err("[SoundCloud] (%s) : %s" % (exception_type.__name__, exception))

    if os.environ.get("TIZONIA_SOUNDCLOUDPROXY_DEBUG"):
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


class tizsoundcloudproxy(object):
    """A class that logs into a SoundCloud account, retrieves track URLs
    on behalf of the user and creates and manages a playback queue.

    """

    CLIENT_ID = "f3399c9c80866d417ae70009dfc95b2e"

    def __init__(self, oauth_token):
        self.__api = soundcloud.Client(
            client_id=self.CLIENT_ID, access_token=oauth_token
        )
        self.queue = list()
        self.queue_index = -1
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_track = None

    def logout(self):
        """ Reset the session to an unauthenticated, default state.

        """
        self.__api.logout()

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: curren tvalid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)
        self._update_play_queue_order()

    def enqueue_user_stream(self):
        """Adds the tracks in the user stream to the playback queue.

        """
        try:
            logging.info("enqueue_user_stream")
            stream_resource = self.__api.get("/me/activities", offset=0)
            count = 0
            stream = stream_resource.fields()
            for data in stream.get("collection"):
                item = data.get("origin")
                kind = item.get("kind")
                # multiple types of track with same data
                if "track" in kind:
                    if item["streamable"]:
                        self.queue.append(item)
                        count += 1
                if kind == "playlist":
                    playlist_tracks_uri = (
                        "/playlists/" + str(item.get("id")) + "/tracks"
                    )
                    tracks_resource = self.__api.get(playlist_tracks_uri, offset=0)
                    for resource in tracks_resource:
                        track = resource.fields()
                        if track["streamable"]:
                            self.queue.append(track)
                            count += 1
            if count == 0:
                raise RuntimeError("The user stream is empty")

            logging.info("Added {0} stream tracks to queue".format(count))
            self._update_play_queue_order()

        except KeyError:
            raise

    def enqueue_user_likes(self):
        """Adds the tracks that the user liked to the playback queue.

        """
        try:
            logging.info("enqueue_user_likes")
            likes_resource = self.__api.get("/me/favorites", limit=100)
            count = 0
            for resource in likes_resource:
                like = resource.fields()
                if like and like["streamable"]:
                    title = to_ascii(like.get("title"))
                    print_nfo("[SoundCloud] [Track] '{0}'.".format(title))
                    self.queue.append(like)
                    count += 1
                playlist = like.get("playlist")
                if playlist:
                    tracks = playlist.get("tracks")
                    if isinstance(tracks, collections.Iterable):
                        for track in tracks:
                            if track["streamable"]:
                                title = to_ascii(track.get("title"))
                                print_nfo("[SoundCloud] [Track] '{0}'.".format(title))
                                self.queue.append(track)
                                count += 1

            if count == 0:
                raise RuntimeError(
                    "SoundCloud did not return any" " tracks favourited by the user."
                )

            logging.info("Added {0} stream tracks to queue".format(count))
            self._update_play_queue_order()

        except KeyError:
            raise

    def enqueue_user_playlist(self, arg):
        """Search the user's collection for a playlist and add its tracks to the
        playback queue.

        :param arg: a playlist

        """
        logging.info("enqueue_playlist")
        try:
            resources = self.__api.get("/me/playlists")
            count = 0
            for resource in resources:
                playlist = resource.fields()
                pid = playlist.get("id")
                title = playlist.get("title")
                print_nfo("[SoundCloud] [Playlist] '{0}'.".format(title))
                if arg.lower() in title.lower():
                    playlist_resource = self.__api.get("/playlists/%s" % pid)
                    tracks = playlist_resource.tracks
                    for track in tracks:
                        if track["streamable"]:
                            title = to_ascii(track.get("title"))
                            print_nfo("[SoundCloud] [Track] '{0}'.".format(title))
                            self.queue.append(track)
                            count += 1

            if count == 0:
                raise RuntimeError("SoundCloud did not return any" " playlists.")

            logging.info("Added {0} stream tracks to queue".format(count))
            self._update_play_queue_order()

        except KeyError:
            raise KeyError(str("Playlist not found : %s" % arg))

    def enqueue_creator(self, arg):
        """Enqueue the last 50 tracks uploaded by a user/creator.

        :param arg: a creator

        """
        try:
            logging.info("enqueue_creator : %s", arg)

            resources = self.__api.get("/users", q=arg)
            count = 0
            for resource in resources:
                creator = resource.fields()
                cid = creator.get("id")
                username = creator.get("username")
                fullname = creator.get("full name")
                permalink = creator.get("permalink")
                track_count = creator.get("track_count")
                arg_permalink = permalink.replace(" ", "-").lower()
                if track_count == 0:
                    continue
                if (
                    arg.lower() == username.lower()
                    or arg_permalink == permalink.lower()
                    or (fullname and arg.lower() == fullname.lower())
                ):
                    try:
                        track_resources = self.__api.get(
                            "/users/%s/tracks" % cid, filter="streamable"
                        )
                    except (KeyError, AttributeError):
                        continue
                    for track_resource in track_resources:
                        track = track_resource.fields()
                        if track["streamable"]:
                            title = to_ascii(track.get("title"))
                            print_nfo("[SoundCloud] [Track] '{0}'.".format(title))
                            self.queue.append(track)
                            count += 1
                    if count > 0:
                        break

            if count == 0:
                raise RuntimeError(str("Creator not found : %s" % arg))

            logging.info("Added {0} user tracks to queue".format(count))
            self._update_play_queue_order()

        except KeyError:
            raise

    def enqueue_tracks(self, arg):
        """Search SoundCloud for tracks with a given title and add them
        to the playback queue.

        :param arg: a search string

        """
        logging.info("enqueue_tracks : %s", arg)
        try:
            page_size = 100
            track_resources = self.__api.get(
                "/tracks", q=arg, limit=page_size, filter="streamable"
            )
            count = 0
            for resource in track_resources:
                track = resource.fields()
                title = to_ascii(track.get("title"))
                print_nfo("[SoundCloud] [Track] '{0}'.".format(title))
                self.queue.append(track)
                count += 1

            if count == 0:
                raise RuntimeError(str("No tracks found : %s" % arg))

            logging.info("Added {0} tracks to queue".format(count))
            try:
                self.queue = sorted(
                    self.queue, key=itemgetter("likes_count"), reverse=True
                )
            except KeyError:
                pass

            self._update_play_queue_order()

        except KeyError:
            raise KeyError(str("No tracks found : %s" % arg))

    def enqueue_playlists(self, arg):
        """Search SoundCloud for playlists and add their tracks to the
        playback queue.

        :param arg: a search string

        """
        logging.info("enqueue_playlists : %s", arg)
        try:
            playlist_resources = self.__api.get("/playlists", q=arg)
            count = 0
            choice_titles = list()
            choices = dict()
            for resource in playlist_resources:
                playlist = resource.fields()
                pid = resource.id
                title = playlist.get("title")
                print_nfo("[SoundCloud] [Playlist] '{0}'.".format(to_ascii(title)))
                choice_titles.append(title)
                choices[title] = pid

            tracks = list()
            playlist_title = ""
            while len(choice_titles) and not len(tracks):
                playlist_title = process.extractOne(arg, choice_titles)[0]
                playlist_id = choices[playlist_title]
                playlist_resource = self.__api.get("/playlists/%s" % playlist_id)
                tracks = playlist_resource.tracks
                if not len(tracks):
                    print_err(
                        "[SoundCloud] '{0}' No tracks found.".format(
                            to_ascii(playlist_title)
                        )
                    )
                    del choices[playlist_title]
                    choice_titles.remove(playlist_title)

            print_wrn("[SoundCloud] Playing '{0}'.".format(to_ascii(playlist_title)))
            for track in tracks:
                if track["streamable"]:
                    self.queue.append(track)
                    title = to_ascii(track.get("title"))
                    print_nfo("[SoundCloud] [Track] '{0}'.".format(title))
                    count += 1

            if count == 0:
                raise RuntimeError(
                    "SoundCloud did not return any"
                    " playlists or returned an empty "
                    " playlist."
                )

            logging.info("Added {0} playlist tracks to queue".format(count))
            self._update_play_queue_order()

        except KeyError:
            raise KeyError(str("No playlists found : %s" % arg))

    def enqueue_genres(self, arg):
        """Search SoundCloud for a genre (or list of) and add its tracks to the
        playback queue.

        :param arg: a search string

        """
        logging.info("enqueue_genres : %s", arg)
        try:
            page_size = 100
            genre_resources = self.__api.get(
                "/tracks", genres=arg, limit=page_size, filter="streamable"
            )
            count = 0
            for resource in genre_resources:
                track = resource.fields()
                title = to_ascii(track.get("title"))
                print_nfo("[SoundCloud] [Track] '{0}'.".format(title))
                self.queue.append(track)
                count += 1

            if count == 0:
                raise RuntimeError(str("No genres found : %s" % arg))

            logging.info("Added {0} tracks to queue".format(count))
            try:
                self.queue = sorted(
                    self.queue, key=itemgetter("likes_count"), reverse=True
                )
            except KeyError:
                pass

            self._update_play_queue_order()

        except KeyError:
            raise KeyError(str("Genre not found : %s" % arg))
        except AttributeError:
            print(("Unexpected error:", sys.exc_info()[0]))

    def enqueue_tags(self, arg):
        """Search SoundCloud for a tag (or list of) and add its tracks to
        the playback queue.

        :param arg: a search string

        """
        logging.info("enqueue_tags : %s", arg)
        try:
            page_size = 100
            tag_resources = self.__api.get(
                "/tracks", tags=arg, limit=page_size, filter="streamable"
            )
            count = 0
            for resource in tag_resources:
                track = resource.fields()
                title = to_ascii(track.get("title"))
                print_nfo("[SoundCloud] [Track] '{0}'.".format(title))
                self.queue.append(track)
                count += 1

            if count == 0:
                raise RuntimeError(str("No tags found : %s" % arg))

            logging.info("Added {0} tracks to queue".format(count))
            try:
                self.queue = sorted(
                    self.queue, key=itemgetter("likes_count"), reverse=True
                )
            except KeyError:
                pass

            self._update_play_queue_order()

        except KeyError:
            raise KeyError(str("Tag(s) not found : %s" % arg))
        except AttributeError:
            print(("Unexpected error:", sys.exc_info()[0]))

    def current_track_title_and_user(self):
        """ Retrieve the current track's title and user name.

        """
        logging.info("current_track_title_and_user")
        track = self.now_playing_track
        title = ""
        user = ""
        if track:
            try:
                title = to_ascii(track.get("title"))
                user = to_ascii(track["user"]["username"])
                logging.info("Now playing {0} by {1}".format(title, user))
            except KeyError:
                logging.info("title/user : not found")
        if sys.version[0] == "2":
            return title, user
        else:
            return title, user

    def current_track_duration(self):
        """ Retrieve the current track's duration.

        """
        logging.info("current_track_duration")
        track = self.now_playing_track
        track_duration = 0
        if track:
            try:
                duration = track.get("duration")
                if duration:
                    track_duration = duration
                logging.info("duration {0}".format(duration))
            except KeyError:
                logging.info("duration : not found")
        return track_duration

    def current_track_year(self):
        """ Return the current track's year of publication.

        """
        logging.info("current_track_year")
        track = self.now_playing_track
        track_year = 0
        if track:
            try:
                year = track.get("release_year")
                if year:
                    track_year = year
                logging.info("track year {0}".format(year))
            except KeyError:
                logging.info("year : not found")
        return track_year

    def current_track_permalink(self):
        """ Return the current track's permalink.

        """
        logging.info("current_track_permalink")
        track = self.now_playing_track
        track_permalink = ""
        if track:
            try:
                permalink = track.get("permalink_url")
                if permalink:
                    track_permalink = permalink
                logging.info("track permalink {0}".format(permalink))
            except KeyError:
                logging.info("permalink : not found")
        return track_permalink

    def current_track_license(self):
        """ Return the current track's license.

        """
        logging.info("current_track_license")
        track = self.now_playing_track
        track_license = ""
        if track:
            try:
                tlicense = track.get("license")
                if tlicense:
                    track_license = tlicense
                logging.info("track license {0}".format(tlicense))
            except KeyError:
                logging.info("license : not found")
        return track_license

    def current_track_likes(self):
        """ Return the current track's likes.

        """
        logging.info("current_track_likes")
        track = self.now_playing_track
        track_likes = 0
        if track:
            try:
                likes = track.get("likes_count")
                if likes:
                    track_likes = likes
                logging.info("track likes {0}".format(likes))
            except KeyError:
                logging.info("likes : not found")
        return track_likes

    def current_track_user_avatar(self):
        """ Return the avatar of the user associated with the current track.

        """
        logging.info("current_track_user_avatar")
        track = self.now_playing_track
        track_user_avatar = ""
        if track:
            try:
                user = track.get("user")
                if user:
                    user_avatar = user.get("avatar_url")
                    if user_avatar:
                        track_user_avatar = user_avatar
                        logging.info("track user_avatar {0}".format(user_avatar))
            except KeyError:
                logging.info("user_avatar : not found")
        return track_user_avatar

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1

    def next_url(self):
        """ Retrieve the url of the next track in the playback queue.

        """
        logging.info("next_url")
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
                return to_ascii("")
        except (KeyError, AttributeError, HTTPError):
            del self.queue[self.queue_index]
            print_err("[SoundCloud] 'HTTP 404 while retrieving the track URL.")
            return self.next_url()

    def prev_url(self):
        """ Retrieve the url of the previous track in the playback queue.

        """
        logging.info("prev_url")
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
        except (KeyError, AttributeError, HTTPError):
            del self.queue[self.queue_index]
            return self.prev_url()

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
            print_nfo("[SoundCloud] [Tracks in queue] '{0}'.".format(total_tracks))

    def _retrieve_track_url(self, track):
        """ Retrieve a track url

        """
        logging.info("_retrieve_track_url : {0}".format(track["id"]))
        try:
            self.now_playing_track = track
            stream_url = track["stream_url"]
            stream = self.__api.get(stream_url, allow_redirects=False)
            return stream.location
        except AttributeError:
            logging.info("Could not retrieve the track url!")
            raise


if __name__ == "__main__":
    tizsoundcloudproxy()

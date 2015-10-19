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

"""Simple SoundCloud proxy class.

Access a user's SoundCloud account to retrieve track URLs to be used for
streaming.

"""

from __future__ import unicode_literals

import sys
import logging
import random
from operator import itemgetter
import requests
from requests.structures import CaseInsensitiveDict
import soundcloud

import pprint

import collections
import re
import string
import time
import unicodedata
from multiprocessing.pool import ThreadPool
from urllib import quote_plus

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
    print_err("[SoundCloud] %s" % exception)

sys.excepthook = exception_handler

class TizEnumeration(set):
    """A simple enumeration class.

    """
    def __getattr__(self, name):
        if name in self:
            return name
        raise AttributeError

class tizsoundcloudproxy(object):
    """A class for logging into a SoundCloud account and retrieving track
    URLs.

    """
    CLIENT_ID = 'f3399c9c80866d417ae70009dfc95b2e'
    CLIENT_SECRET = 'xxx'

    def __init__(self, email, password):
        self.__email = email
        self.__api = soundcloud.Client(
            client_id=self.CLIENT_ID,
            client_secret=self.CLIENT_SECRET,
            username=email,
            password=password
        )
        me = self.__api.get('/me')

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
        self.__update_play_queue_order()

    def enqueue_stream(self):
        """Adds the tracks in the user stream to the playback queue.

        """
        try:
            logging.info("enqueue_stream")
            # TODO
        except KeyError:
            raise KeyError(str("User not found"))

    def enqueue_user_playlist(self, arg):
        """Search the user's collection for a playlist and add its tracks to the
        playback queue.

        :param arg: a playlist

        """
        logging.info("enqueue_playlist")
        try:
            playlists = self.__api.get('/me/playlists')
            count = 0
            for playlist in playlists:
                fields = playlist.fields()
                pid = playlist.id
                title = fields['title']
                print_nfo("[SoundCloud] [Playlist] '{0}'." \
                          .format(title.encode("utf-8")))
                if arg.lower() in title.encode("utf-8").lower():
                    pl = self.__api.get('/playlists/%s' % pid)
                    tracks = pl.tracks
                    for track in tracks:
                        if track['streamable']:
                            self.queue.append(track)
                            count += 1
            if count == 0:
                raise KeyError
            logging.info("Added {0} playlist tracks to queue" \
                         .format(count))
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError(str("Playlist not found : %s" % arg))

    def enqueue_creator(self, arg):
        """Enqueue the last 50 tracks uploaded by a user/creator.

        :param arg: a creator

        """
        try:
            logging.info('enqueue_creator : %s' % arg)

            resources = self.__api.get('/users', q=arg)
            count = 0
            for resource in resources:
                creator = resource.fields()
                cid = creator.get('id')
                username = creator.get('username')
                fullname = creator.get('full name')
                permalink = creator.get('permalink')
                track_count = creator.get('track_count')
                arg_permalink = permalink.replace(' ', '-').lower()
                if track_count == 0:
                    continue
                if arg.lower() == username.encode("utf-8").lower() \
                   or arg_permalink == permalink.encode("utf-8").lower() \
                   or (fullname and arg.lower() == fullname.encode("utf-8").lower()):
                    try:
                        track_resources = self.__api.get('/users/%s/tracks' % cid, filter='streamable')
                    except:
                        continue
                    for track_resource in track_resources:
                        track = track_resource.fields()
                        if track['streamable']:
                            self.queue.append(track)
                            count += 1
                    if count > 0:
                        break;
            if count == 0:
                raise KeyError(str("Creator not found : %s" % arg))
            logging.info("Added {0} user tracks to queue" \
                         .format(count))
            self.__update_play_queue_order()

        except KeyError:
            raise

    def enqueue_tracks(self, arg):
        """Search SoundCloud for tracks with a given title and add them to the playback
        queue.

        :param arg: a search string

        """
        logging.info('enqueue_tracks : %s' % arg)
        try:
            page_size = 100
            track_resources = self.__api.get('/tracks', q=arg, limit=page_size, filter='streamable')
            count = 0
            for resource in track_resources:
                track = resource.fields()
                title = track.get('title')
                print_nfo("[SoundCloud] [Track] '{0}'." \
                          .format(title.encode("utf-8")))
                self.queue.append(track)
                count += 1
            if count == 0:
                raise KeyError
            logging.info("Added {0} tracks to queue" \
                         .format(count))
            self.queue = sorted(self.queue, key=itemgetter('likes_count'), reverse=True)
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError(str("No tracks found : %s" % arg))

    def enqueue_playlists(self, arg):
        """Search SoundCloud for playlists and add their tracks to the playback queue.

        :param arg: a search string

        """
        logging.info('enqueue_playlists : %s' % arg)
        try:
            playlist_resources = self.__api.get('/playlists', q=arg)
            count = 0
            for resource in playlist_resources:
                playlist = resource.fields()
                pid = resource.id
                title = playlist.get('title')
                print_nfo("[SoundCloud] [Playlist] '{0}'." \
                          .format(title.encode("utf-8")))
                if arg.lower() in title.encode("utf-8").lower():
                    pl = self.__api.get('/playlists/%s' % pid)
                    tracks = pl.tracks
                    for track in tracks:
                        if track['streamable']:
                            self.queue.append(track)
                            count += 1
            if count == 0:
                raise KeyError
            logging.info("Added {0} playlist tracks to queue" \
                         .format(count))
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError(str("No playlists found : %s" % arg))

    def enqueue_genres(self, arg):
        """Search SoundCloud for a genre (or list of) and add its tracks to the
        playback queue.

        :param arg: a search string

        """
        logging.info('enqueue_genres : %s' % arg)
        try:
            page_size = 100
            genre_resources = self.__api.get('/tracks', genres=arg, limit=page_size, filter='streamable')
            count = 0
            for resource in genre_resources:
                track = resource.fields()
                gid = track.get('id')
                title = track.get('title')
                title = unicodedata.normalize('NFKD', unicode(title)).encode('ASCII', 'ignore')
                print_nfo("[SoundCloud] [Track] '{0}'." \
                          .format(title))
                self.queue.append(track)
                count += 1
            if count == 0:
                raise KeyError
            logging.info("Added {0} tracks to queue" \
                         .format(count))
            self.queue = sorted(self.queue, key=itemgetter('likes_count'), reverse=True)
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError(str("Genre not found : %s" % arg))
        except AttributeError:
            print("Unexpected error:", sys.exc_info()[0])

    def enqueue_tags(self, arg):
        """Search SoundCloud for a tag (or list of) and add its tracks to the playback
        queue.

        :param arg: a search string

        """
        logging.info('enqueue_tags : %s' % arg)
        try:
            page_size = 100
            tag_resources = self.__api.get('/tracks', tags=arg, limit=page_size, filter='streamable')
            count = 0
            for resource in tag_resources:
                track = resource.fields()
                gid = track.get('id')
                title = track.get('title')
                title = unicodedata.normalize('NFKD', unicode(title)).encode('ASCII', 'ignore')
                print_nfo("[SoundCloud] [Track] '{0}'." \
                          .format(title))
                self.queue.append(track)
                count += 1
            if count == 0:
                raise KeyError
            logging.info("Added {0} tracks to queue" \
                         .format(count))
            self.queue = sorted(self.queue, key=itemgetter('likes_count'), reverse=True)
            self.__update_play_queue_order()

        except KeyError:
            raise KeyError(str("Tag(s) not found : %s" % arg))
        except AttributeError:
            print("Unexpected error:", sys.exc_info()[0])

    def current_track_title_and_user(self):
        """ Retrieve the current track's title and user name.

        """
        logging.info("current_track_title_and_user")
        track = self.now_playing_track
        title = ''
        user = ''
        if track:
            try:
                title = unicodedata.normalize('NFKD', unicode(track.get('title'))).encode('ASCII', 'ignore')
                user = unicodedata.normalize('NFKD', unicode(track['user']['username'])).encode('ASCII', 'ignore')
                logging.info("Now playing {0} by {1}".format(title.encode("utf-8"),
                                                             user.encode("utf-8")))
            except KeyError:
                logging.info("title/user : not found")
        return title.encode("utf-8"), user.encode("utf-8")

    def current_track_duration(self):
        """ Retrieve the current track's duration.

        """
        logging.info("current_track_duration")
        track = self.now_playing_track
        track_duration = 0
        if track:
            try:
                duration = track.get('duration')
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
                year = track.get('release_year')
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
        track_permalink = ''
        if track:
            try:
                permalink = track.get('permalink_url')
                if permalink:
                    track_permalink = permalink
                logging.info("track permalink {0}".format(permalink))
            except KeyError:
                logging.info("permalink : not found")
        return track_permalink.encode("utf-8")

    def current_track_license(self):
        """ Return the current track's license.

        """
        logging.info("current_track_license")
        track = self.now_playing_track
        track_license = ''
        if track:
            try:
                tlicense = track.get('license')
                if tlicense:
                    track_license = tlicense
                logging.info("track license {0}".format(tlicense))
            except KeyError:
                logging.info("license : not found")
        return track_license.encode("utf-8")

    def current_track_likes(self):
        """ Return the current track's likes.

        """
        logging.info("current_track_likes")
        track = self.now_playing_track
        track_likes = 0
        if track:
            try:
                likes = track.get('likes_count')
                if likes:
                    track_likes = likes
                logging.info("track likes {0}".format(likes))
            except KeyError:
                logging.info("likes : not found")
        return track_likes

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
                if (self.queue_index < len(self.queue)) \
                   and (self.queue_index >= 0):
                    next_track = self.queue[self.play_queue_order[self.queue_index]]
                    return self.__retrieve_track_url(next_track)
                else:
                    self.queue_index = -1
                    return self.next_url()
            else:
                return ''
        except:
            del self.queue[self.queue_index]
            return self.prev_url()

    def prev_url(self):
        """ Retrieve the url of the previous track in the playback queue.

        """
        logging.info("prev_url")
        try:
            if len(self.queue):
                self.queue_index -= 1
                if (self.queue_index < len(self.queue)) \
                   and (self.queue_index >= 0):
                    prev_track = self.queue[self.play_queue_order[self.queue_index]]
                    return self.__retrieve_track_url(prev_track)
                else:
                    self.queue_index = len(self.queue)
                    return self.prev_url()
            else:
                return ''
        except:
            del self.queue[self.queue_index]
            return self.prev_url()


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

    def __retrieve_track_url(self, track):
        """ Retrieve a track url

        """
        logging.info("__retrieve_track_url : {0}".format(track['id']))
        try:
            self.now_playing_track = track
            #pprint.pprint(track)
            stream_url = track['stream_url']
            stream = self.__api.get(stream_url, allow_redirects=False)
            #pprint.pprint("location {0}".format(stream.location))
            return stream.location.encode("utf-8")
        except AttributeError:
            logging.info("Could not retrieve the track url!")
            raise

if __name__ == "__main__":
    tizsoundcloudproxy()

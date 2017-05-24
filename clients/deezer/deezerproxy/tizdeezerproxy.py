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

import sys
import logging
import random
import collections
import unicodedata
from requests.exceptions import HTTPError
from operator import itemgetter

from .deezer import DeezerClient

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

    print_err("[Deezer] (%s) : %s" % (exception_type.__name__, exception))
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

class tizdeezerproxy(object):
    """A class that logs into a Deezer account, retrieves track URLs
    on behalf of the user and creates and manages a playback queue.

    """
    def __init__(self, data_dir, user_id):
        self.__api = DeezerClient(data_dir, user_id)
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

    def enqueue_album(self, arg):
        """ Search the user's library for albums with a given name and adds
        them to the playback queue.

        """
        try:
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

if __name__ == "__main__":
    tizdeezerproxy()

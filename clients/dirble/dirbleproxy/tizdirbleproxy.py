# Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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

"""@package tizdirbleproxy
Simple Dirble proxy/wrapper.

Access Dirble to retrieve station URLs and create a play queue for streaming.

"""

from __future__ import unicode_literals

import sys
import os
import logging
import random
import unicodedata
import urllib
import pkgutil
import requests
import StringIO
import eventlet
from collections import namedtuple
from requests import Session, exceptions
from requests.adapters import HTTPAdapter
from operator import itemgetter
eventlet.monkey_patch()

# For use during debugging
# import pprint

FORMAT = '[%(asctime)s] [%(levelname)5s] [%(thread)d] ' \
         '[%(module)s:%(funcName)s:%(lineno)d] - %(message)s'

logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get('TIZONIA_DIRBLEPROXY_DEBUG'):
    logging.basicConfig(format=FORMAT)
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
    """A simple handler that prints the exception message.

    """

    print_err("[Dirble] (%s) : %s" % (exception_type.__name__, exception))

    if os.environ.get('TIZONIA_DIRBLEPROXY_DEBUG'):
        print_exception(exception_type, exception, traceback)

sys.excepthook = exception_handler

class m3u_station():
    def __init__(self, length, title, path):
        self.length = length
        self.title = title
        self.path = path

def parse_m3u(url):
    """
        Parse m3u file.
    """

    content_type = ''
    req = None
    data = None
    try:
        req = requests.get(url, timeout=0.2)
    except:
        pass
    if req:
        data = req.text

    m3u = StringIO.StringIO(data)
    line = m3u.readline()
    if not line.startswith('#EXTM3U'):
        return

    # initialize playlist variables before reading file
    playlist = []
    song = m3u_station(None, None, None)

    for line in m3u.readlines():
        line = line.strip()
        if line.startswith('#EXTINF:'):
            # length and title from #EXTINF line
            length, title = line.split('#EXTINF:')[1].split(',',1)
            song = m3u_station(length, title, None)
        elif (len(line) != 0):
            # song path from all other, non-blank lines
            song.path = line
            playlist.append(song)
            # reset the song variable
            song = m3u_station(None, None, None)

    m3u.close()

    return playlist

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

class tizdirbleproxy(object):
    """A class that accesses Dirble, retrieves station URLs and creates and manages
    a playback queue.

    """
    base_url = 'http://api.dirble.com/v2/'

    Station = namedtuple("Station", "id name country website category streamurl bitrate content_type")

    def __init__(self, api_key):
        self.key = api_key
        self.base_url = tizdirbleproxy.base_url
        self.queue = list()
        self.queue_index = -1
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_station = None

        self._api = Session()
        self._api.params = {'token': api_key}
        self._api.headers['User-Agent'] = ' '.join([
            'Tizonia',
            self._api.headers['User-Agent']])
        self._api.mount(self.base_url, HTTPAdapter(max_retries=3))

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: current valid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)
        self.__update_play_queue_order()

    def enqueue_popular_stations(self):
        """Search Dirble for popular stations and add them to the playback
        queue.

        :param arg: a search string

        """
        logging.info('enqueue_popular_stations')
        try:
            count = len(self.queue)
            self._api.params = {'token': self.key}
            for d in self.api_call("stations/popular"):
                self.add_to_playback_queue(d)

            logging.info("Added {0} stations to queue" \
                         .format(len(self.queue) - count))

            if count == len(self.queue):
                raise ValueError

            self.__update_play_queue_order()

        except ValueError:
            raise ValueError(str("No stations found"))

    def enqueue_category(self, arg):
        """Search Dirble for a station category and add its stations to the
        playback queue.

        :param arg: a search string

        """
        logging.info('enqueue_category : %s', arg)
        try:
            catids = list ()
            self._api.params = {'token': self.key}
            for d in self.api_call("categories"):
                title = to_ascii(d["title"])
                print_nfo("[Dirble] [Category] '{0}'.".format(title))
                if arg.lower() in title.lower():
                    catids.append(d["id"])

            count = len(self.queue)
            for catid in catids:
                for p in range(0, 10):
                    self._api.params = {'token': self.key, 'page': p}
                    for d in self.api_call("category/{0}/stations".format(catid)):
                        self.add_to_playback_queue(d)

            logging.info("Added {0} stations to queue" \
                         .format(len(self.queue) - count))

            if count == len(self.queue):
                raise ValueError

            self.__update_play_queue_order()

        except ValueError:
            raise ValueError(str("No stations found : %s" % arg))

    def enqueue_country(self, arg):
        """Search Dirble for stations from a particular country code and add them to
        the playback queue.

        :param arg: a search string

        """
        logging.info('enqueue_country : %s', arg)
        try:
            country = arg

            # Try to find the actual country code from the country name, but
            # only if pycountry and titlecase are available
            if len(arg) != 2 and len(arg) != 3:
                pycountry_loader = pkgutil.find_loader('pycountry')
                titlecase_loader = pkgutil.find_loader('titlecase')
                if pycountry_loader and titlecase_loader:
                    pycountry = pycountry_loader.load_module("pycountry")
                    titlecase = titlecase_loader.load_module("titlecase")
                    country = titlecase.titlecase(arg)
                    try:
                        country = pycountry.countries.get(name=country).alpha2
                    except AttributeError:
                        country = pycountry.countries.get(name=country).alpha_2

            count = len(self.queue)
            for p in range(0, 10):
                self._api.params = {'token': self.key, 'page': p}
                for d in self.api_call("countries/{0}/stations".format(country.upper())):
                    self.add_to_playback_queue(d)

            logging.info("Added {0} stations to queue" \
                         .format(len(self.queue) - count))

            if count == len(self.queue):
                raise ValueError

            self.__update_play_queue_order()

        except ValueError:
            raise ValueError(str("No stations found : %s" % arg))

    def enqueue_stations(self, arg):
        """Search Dirble for a station name and add all matches to the
        playback queue.

        :param arg: a search string

        """
        logging.info('enqueue_stations : %s', arg)
        try:
            count = len(self.queue)
            for p in range(0, 10):
                self._api.params = {'token': self.key, 'page': p}
                for d in self.api_call("search/{0}".format(arg)):
                    self.add_to_playback_queue(d)

            logging.info("Added {0} stations to queue" \
                         .format(len(self.queue) - count))

            if count == len(self.queue):
                raise ValueError

            self.__update_play_queue_order()

        except ValueError:
            raise ValueError(str("No stations found : %s" % arg))

    def current_station_name_and_country(self):
        """ Retrieve the current station's name and country.

        """
        logging.info("current_station_name")
        station = self.now_playing_station
        name = ''
        country = ''
        if station:
            name = to_ascii(station.name).encode("utf-8")
            country = to_ascii(station.country).encode("utf-8")
        return name, country

    def current_station_category(self):
        """ Retrieve the current station's category.

        """
        logging.info("current_station_category")
        station = self.now_playing_station
        category = ''
        if station:
            category = to_ascii(station.category).encode("utf-8")
        return category

    def current_station_website(self):
        """ Retrieve the current station's website.

        """
        logging.info("current_station_website")
        station = self.now_playing_station
        website = ''
        if station:
            website = to_ascii(station.website).encode("utf-8")
        return website

    def current_station_bitrate(self):
        """ Retrieve the current station's bitrate.

        """
        logging.info("current_station_bitrate")
        station = self.now_playing_station
        bitrate = 0
        if station:
            bitrate = station.bitrate
        return bitrate

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1

    def remove_current_url(self):
        """Remove the currently active url from the playback queue.

        """
        logging.info("remove_current_url")
        if len(self.queue) and self.queue_index:
            station = self.queue[self.queue_index]
            print_err("[Dirble] '{0}' removed from queue." \
                      .format(to_ascii(station.name).encode("utf-8")))
            del self.queue[self.queue_index]
            self.queue_index -= 1
            if self.queue_index < 0:
                self.queue_index = 0
            self.__update_play_queue_order()

    def next_url(self):
        """ Retrieve the url of the next station in the playback queue.

        """
        logging.info("next_url")
        try:
            if len(self.queue):
                self.queue_index += 1
                if (self.queue_index < len(self.queue)) \
                   and (self.queue_index >= 0):
                    next_station = self.queue[self.play_queue_order \
                                            [self.queue_index]]
                    return self.__retrieve_station_url(next_station).rstrip()
                else:
                    self.queue_index = -1
                    return self.next_url()
            else:
                return ''
        except (KeyError, AttributeError):
            del self.queue[self.queue_index]
            return self.next_url()

    def prev_url(self):
        """ Retrieve the url of the previous station in the playback queue.

        """
        logging.info("prev_url")
        try:
            if len(self.queue):
                self.queue_index -= 1
                if (self.queue_index < len(self.queue)) \
                   and (self.queue_index >= 0):
                    prev_station = self.queue[self.play_queue_order \
                                            [self.queue_index]]
                    return self.__retrieve_station_url(prev_station).rstrip()
                else:
                    self.queue_index = len(self.queue)
                    return self.prev_url()
            else:
                return ''
        except (KeyError, AttributeError):
            del self.queue[self.queue_index]
            return self.prev_url()

    def __update_play_queue_order(self):
        """ Update the queue playback order.

        A sequential order is applied if the current play mode is "NORMAL" or a
        random order if current play mode is "SHUFFLE"

        """
        total_stations = len(self.queue)
        if total_stations:
            if not len(self.play_queue_order):
                # Create a sequential play order, if empty
                self.play_queue_order = range(total_stations)
            if self.current_play_mode == self.play_modes.SHUFFLE:
                random.shuffle(self.play_queue_order)
            print_nfo("[Dirble] [Stations in queue] '{0}'." \
                      .format(total_stations))

    def __retrieve_station_url(self, station):
        """ Retrieve a station url

        """
        try:
            self.now_playing_station = station
            logging.info("__retrieve_station_url streamurl : {0}".format(station.streamurl))
            logging.info("__retrieve_station_url bitrate   : {0}".format(station.bitrate))
            logging.info("__retrieve_station_url content_type: {0}".format(station.content_type))
            print_wrn("[Dirble] Playing '{0} ({1})'." \
                          .format(to_ascii(station.name).encode("utf-8").rstrip(),
                          to_ascii(station.streamurl).encode("utf-8").rstrip()))
            return station.streamurl.encode("utf-8")
        except AttributeError:
            logging.info("Could not retrieve the station url!")
            raise

    def api_call(self, path):
        uri = self.base_url + urllib.quote(path)

        logging.debug('Fetching: %s', uri)
        try:
            resp = self._api.get(uri)

            if resp.status_code == 200:
                data = resp.json(
                    object_hook=lambda d: {k.lower(): v for k, v in d.items()})
                return data

            logging.debug('Fetch failed, HTTP %s', resp.status_code)

            if resp.status_code == 404:
                return []

        except exceptions.RequestException as e:
            logging.debug('Fetch failed: %s', e)
        except ValueError as e:
            logging.warning('Fetch failed: %s', e)

        return []

    def _examine_url(self, url):
        content_type = ''
        req = None
        try:
            with eventlet.Timeout(2):
                req = requests.get(url, verify=False, timeout=0.1)
        except:
            pass
        if req:
            content_type = req.headers.get('Content-Type')
        return content_type

    def add_to_playback_queue(self, d):
        catlist = list()
        for cat in d["categories"]:
            catlist.append(cat["title"])
        separator = ', '
        category = separator.join(catlist)

        streamurl = ''
        bitrate = ''
        for stream in d['streams']:
            streamurl = stream['stream']
            bitrate = stream['bitrate']
            content_type = stream['content_type']
            if content_type:
                content_type = content_type.rstrip()
                if content_type == '?':
                    content_type = self._examine_url(streamurl)
                if content_type:
                    if content_type == 'audio/x-mpegurl' \
                       or content_type == 'application/mpegurl' \
                       or content_type == 'application/x-mpegurl' \
                       or content_type == 'audio/mpegurl':
                        playlist = parse_m3u(streamurl)
                        if playlist:
                            # TODO: loop through the available urls in the playlist
                            streamurl = playlist[0].path.rstrip()

                    streamurl = streamurl.rstrip()
                    print_nfo("[Dirble] [Station] '{0}' [{1}] ({2})." \
                              .format(to_ascii(d['name']), \
                                      to_ascii(content_type), to_ascii(streamurl)))
                    self.queue.append(
                        tizdirbleproxy.Station(d['id'], d['name'], d['country'], \
                                               d['website'], \
                                               category, \
                                               streamurl, \
                                               bitrate,
                                               content_type))

if __name__ == "__main__":
    tizdirbleproxy()

# Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
#
# Portions Copyright (C) 2020 Nick Steel and contributors
# (see https://github.com/kingosticks/mopidy-iheart)
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

"""@package tiziheartproxy
Simple Iheart proxy/wrapper.

Access Iheart to retrieve station URLs and create a play queue for streaming.

With ideas and code from mopidy_iheart. For further information see:
- https://github.com/kingosticks/mopidy-iheart/blob/master/mopidy_iheart/iheart.py

"""

from __future__ import unicode_literals

import configparser
import sys
import os
import io
import re
import logging
import random
import datetime
import getpass
import xml.etree.ElementTree as elementtree
import urllib.parse
import json
from collections import OrderedDict
from contextlib import closing
from urllib.parse import urlparse
import requests
from joblib import Memory
from fuzzywuzzy import fuzz

# FOR REFERENCE
# -> station
# {'band': 'FM',
#  'callLetters': 'WMGK-FM',
#  'city': 'Philadelphia',
#  'dartUrl': None,
#  'description': "Philadelphia's Classic Rock",
#  'frequency': '102.9',
#  'id': 5297,
#  'logo': '{img_url_1}/2135/2012/04/default/wmgk-fm_logo_0_1333655870.png',
#  'name': 'Classic Rock 102.9 MGK',
#  'newlogo': 'http://i.iheart.com/v3/re/assets/images/5297.png',
#  'rank': None,
#  'score': 6.2353005,
#  'shareLink': None,
#  'state': 'PA'}

# FOR REFERENCE
# -> station info
# {'adswizz': {'adswizzHost': '',
#              'enableAdswizzTargeting': 'false',
#              'publisher_id': '0'},
#  'adswizzZones': {},
#  'band': 'FM',
#  'callLetters': 'WMGK-FM',
#  'countries': 'US',
#  'cume': 1138700,
#  'description': "Philadelphia's Classic Rock",
#  'feeds': {'enableTritonTracking': 'false',
#            'feed': 'www.iheart.com/live/5297/'},
#  'format': 'Prov_Beasley',
#  'freq': '102.9',
#  'genres': [{'id': 3,
#              'name': 'Classic Rock',
#              'primary': True,
#              'sortIndex': 15}],
#  'id': 5297,
#  'isActive': True,
#  'link': 'https://www.iheart.com/live/5297/?autoplay=true',
#  'logo': 'http://i.iheart.com/v3/re/assets/images/5297.png',
#  'markets': [{'city': 'Philadelphia',
#               'cityId': 196,
#               'country': 'US',
#               'countryId': 1,
#               'marketId': '196',
#               'name': 'PHILADELPHIA-PA',
#               'origin': True,
#               'primary': True,
#               'sortIndex': 2,
#               'stateAbbreviation': 'PA',
#               'stateId': 45}],
#  'modified': '1561472046202',
#  'name': 'Classic Rock 102.9 MGK',
#  'pronouncements': [],
#  'provider': 'Beasley',
#  'rds': '',
#  'responseType': 'LIVE',
#  'score': 0.0,
#  'social': {},
#  'streams': {'pls_stream': 'http://playerservices.streamtheworld.com/pls/WMGKFMAACIHR.pls',
#              'secure_pls_stream': 'https://playerservices.streamtheworld.com/pls/WMGKFMAACIHR.pls'},
#  'website': 'www.wmgk.com'}


# For use during debugging
# from pprint import pprint

NOW = datetime.datetime.now()
TMPDIR = "/var/tmp"
CACHE_DIR_PREFIX = os.getenv("SNAP_USER_COMMON") or TMPDIR

IHEART_CACHE_LOCATION = os.path.join(
    CACHE_DIR_PREFIX, "tizonia-" + getpass.getuser() + "-iheart"
)
MEMORY = Memory(IHEART_CACHE_LOCATION, compress=9, verbose=0, bytes_limit=10485760)
MEMORY.reduce_size()

FORMAT = (
    "[%(asctime)s] [%(levelname)5s] [%(thread)d] "
    "[%(module)s:%(funcName)s:%(lineno)d] - %(message)s"
)

logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get("TIZONIA_IHEARTPROXY_DEBUG"):
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

    print_err("[iHeart] (%s) : %s" % (exception_type.__name__, exception))

    if os.environ.get("TIZONIA_IHEARTPROXY_DEBUG"):
        print_exception(exception_type, exception, traceback)


sys.excepthook = exception_handler

# From https://github.com/kingosticks/mopidy-iheart/blob/master/mopidy_iheart/iheart.py
def parse_m3u(data):
    # Copied from mopidy.audio.playlists
    # Mopidy version expects a header but it's not always present
    for line in data.splitlines():
        if not line.strip() or line.startswith(b"#"):
            continue

        try:
            line = line.decode()
        except UnicodeDecodeError:
            continue

        yield line.strip()


# From https://github.com/kingosticks/mopidy-iheart/blob/master/mopidy_iheart/iheart.py
def parse_pls(data):
    # Copied from mopidy.audio.playlists
    try:
        cp = configparser.RawConfigParser()
        cp.read_string(data.decode())
    except configparser.Error:
        return

    for section in cp.sections():
        if section.lower() != "playlist":
            continue
        for i in range(cp.getint(section, "numberofentries")):
            try:
                # TODO: Remove this horrible hack to avoid adverts
                if cp.has_option(section, f"length{i + 1}"):
                    if cp.get(section, f"length{i + 1}") == "-1":
                        yield cp.get(section, f"file{i + 1}").strip("\"'")
                else:
                    yield cp.get(section, f"file{i + 1}").strip("\"'")
            except configparser.NoOptionError:
                return


def fix_asf_uri(uri):
    return re.sub(r"http://(.+\?mswmext=\.asf)", r"mms://\1", uri, flags=re.I)


def parse_old_asx(data):
    try:
        cp = configparser.RawConfigParser()
        cp.read_string(data.decode())
    except configparser.Error:
        return

    for section in cp.sections():
        if section.lower() != "reference":
            continue
        for option in cp.options(section):
            if option.lower().startswith("ref"):
                uri = cp.get(section, option).lower()
                yield fix_asf_uri(uri.strip())


# From https://github.com/kingosticks/mopidy-iheart/blob/master/mopidy_iheart/iheart.py
def parse_new_asx(data):
    # Copied from mopidy.audio.playlists
    try:
        # Last element will be root.
        for _event, element in elementtree.iterparse(io.BytesIO(data)):
            element.tag = element.tag.lower()  # normalize
    except elementtree.ParseError:
        return

    if element:
        for ref in element.findall("entry/ref[@href]"):
            yield fix_asf_uri(ref.get("href", "").strip())

        for entry in element.findall("entry[@href]"):
            yield fix_asf_uri(entry.get("href", "").strip())


# From https://github.com/kingosticks/mopidy-iheart/blob/master/mopidy_iheart/iheart.py
def parse_asx(data):
    if b"asx" in data[0:50].lower():
        return parse_new_asx(data)
    return parse_old_asx(data)


# From https://github.com/kingosticks/mopidy-iheart/blob/master/mopidy_iheart/iheart.py
def find_playlist_parser(extension, content_type):
    extension_map = {
        ".asx": parse_asx,
        ".wax": parse_asx,
        ".m3u": parse_m3u,
        ".pls": parse_pls,
    }
    content_type_map = {
        "video/x-ms-asf": parse_asx,
        "application/x-mpegurl": parse_m3u,
        "audio/x-mpegurl": parse_m3u,
        "audio/x-scpls": parse_pls,
    }

    parser = extension_map.get(extension, None)
    if not parser and content_type:
        # Annoying case where the url gave us no hints so try and work it out
        # from the header's content-type instead.
        # This might turn out to be server-specific...
        parser = content_type_map.get(content_type.lower(), None)
    return parser


def run_iheart_query(session, timeout, uri):
    logging.debug(f"Iheart request: {uri!r}")
    try:
        with closing(session.get(uri, timeout=timeout)) as r:
            r.raise_for_status()
            return r.json()["body"]
    except Exception as e:
        logging.info(f"Iheart API request for {variant} failed: {e}")
    return {}


def run_playlist_query(session, timeout, url):
    data, content_type = None, None
    results = []
    logging.debug(f"Extracting URIs from {url!r}")
    extension = urlparse(url).path[-4:]
    if extension in [".mp3", ".wma"]:
        return [url]  # Catch these easy ones

    try:
        # Defer downloading the body until know it's not a stream
        with closing(session.get(url, timeout=timeout, stream=True)) as r:
            r.raise_for_status()
            content_type = r.headers.get("content-type", "audio/mpeg")
            logging.debug(f"{url} has content-type: {content_type}")
            content_type = content_type.split(";")[0].strip()
            # TODO: review this. There is a case of aac station that hangs in
            # r.content
            if not content_type.startswith("audio/aacp"):
                if content_type != "audio/mpeg" and content_type != "audio/aac":
                    data = r.content

    except Exception as e:
        logging.info(f"Iheart playlist request for {url} failed: {e}")

    if data:
        parser = find_playlist_parser(extension, content_type)
        if parser:
            try:
                results = [u for u in parser(data) if u and u != url]
            except Exception as e:
                logging.error(f"Iheart playlist parsing failed {e}")
            if not results:
                playlist_str = data.decode(errors="ignore")
                logging.debug(f"Parsing failure, malformed playlist: {playlist_str}")
    else:
        results = [url]

    logging.debug(f"Got {results}")
    return list(OrderedDict.fromkeys(results)), content_type


class TizEnumeration(set):
    """A simple enumeration class.

    """

    def __getattr__(self, name):
        if name in self:
            return name
        raise AttributeError


# From https://github.com/kingosticks/mopidy-iheart/blob/master/mopidy_iheart/iheart.py
# with modifications
class Iheart:
    """Wrapper for the Iheart API."""

    def __init__(self, timeout):
        self._base_uri = "http://opml.radiotime.com/%s"
        self._session = requests.Session()
        self._timeout = timeout / 1000.0
        self._stations = {}
        self._last_content_type = ''

    def search(self, search_keywords, tls=True):
        """Returns a dict containing iHeartRadio search results for search_keyword

        """

        # http://api2.iheart.com/api/v1/catalog/searchAll?&keywords=wjr&bestMatch=True&queryStation=True&queryArtist=True&queryTrack=True&queryTalkShow=True&startIndex=0&maxRows=12&queryFeaturedStation=False&queryBundle=False&queryTalkTheme=False&amp_version=4.11.0
        #
        # Analysis of these parameters will be conducted some other time (TM),
        # however it appears only &keywords is necessary; it is the only one
        # this program uses.

        # Base URL for our API request
        iheart_base_url = "%s://api.iheart.com/api/v1/catalog/searchAll?"

        paramaters = urllib.parse.urlencode(
            {"keywords": search_keywords, "startIndex": 0, "maxRows": 250}
        )

        response = urllib.request.urlopen(
            (iheart_base_url % ("https" if tls else "http")) + paramaters
        )
        # We assume we're dealing with UTF-8 encoded JSON, if we aren't the API
        # has probably changed in ways we can't deal with.
        assert (
            response.getheader("Content-Type").casefold()
            == "application/json;charset=UTF-8".casefold()
        )

        results = json.loads(response.read().decode("utf-8"))

        if results["errors"]:
            raise RuntimeError(results["errors"])
        else:
            return results

    def station_info(self, station_id, tls=True):
        """ Returns a dict containing all available information about station_id

        station_id is a five-digit number assigned to an iHeartRadio station.
        No publicly documented method of obtaining a list of valid station_ids
        is currently available, however see the function search in this
        file.
        """

        # The base URL for our API request (%s is for string substitution)
        iheart_base_url = "%s://api.iheart.com/api/v2/content/liveStations/%s"

        response = urllib.request.urlopen(
            iheart_base_url % (("https" if tls else "http"), station_id)
        )

        # We assume we're dealing with UTF-8 encoded JSON, if we aren't the API
        # has probably changed in ways we can't deal with.
        (response.getheader("Content-Type") == "application/json;charset=UTF-8")

        # TODO: investigate this new "hits" structure, perhaps searching and
        #       stream-fetching can be unified
        station = json.loads(response.read().decode("utf-8"))["hits"][0]

        if not station["streams"]:
            raise RuntimeError("station streams list empty")

        return station

    def last_content_type(self):
        return self._last_content_type;

    def station_url(self, station, tls=True):
        """Takes a station dictionary and returns a URL.
        """

        def streamcmp(x, y):
            """By analogy with C strcmp(), this returns >0 if x is more
                preferred than y, >0 if it is less preferred, or 0 if x and y
                are equal.
                """
            # We prefer shoutcast over pls over rtmp (and we prefer no
            # stream at all over stw_stream), and unless otherwise specified
            # we prefer tls ("secure_") variants. Note that we will also
            # never pick an https stream if told not to, even if that means
            # picking no valid stream.
            order = {
                "secure_shoutcast_stream": (3.1 if tls else -1),
                "shoutcast_stream": 3,
                "secure_pls_stream": (2.1 if tls else -1),
                "pls_stream": 2,
                "secure_hls_stream": (2.1 if tls else -1),
                "hls_stream": 2,
                "secure_rtmp_stream": (1.1 if tls else -1),
                "rtmp_stream": 1,
                None: 0,
                # appears to be a normal HLS stream fed through an
                # (HTTPS) redirector hosted by iHeart; since it seems
                # to occur alongside non-redirected HLS streams,
                # prefer those.
                "pivot_hls_stream": (0.2 if tls else -1),
                # one station observed with this stream type (github
                # issue #6), but no URL for it
                "flv_stream": 0.1,
                "stw_stream": -1,
            }

            if order[x] > order[y]:
                return 1
            elif order[x] == order[y]:  # should never happen
                return 0
            elif order[x] < order[y]:
                return -1

        preferred_stream = None
        for stream in list(station["streams"].keys()):
            # stations with blank URLs have been observed
            if (streamcmp(stream, preferred_stream) > 0) and len(
                station["streams"][stream]
            ) > 0:
                preferred_stream = stream

        return station["streams"][preferred_stream]

    def parse_stream_url(self, url):
        playlist_query = MEMORY.cache(run_playlist_query)
        obj, self._last_content_type = playlist_query(self._session, self._timeout, url)
        return obj


class tiziheartproxy(object):
    """A class that accesses Iheart, retrieves station URLs and creates and manages
    a playback queue.

    """

    def __init__(self):
        self.queue = list()
        self.queue_index = -1
        self.unique_names = set()
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.search_modes = TizEnumeration(["ALL", "STATIONS", "SHOWS"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_radio = None
        self.timeout = 5000
        self.iheart = Iheart(self.timeout)

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: current valid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)

    def enqueue_radios(self, arg, keywords1="", keywords2="", keywords3=""):
        """Search Iheart for stations or shows and add them to the playback
        queue.

        :param arg: a search string

        """
        logging.info("enqueue_radios : %s", arg)
        try:
            print_msg("[iHeart] [iHeart search] : '{0}'. ".format(arg))

            count = len(self.queue)
            results = self.iheart.search(arg)
            for s in results["stations"]:
                self._add_to_playback_queue(s)

            remaining_keywords = [keywords1, keywords2, keywords3]
            self._filter_play_queue("Search", remaining_keywords)

            self._finalise_play_queue(count, arg)

        except ValueError:
            raise ValueError(str("No stations found"))

    def current_radio_name(self):
        """ Retrieve the current station's name.

        """
        logging.info("current_radio_name")
        radio = self.now_playing_radio
        name = ""
        if radio and radio.get("name"):
            name = radio["name"]
        return name

    def current_radio_description(self):
        """ Retrieve the current station's description.

        """
        logging.info("current_radio_description")
        radio = self.now_playing_radio
        description = ""
        if radio and radio.get("description"):
            description = radio["description"]
        return description

    def current_radio_city(self):
        """ Retrieve the current station's city.

        """
        logging.info("current_radio_city")
        radio = self.now_playing_radio
        city = ""
        if radio and radio.get("city"):
            city = radio["city"]

        return city

    def current_radio_state(self):
        """ Retrieve the current station's state.

        """
        logging.info("current_radio_state")
        radio = self.now_playing_radio
        state = ""
        if radio and radio.get("state"):
            state = radio["state"]
        return state

    def current_radio_audio_encoding(self):
        """Retrieve the current station's audio encoding (from the http stream
        content-type).

        """
        return self.iheart._last_content_type

    def current_radio_website_url(self):
        """ Retrieve the current station's website url.

        """
        logging.info("current_radio_website_url")
        radio = self.now_playing_radio
        web = ""
        if radio and radio.get("info") and radio["info"].get("website"):
            web = radio["info"]["website"]
        return web

    def current_radio_thumbnail_url(self):
        """ Retrieve the current station's thumbnail url.

        """
        logging.info("current_radio_thumbnail_url")
        radio = self.now_playing_radio
        image = ""
        if radio and radio.get("newlogo"):
            image = radio["newlogo"]
        return image

    def current_radio_queue_index_and_queue_length(self):
        """ Retrieve index in the queue (starting from 1) of the current radio and the
        length of the playback queue.

        """
        return self.play_queue_order[self.queue_index] + 1, len(self.queue)

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1
        self.unique_names = set()
        self.play_queue_order = list()
        self.now_playing_radio = None

    def print_queue(self):
        for i in range(0, len(self.queue)):
            s = self.queue[self.play_queue_order[i]]
            order_num = str("#{:0{}d}".format(i + 1, len(str(len(self.queue)))))
            print_nfo(
                "[iHeart] [station] [{0}] '{1}' [{2}] ({3}, {4}).".format(
                    order_num, s["name"], s["description"], s["city"], s["state"]
                )
            )

        print_nfo("[iHeart] [Stations in queue] '{0}'.".format(len(self.queue)))

    def remove_current_url(self):
        """Remove the currently active url from the playback queue.

        """
        logging.info("remove_current_url")
        if len(self.queue) and self.queue_index >= 0:
            station = self.queue[self.queue_index]
            print_err(
                "[iHeart] '[{0}]' removed from queue.".format(
                    station["name"]
                )
            )
            del self.queue[self.queue_index]
            self.queue_index -= 1
            if self.queue_index < 0:
                self.queue_index = 0
            self._update_play_queue_order()

    def get_url(self, position=None):
        """Retrieve the url on a particular position in the playback queue. If no
        position is given, the url at the current position of the playback is returned.

        """
        logging.info("get_url {}".format(position if position else "-1"))
        try:
            if len(self.queue):
                queue_pos = self.play_queue_order[self.queue_index]
                if position and position > 0 and position <= len(self.queue):
                    self.queue_index = position - 1
                    queue_pos = self.play_queue_order[self.queue_index]
                    logging.info("get_url : self.queue_index {}".format(self.queue_index))
                logging.info(
                    "get_url : play_queue_order {}".format(
                        self.play_queue_order[self.queue_index]
                    )
                )
                return self._retrieve_station_url(queue_pos)
            else:
                return ""
        except (KeyError, AttributeError):
            # TODO: We don't remove this for now
            # del self.queue[self.queue_index]
            logging.info("exception")
            return ""

    def next_url(self):
        """ Retrieve the url of the next station/show in the playback queue.

        """
        logging.info("next_url")
        try:
            if len(self.queue):
                self.queue_index += 1
                if (self.queue_index < len(self.queue)) and (self.queue_index >= 0):
                    return self._retrieve_station_url(
                        self.play_queue_order[self.queue_index]
                    )
                else:
                    self.queue_index = -1
                    return self.next_url()
            else:
                raise RuntimeError("The playback queue is empty!")

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
                if (self.queue_index < len(self.queue)) and (self.queue_index >= 0):
                    return self._retrieve_station_url(
                        self.play_queue_order[self.queue_index]
                    )
                else:
                    self.queue_index = len(self.queue)
                    return self.prev_url()
            else:
                raise RuntimeError("The playback queue is empty!")

        except (KeyError, AttributeError):
            del self.queue[self.queue_index]
            return self.prev_url()

    def _retrieve_station_url(self, station_idx):
        """ Retrieve a station url

        """
        logging.info("_retrieve_station_url")
        try:
            station = self.queue[station_idx]
            info = self.iheart.station_info(station["id"])
            url = self.iheart.station_url(info)
            station_url = None
            name = station["name"]
            urls = self.iheart.parse_stream_url(url)
            if len(urls) > 0:
                station_url = urls[0]
            else:
                station_url = urls
            # Add the url key
            station["streamurl"] = station_url
            station["info"] = info
            self.queue[station_idx] = station
            self.now_playing_radio = station
            return station_url

        except AttributeError:
            logging.info("Could not retrieve the station url!")
            raise
        except Exception as e:
            logging.info(f"Iheart API request failed: {e}")

    def _update_play_queue_order(self):
        """ Update the queue playback order.

        A sequential order is applied if the current play mode is "NORMAL" or a
        random order if current play mode is "SHUFFLE"

        """
        total_stations = len(self.queue)
        if total_stations:
            if len(self.play_queue_order) == 0:
                # Create a sequential play order, if empty
                self.play_queue_order = list(range(total_stations))
            if self.current_play_mode == self.play_modes.SHUFFLE:
                random.shuffle(self.play_queue_order)


    def _add_to_playback_queue(self, s):
        self.queue.append(s)

    def _filter_play_queue(self, category, keyword_list):

        if len(self.queue) == 0 or len(keyword_list) == 0:
            return

        for k in keyword_list:
            phrase = k.rstrip()
            if phrase:
                filtered_queue = list()
                print_adv(
                    "[iHeart] [{0}] Filtering results: '{1}'.".format(category, phrase)
                )
                for item in self.queue:
                    title = item["name"] if item.get("name") else ""
                    title = (
                        title + " " + item["description"]
                        if item.get("description")
                        else ""
                    )
                    if fuzz.partial_ratio(phrase, title) > 50:
                        filtered_queue.append(item)
                if len(filtered_queue) > 0:
                    self.queue = filtered_queue

    def _finalise_play_queue(self, count, arg):
        """ Helper function to grou the various actions needed to ready play
        queue.

        """

        if count == len(self.queue):
            logging.info("no stations found arg : %s", arg)
            raise ValueError
        self._update_play_queue_order()
        self.print_queue()


if __name__ == "__main__":
    tiziheartproxy()

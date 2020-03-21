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
Simple IHeart proxy/wrapper.

Access IHeart to retrieve station URLs and create a play queue for streaming.

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
from collections import OrderedDict
from contextlib import closing
from requests import Session
from urllib.parse import urlparse
from joblib import Memory
from fuzzywuzzy import process
from fuzzywuzzy import fuzz
from functools import reduce
import requests

# FOR REFERENCE

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
    logging.debug(f"IHeart request: {uri!r}")
    try:
        with closing(session.get(uri, timeout=timeout)) as r:
            r.raise_for_status()
            return r.json()["body"]
    except Exception as e:
        logging.info(f"IHeart API request for {variant} failed: {e}")
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
                if content_type != "audio/mpeg":
                    data = r.content

    except Exception as e:
        logging.info(f"IHeart playlist request for {url} failed: {e}")

    if data:
        parser = find_playlist_parser(extension, content_type)
        if parser:
            try:
                results = [u for u in parser(data) if u and u != url]
            except Exception as e:
                logging.error(f"IHeart playlist parsing failed {e}")
            if not results:
                playlist_str = data.decode(errors="ignore")
                logging.debug(f"Parsing failure, malformed playlist: {playlist_str}")
    elif content_type:
        results = [url]

    logging.debug(f"Got {results}")
    return list(OrderedDict.fromkeys(results))


class TizEnumeration(set):
    """A simple enumeration class.

    """

    def __getattr__(self, name):
        if name in self:
            return name
        raise AttributeError


# From https://github.com/kingosticks/mopidy-iheart/blob/master/mopidy_iheart/iheart.py
# with modifications
class IHeart:
    """Wrapper for the IHeart API."""

    def __init__(self, timeout):
        self._base_uri = "http://opml.radiotime.com/%s"
        self._session = requests.Session()
        self._timeout = timeout / 1000.0
        self._stations = {}
        self.nextStationsURL = ""


    def search(search_keywords, tls=True):
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
            {"keywords": search_keywords, "startIndex": 0, "maxRows": 20}
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


    def station_info(station_id, tls=True):
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
        else:
            return station


    def station_url(station, tls=True):
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


class tiziheartproxy(object):
    """A class that accesses IHeart, retrieves station URLs and creates and manages
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
        self.current_search_mode = self.search_modes.ALL
        self.now_playing_radio = None
        self.timeout = 5000
        self.iheart = IHeart(self.timeout)

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: current valid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)

    def set_search_mode(self, mode):
        """ Set the search mode.

        :param mode: current valid values are "ALL", "STATIONS" and "SHOWS"

        """
        self.current_search_mode = getattr(self.search_modes, mode)

    def enqueue_radios(self, arg, keywords1="", keywords2="", keywords3=""):
        """Search IHeart for stations or shows and add them to the playback
        queue.

        :param arg: a search string

        """
        logging.info("enqueue_radios : %s", arg)
        try:
            print_msg("[iHeart] [iHeart search] : '{0}'. ".format(arg))

            count = len(self.queue)
            results = self.iheart.search(arg)
            for s in results['stations']:
                self._add_to_playback_queue(s)

            # remaining_keywords = [keywords1, keywords2, keywords3]
            # self._filter_play_queue("Search", remaining_keywords)

            logging.info(
                "Added {0} stations/shows to queue".format(len(self.queue) - count)
            )

            if count == len(self.queue):
                raise ValueError

            self._update_play_queue_order()

        except ValueError:
            raise ValueError(str("No stations/shows/episodes found"))

    def current_radio_name(self):
        """ Retrieve the current station's or show's name.

        """
        logging.info("current_radio_name")
        radio = self.now_playing_radio
        name = ""
        if radio and radio.get("text"):
            name = radio["text"]
        return name

    def current_radio_description(self):
        """ Retrieve the current station's or show's description.

        """
        logging.info("current_radio_description")
        radio = self.now_playing_radio
        description = ""
        if radio and radio.get("subtext"):
            description = radio["subtext"]
        return description

    def current_radio_type(self):
        """ Retrieve whether the current radio is a station or show.

        """
        logging.info("current_radio_type")
        radio = self.now_playing_radio
        radiotype = ""
        if radio and radio.get("item"):
            radiotype = "podcast" if radio["item"] == "topic" else radio["item"]

        return radiotype

    def current_radio_formats(self):
        """ Retrieve the formats of the current station or show.

        """
        logging.info("current_radio_formats")
        radio = self.now_playing_radio
        formats = ""
        if radio and radio.get("formats"):
            formats = radio["formats"]
        return formats

    def current_radio_bitrate(self):
        """ Retrieve the bitrate of the current station or show.

        """
        logging.info("current_radio_bitrate")
        radio = self.now_playing_radio
        bitrate = ""
        if radio and radio.get("bitrate"):
            bitrate = radio["bitrate"]
        return bitrate

    def current_radio_reliability(self):
        """ Retrieve the reliability of the current station or show.

        """
        logging.info("current_radio_reliability")
        radio = self.now_playing_radio
        reliability = ""
        if radio and radio.get("reliability"):
            reliability = radio["reliability"]
        return reliability

    def current_radio_thumbnail_url(self):
        """ Retrieve the url of the current station's or show's thumbnail image.

        """
        logging.info("current_radio_thumbnail_url")
        radio = self.now_playing_radio
        image = ""
        if radio and radio.get("image"):
            image = radio["image"]
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

    def remove_current_url(self):
        """Remove the currently active url from the playback queue.

        """
        logging.info("remove_current_url")
        if len(self.queue) and self.queue_index >= 0:
            station = self.queue[self.queue_index]
            print_err(
                "[iHeart] '{0} [{1}]' removed from queue.".format(
                    station["text"], station["streamurl"]
                )
            )
            del self.queue[self.queue_index]
            self.queue_index -= 1
            if self.queue_index < 0:
                self.queue_index = 0
            self._update_play_queue_order(verbose=False)

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
            info = self.iheart.station_info(station['id'])
            url = self.iheart.station_url(info)

            name = station["text"]
            streamurls = self.iheart.tune(station)
            print_wrn("[iHeart] Playing '{0}'.".format(name))
            if len(streamurls) > 0:
                urls = self.iheart.parse_stream_url(streamurls[0])
                if len(urls) > 0:
                    station_url = urls[0]
                else:
                    station_url = streamurls[0]
            # Add the url key
            station["streamurl"] = station_url
            self.queue[station_idx] = station
            self.now_playing_radio = station
            return station_url

        except AttributeError:
            logging.info("Could not retrieve the station url!")
            raise
        except Exception as e:
            logging.info(f"IHeart API request failed: {e}")

    def _update_play_queue_order(self, verbose=True):
        """ Update the queue playback order.

        A sequential order is applied if the current play mode is "NORMAL" or a
        random order if current play mode is "SHUFFLE"

        """
        total_stations = len(self.queue)
        if total_stations:
            if not len(self.play_queue_order):
                # Create a sequential play order, if empty
                self.play_queue_order = list(range(total_stations))
            if self.current_search_mode == self.search_modes.STATIONS:
                # order stations by reliability (most reliable first)
                self.queue = sorted(
                    self.queue, key=lambda k: int(k["reliability"]), reverse=True,
                )
            if self.current_search_mode == self.search_modes.SHOWS:
                # order shows by date (newest first)
                self.queue = sorted(
                    self.queue,
                    key=lambda k: datetime.datetime.strptime(k["subtext"], "%d %b %Y"),
                    reverse=True,
                )
            if self.current_play_mode == self.play_modes.SHUFFLE:
                random.shuffle(self.play_queue_order)

        if verbose:
            self._print_play_queue()

        print_nfo("[iHeart] [Items in queue] '{0}'.".format(total_stations))

    def _add_to_playback_queue(self, s):
        self.queue.append(s)

    def _select_one(self, results, keywords, name=""):
        res = None
        res_dict = dict()
        res_names = list()
        for r in results:
            if r["text"] != "By Genre":
                print_nfo("[iHeart] [{0}] '{1}'.".format(name, r["text"]))
                res_names.append(r["text"])
                res_dict[r["text"]] = r

        if not keywords:
            res_name = random.choice(res_names)
            res = res_dict[res_name]
        else:
            if len(res_names) > 1:
                res_name = process.extractOne(keywords, res_names)[0]
                res = res_dict[res_name]
            elif len(res_names) == 1:
                res_name = res_names[0]
                res = res_dict[res_name]

        return res

    def _print_play_queue(self):
        for r in self.queue:
            st_or_pod = r["item"]
            if st_or_pod == "topic":
                st_or_pod = "episode"

            if r.get("formats") and r.get("bitrate"):
                # Make sure we allow only mp3 stations for now
                if "mp3" not in r.get("formats") and "ogg" not in r.get("formats"):
                    logging.info(
                        "Ignoring non-mp3/non-ogg station : {0}".format(r["formats"])
                    )
                    continue

                print_nfo(
                    "[iHeart] [{0}] '{1}' [{2}] ({3}, {4}kbps, reliability: {5}%).".format(
                        st_or_pod,
                        r["text"],
                        r["subtext"],
                        r["formats"],
                        r["bitrate"],
                        r["reliability"],
                    )
                )
            else:
                print_nfo(
                    "[iHeart] [{0}] '{1}' [{2}].".format(
                        st_or_pod, r["text"], r["subtext"]
                    )
                )

    def _ensure_expected_date_format(self, date):

        correct_date = None
        try:
            d = datetime.datetime.strptime(date, "%d %b %Y")
            correct_date = date
        except ValueError:
            logging.debug("Unexpected date format: {0}".format(date))

        if correct_date:
            return correct_date

        try:
            d = datetime.datetime.strptime(date, "%A %b %d")
            correct_date = d.strftime("%d %b {0}".format(NOW.year))
        except ValueError:
            logging.debug("Unexpected date format: {0}".format(date))

        return correct_date

    def _filter_play_queue(self, category, keyword_list):

        if not len(self.queue) or not len(keyword_list):
            return

        for k in keyword_list:
            phrase = k.rstrip()
            if phrase:
                filtered_queue = list()
                print_adv(
                    "[iHeart] [{0}] Filtering results: '{1}'.".format(category, phrase)
                )
                for item in self.queue:
                    title = item["text"] if item.get("text") else ""
                    title = title + " " + item["subtext"] if item.get("subtext") else ""
                    if fuzz.partial_ratio(phrase, title) > 50:
                        filtered_queue.append(item)
                if len(filtered_queue):
                    self.queue = filtered_queue


if __name__ == "__main__":
    tiziheartproxy()

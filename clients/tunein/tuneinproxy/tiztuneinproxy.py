# Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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

"""@package tiztuneinproxy
Simple Tunein proxy/wrapper.

Access Tunein to retrieve station URLs and create a play queue for streaming.

"""

from __future__ import unicode_literals

import configparser
import sys
import os
import io
import re
import json
import logging
import random
import requests
import datetime
import xml.etree.ElementTree as elementtree
from collections import OrderedDict
from contextlib import closing
from requests import Session, exceptions
from urllib.parse import urlparse
from operator import itemgetter
from joblib import Memory
from fuzzywuzzy import process
from fuzzywuzzy import fuzz
from functools import reduce

# FOR REFERENCE
# {'URL': 'http://opml.radiotime.com/Tune.ashx?id=s290003',
#   'bitrate': '128',
#   'element': 'outline',
#   'formats': 'mp3',
#   'genre_id': 'g2754',
#   'guide_id': 's290003',
#   'image': 'http://cdn-profiles.tunein.com/s290003/images/logoq.jpg?t=157607',
#   'item': 'station',
#   'now_playing_id': 's290003',
#   'preset_id': 's290003',
#   'reliability': '100',
#   'subtext': 'Der Beste Musikmix - Gute Laune von der Südpfalz bis nach Köln',
#   'text': 'RPR1.2000er Pop (Germany)',
#   'type': 'audio'},

# For use during debugging
from pprint import pprint

NOW = datetime.datetime.now()
TUNEIN_CACHE_LOCATION = os.path.join(os.getenv("HOME"), ".config/tizonia/tunein-cache")
MEMORY = Memory(TUNEIN_CACHE_LOCATION, verbose=0)

FORMAT = (
    "[%(asctime)s] [%(levelname)5s] [%(thread)d] "
    "[%(module)s:%(funcName)s:%(lineno)d] - %(message)s"
)

logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get("TIZONIA_TUNEINPROXY_DEBUG"):
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

    print_err("[TuneIn] (%s) : %s" % (exception_type.__name__, exception))

    if os.environ.get("TIZONIA_TUNEINPROXY_DEBUG"):
        print_exception(exception_type, exception, traceback)


sys.excepthook = exception_handler


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


def parse_asx(data):
    if b"asx" in data[0:50].lower():
        return parse_new_asx(data)
    return parse_old_asx(data)


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


def run_tunein_query(session, timeout, uri):
    logging.debug(f"TuneIn request: {uri!r}")
    try:
        with closing(session.get(uri, timeout=timeout)) as r:
            r.raise_for_status()
            return r.json()["body"]
    except Exception as e:
        logging.info(f"TuneIn API request for {variant} failed: {e}")
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
        logging.info(f"TuneIn playlist request for {url} failed: {e}")

    if data:
        parser = find_playlist_parser(extension, content_type)
        if parser:
            try:
                results = [u for u in parser(data) if u and u != url]
            except Exception as e:
                logging.error(f"TuneIn playlist parsing failed {e}")
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


class TuneIn:
    """Wrapper for the TuneIn API."""

    ID_PROGRAM = "program"
    ID_STATION = "station"
    ID_GROUP = "group"
    ID_TOPIC = "topic"
    ID_CATEGORY = "category"
    ID_REGION = "region"
    ID_PODCAST = "podcast_category"
    ID_AFFILIATE = "affiliate"
    ID_STREAM = "stream"
    ID_UNKNOWN = "unknown"

    def __init__(self, timeout, filter_=None, session=None):
        self._base_uri = "http://opml.radiotime.com/%s"
        self._session = session or requests.Session()
        self._timeout = timeout / 1000.0
        if filter_ in [TuneIn.ID_PROGRAM, TuneIn.ID_STATION]:
            self._filter = f"&filter={filter_[0]}"
        else:
            self._filter = ""
        self._stations = {}
        self.nextStationsURL = ""

    def reload(self):
        self._stations.clear()
        self._tunein.clear()
        self._get_playlist.clear()

    def _flatten(self, data):
        results = []
        if type(data) is not list:
            non_list_data = data
            list_data = []
            list_data.append(non_list_data)
        else:
            list_data = data
        for item in list_data:
            if "children" in item:
                results.extend(item["children"])
            else:
                results.append(item)
        return results

    def _filter_results(self, data, section_name=None, map_func=None):
        results = []

        def grab_item(item):
            if "guide_id" not in item:
                return
            if map_func:
                station = map_func(item)
            elif item.get("type") == "link":
                results.append(item)
                return
            else:
                station = item
            self._stations[station["guide_id"]] = station
            results.append(station)

        for item in data:
            if section_name is not None:
                section_key = item.get("key", "").lower()
                if section_key.startswith(section_name.lower()):
                    for child in item["children"]:
                        grab_item(child)

            else:
                grab_item(item)

        return results

    def categories(self, category=""):
        if category == "location":
            args = "&id=r0"  # Annoying special case
        elif category == "language":
            args = "&c=lang"
            return []  # TuneIn's API is a mess here, cba
        else:
            args = "&c=" + category

        # Take a copy so we don't modify the cached data
        results = list(self._tunein("Browse.ashx", args))
        if category in ("podcast", "local"):
            # Flatten the results!
            results = self._filter_results(self._flatten(results))
        elif category == "":
            trending = {
                "text": "Trending",
                "key": "trending",
                "type": "link",
                "URL": self._base_uri % "Browse.ashx?c=trending",
            }
            # Filter out the language root category for now
            results = [x for x in results if x["key"] != "language"]
            results.append(trending)
        else:
            results = self._filter_results(results)
        return results

    def locations(self, location):
        args = "&id=" + location
        results = self._tunein("Browse.ashx", args)
        # TODO: Support filters here
        return [x for x in results if x.get("type", "") == "link"]

    def _browse(self, section_name, guide_id):
        args = "&id=" + guide_id
        results = self._tunein("Browse.ashx", args)
        return self._filter_results(results, section_name)

    def _browse_unfiltered(self, guide_id):
        args = "&id=" + guide_id
        return self._tunein("Browse.ashx", args)

    def featured(self, guide_id):
        return self._browse("Featured", guide_id)

    def local(self, guide_id):
        return self._browse("Local", guide_id)

    def stations(self, guide_id):
        return self._browse("Station", guide_id)

    def stations_popular(self, guide_id):
        results = self._browse_unfiltered(guide_id)
        for item in results:
            item_key = item.get("key", "").lower()
            if item_key.startswith("related"):
                for child in item["children"]:
                    child_key = child.get("key", "").lower()
                    if child_key.startswith("popular"):
                        args = "&" + child["URL"].split("?", 2)[1]
                        return self._tunein("Browse.ashx", args)

    def stations_next(self, guide_id):
        if self.nextStationsURL == "":
            results = self._browse_unfiltered(guide_id)
            for item in results:
                item_key = item.get("key", "").lower()
                if item_key.startswith("stations"):
                    for child in item["children"]:
                        child_key = child.get("key", "")
                        if child_key.startswith("nextStations"):
                            args = "&" + child["URL"].split("?", 2)[1]
                            self.nextStationsURL = args
                            return self._tunein("Browse.ashx", args)

        else:
            results = self._tunein("Browse.ashx", self.nextStationsURL)
            for item in results:
                item_key = item.get("key", "")
                if item_key.startswith("nextStations"):
                    args = "&" + item["URL"].split("?", 2)[1]
                    self.nextStationsURL = args
                    return self._tunein("Browse.ashx", args)

    def related(self, guide_id):
        return self._browse("Related", guide_id)

    def shows(self, guide_id):
        return self._browse("Show", guide_id)

    def episodes(self, guide_id):
        args = f"&c=pbrowse&id={guide_id}"
        results = self._tunein("Tune.ashx", args)
        return self._filter_results(results, "Topic")

    def episodes_unfiltered(self, guide_id):
        args = f"&c=pbrowse&id={guide_id}"
        return self._tunein("Tune.ashx", args)

    def _map_listing(self, listing):
        # We've already checked 'guide_id' exists
        url_args = f'Tune.ashx?id={listing["guide_id"]}'
        return {
            "text": listing.get("name", "???"),
            "guide_id": listing["guide_id"],
            "type": "audio",
            "image": listing.get("logo", ""),
            "subtext": listing.get("slogan", ""),
            "URL": self._base_uri % url_args,
        }

    def _station_info(self, station_id):
        logging.debug(f"Fetching info for station {station_id}")
        args = f"&c=composite&detail=listing&id={station_id}"
        results = self._tunein("Describe.ashx", args)
        listings = self._filter_results(results, "Listing", self._map_listing)
        if listings:
            return listings[0]

    def parse_stream_url(self, url):
        playlist_query = MEMORY.cache(run_playlist_query)
        return playlist_query(self._session, self._timeout, url)

    def tune(self, station):
        logging.debug(f'Tuning station id {station["guide_id"]}')
        args = f'&id={station["guide_id"]}'
        stream_uris = []
        for stream in self._tunein("Tune.ashx", args):
            if "url" in stream:
                stream_uris.append(stream["url"])
        if not stream_uris:
            logging.error(f'Failed to tune station id {station["guide_id"]}')
        return list(OrderedDict.fromkeys(stream_uris))

    def station(self, station_id):
        if station_id in self._stations:
            station = self._stations[station_id]
        else:
            station = self._station_info(station_id)
            self._stations["station_id"] = station
        return station

    def search(self, query):
        # "Search.ashx?query=" + query + filterVal
        if not query:
            logging.debug("Empty search query")
            return []
        logging.debug(f"Searching TuneIn for '{query}'")
        args = f"&query={query}{self._filter}"
        search_results = self._tunein("Search.ashx", args)
        results = []
        for item in self._flatten(search_results):
            if item.get("type", "") == "audio":
                # Only return stations
                self._stations[item["guide_id"]] = item
                results.append(item)

        return results

    def _tunein(self, variant, args):
        uri = (self._base_uri % variant) + f"?render=json{args}"
        tunein_query = MEMORY.cache(run_tunein_query)
        return tunein_query(self._session, self._timeout, uri)


class tiztuneinproxy(object):
    """A class that accesses Tunein, retrieves station URLs and creates and manages
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
        self.tunein = TuneIn(self.timeout)

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
        """Search Tunein for stations or shows and add them to the playback
        queue.

        :param arg: a search string

        """
        logging.info("enqueue_radios : %s", arg)
        try:
            print_msg("[TuneIn] [TuneIn 'global' search] : '{0}'. ".format(arg))

            count = len(self.queue)
            results = self.tunein.search(arg)
            for r in results:
                self._add_to_playback_queue(r)

            remaining_keywords = [keywords1, keywords2, keywords3]
            self._filter_play_queue("global", remaining_keywords)

            logging.info(
                "Added {0} stations/shows to queue".format(len(self.queue) - count)
            )

            if count == len(self.queue):
                raise ValueError

            self._update_play_queue_order()

        except ValueError:
            raise ValueError(str("No stations/shows/episodes found"))

    def enqueue_category(self, category, keywords1="", keywords2="", keywords3=""):
        """Search Tunein for a station/show category and add its stations to the
        playback queue.

        :param category: a search string
        :param keywords1: additional keywords
        :param keywords2: additional keywords
        :param keywords3: additional keywords

        """
        logging.info(
            "enqueue_category : %s : 1: %s 2: %s 3: %s",
            category,
            keywords1,
            keywords2,
            keywords3,
        )
        try:
            count = len(self.queue)

            if category == "location":
                self._enqueue_location(keywords1, keywords2, keywords3)
            elif category == "podcasts":
                self._enqueue_podcasts(keywords1, keywords2, keywords3)
            elif category == "trending":
                self._enqueue_trending(keywords1)
            else:
                self._enqueue_category(category, keywords1, keywords2, keywords3)

            logging.info(
                "Added {0} stations/shows to queue".format(len(self.queue) - count)
            )

            if count == len(self.queue):
                raise ValueError

            self._update_play_queue_order()

        except ValueError:
            raise ValueError(
                str(
                    "No stations/shows/episodes found : {0} {1} {2} {3} ".format(
                        category, keywords1, keywords2, keywords3
                    )
                )
            )

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
                "[TuneIn] '{0} [{1}]' removed from queue.".format(
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

    def _enqueue_category(self, category, keywords1="", keywords2="", keywords3=""):
        """Search Tunein's Music category and add its stations to the
        playback queue.

        :param category: the category name
        :param keywords1: additional keywords
        :param keywords2: additional keywords
        :param keywords3: additional keywords

        """
        logging.info(
            "_enqueue_category : %s 1: %s 2: %s 3: %s",
            category,
            keywords1,
            keywords2,
            keywords3,
        )
        print_msg(
            "[TuneIn] [TuneIn '{0}' category search] : '{1}' (additional keywords: {2} {3}). ".format(
                category, keywords1, keywords2, keywords3
            )
        )

        results = self.tunein.categories(category)
        cat = self._select_one(results, keywords1, category)

        if cat:
            print_wrn("[TuneIn] [{0}] Searching '{1}'.".format(category, cat["text"]))

            # Enqueue stations
            if (
                self.current_search_mode == self.search_modes.ALL
                or self.current_search_mode == self.search_modes.STATIONS
            ):
                stations = self.tunein.stations(cat["guide_id"])
                for s in stations:
                    if s["type"] == "audio":
                        self._add_to_playback_queue(s)

                # Enqueue more stations
                next_stations = self.tunein.stations_next(cat["guide_id"])
                if next_stations:
                    for n in next_stations:
                        if n["type"] == "audio":
                            self._add_to_playback_queue(n)

                # Enqueue some popular stations
                popular = self.tunein.stations_popular(cat["guide_id"])
                if popular:
                    for p in popular:
                        if p.get("type") and p["type"] == "audio":
                            self._add_to_playback_queue(p)

                # Try an unfiltered search if nothing returned results so far
                if not len(self.queue):
                    stations = self.tunein._browse_unfiltered(cat["guide_id"])
                    for s in stations:
                        if s.get("type") and s["type"] == "audio":
                            self._add_to_playback_queue(s)

            # Enqueue podcasts
            if (
                self.current_search_mode == self.search_modes.ALL
                or self.current_search_mode == self.search_modes.SHOWS
            ):
                podcasts = self.tunein.shows(cat["guide_id"])
                if podcasts:
                    for p in podcasts:
                        print_wrn(
                            "[TuneIn] [{0}] Searching '{1}' podcast show.".format(
                                category, p["text"]
                            )
                        )
                        guide_id = p["guide_id"]
                        episodes = self.tunein.episodes(guide_id)
                        for e in episodes:
                            if e["type"] == "audio":
                                e["text"] = p["text"] + " - " + e["text"]
                                self._add_to_playback_queue(e)

            remaining_keywords = [keywords2, keywords3]
            self._filter_play_queue(category, remaining_keywords)

    def _enqueue_location(self, keywords1="", keywords2="", keywords3=""):
        """Search Tunein's Location category and add its stations to the
        playback queue.

        :param keywords1: additional keywords
        :param keywords2: additional keywords
        :param keywords3: additional keywords

        """
        logging.info(
            "_enqueue_location : 1: %s 2: %s 3: %s", keywords1, keywords2, keywords3
        )

        print_msg(
            "[TuneIn] [TuneIn 'location' category search] : '{0}' (additional keywords: {1} {2}). ".format(
                keywords1, keywords2, keywords3
            )
        )

        results = self.tunein.categories("location")
        region = self._select_one(results, keywords1, "Region")

        if region:
            print_wrn(
                "[TuneIn] [Region] Selecting stations from '{0}'.".format(
                    region["text"]
                )
            )
            guide_id = region["guide_id"]
            args = "&id=" + guide_id
            results = self.tunein._tunein("Browse.ashx", args)
            country = self._select_one(results, keywords2, "Country")

            if country:
                print_wrn(
                    "[TuneIn] [Country] Selecting stations from '{0}-{1}'.".format(
                        region["text"], country["text"]
                    )
                )
                guide_id = country["guide_id"]
                args = "&id=" + guide_id
                results = self.tunein._tunein("Browse.ashx", args)
                area = self._select_one(results, keywords3, "Area")

                if area.get("type") and area["type"] == "link":
                    print_wrn(
                        "[TuneIn] [Area] Selecting stations from '{0}'.".format(
                            area["text"]
                        )
                    )
                    guide_id = area["guide_id"]
                    args = "&id=" + guide_id
                    area = self.tunein._tunein("Browse.ashx", args)

                args = ""
                for item in self.tunein._flatten(area):
                    item_type = item.get("type", "")
                    if item_type == "audio":
                        self._add_to_playback_queue(item)
                        continue
                    item_key = item.get("key", "")
                    if item_key.startswith("popular"):
                        args = "&" + item["URL"].split("?", 2)[1]
                        break
                    if item_key.startswith("stations"):
                        args = "&" + item["URL"].split("?", 2)[1]
                        break

                while len(self.queue) < 200 and args != "":
                    newargs = args
                    stations = self.tunein._tunein("Browse.ashx", args)
                    for s in stations:
                        if s["type"] == "audio":
                            self._add_to_playback_queue(s)
                        elif s["type"] == "link" and s["key"] == "nextStations":
                            newargs = "&" + s["URL"].split("?", 2)[1]

                    if newargs != args:
                        args = newargs
                    else:
                        break

    def _enqueue_trending(self, keywords1="", keywords2="", keywords3=""):
        """Search Tunein's Music category and add its stations to the
        playback queue.

        :param keywords1: additional keywords
        :param keywords2: additional keywords
        :param keywords3: additional keywords

        """
        logging.info(
            "_enqueue_trending : 1: %s 2: %s 3: %s", keywords1, keywords2, keywords3
        )
        print_msg(
            "[TuneIn] [TuneIn 'trending' category search] : '{0}'. ".format(keywords1)
        )

        category = "trending"
        stations = self.tunein.categories(category)

        if stations:
            for s in stations:
                if s["type"] == "audio":
                    self._add_to_playback_queue(s)

        remaining_keywords = [keywords1, keywords2, keywords3]
        self._filter_play_queue(category, remaining_keywords)

    def _enqueue_podcasts(self, keywords1="", keywords2="", keywords3=""):
        """Search Tunein's Location category and add its stations to the
        playback queue.

        :param keywords1: additional keywords
        :param keywords2: additional keywords
        :param keywords3: additional keywords

        """
        logging.info(
            "_enqueue_podcasts : 1: %s 2: %s 3: %s", keywords1, keywords2, keywords3
        )
        print_msg(
            "[TuneIn] [TuneIn 'podcast' category search] : '{0}' (additional keywords: {1} {2}). ".format(
                keywords1, keywords2, keywords3
            )
        )

        results = self.tunein.categories("podcast")
        topic = self._select_one(results, keywords1, "Topic")

        if topic:
            print_wrn(
                "[TuneIn] [Shows] Selecting podcast topic '{0}'.".format(topic["text"])
            )
            guide_id = topic["guide_id"]
            shows = self.tunein.shows(guide_id)
            show = self._select_one(shows, keywords2, "Podcast")

            if show:
                print_wrn(
                    "[TuneIn] [Podcast] Selecting episodes from '{0}'.".format(
                        show["text"]
                    )
                )
                guide_id = show["guide_id"]
                episodes = self.tunein.episodes(guide_id)

                args = ""
                for item in self.tunein._flatten(episodes):
                    item_type = item.get("type", "")
                    if item_type == "audio":
                        item["text"] = show["text"] + " - " + item["text"]
                        self._add_to_playback_queue(item)
                        continue
                    item_key = item.get("key", "")
                    if item_key.startswith("popular"):
                        args = "&" + item["URL"].split("?", 2)[1]
                        break
                    if item_key.startswith("stations"):
                        args = "&" + item["URL"].split("?", 2)[1]
                        break

                while len(self.queue) < 200 and args != "":
                    newargs = args
                    stations = self.tunein._tunein("Browse.ashx", args)
                    for s in stations:
                        if s["type"] == "audio":
                            item["text"] = show["text"] + " - " + item["text"]
                            self._add_to_playback_queue(s)
                        elif s["type"] == "link" and s["key"] == "nextStations":
                            newargs = "&" + s["URL"].split("?", 2)[1]

                    if newargs != args:
                        args = newargs
                    else:
                        break

                # Try an unfiltered search if nothing returned results so far
                if not len(self.queue):
                    episodes = self.tunein.episodes_unfiltered(guide_id)
                    pprint (episodes)
                    for s in episodes:
                        if s.get("type") and s["type"] == "audio":
                            self._add_to_playback_queue(s)

            remaining_keywords = [keywords3]
            self._filter_play_queue("Podcast", remaining_keywords)

    def _retrieve_station_url(self, station_idx):
        """ Retrieve a station url

        """
        logging.info("_retrieve_station_url")
        try:
            station = self.queue[station_idx]
            station_url = ""
            name = station["text"]
            streamurls = self.tunein.tune(station)
            print_wrn("[TuneIn] Playing '{0}'.".format(name))
            if len(streamurls) > 0:
                urls = self.tunein.parse_stream_url(streamurls[0])
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
            logging.info(f"TuneIn API request failed: {e}")

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

        print_nfo("[TuneIn] [Items in queue] '{0}'.".format(total_stations))

    def _add_to_playback_queue(self, r):

        # Check station uniqueness
        names_len = len(self.unique_names)
        self.unique_names.add(r["text"] + r["subtext"])
        if names_len == len(self.unique_names):
            # Station/show exist in the queue
            logging.info("Ignoring duplicate")
            return

        # Ignore georestricted items
        if r["subtext"].lower().startswith("not available in your country"):
            logging.info("Ignoring georestricted station/show")
            return

        if not r.get("item"):
            r["item"] = "station"
        st_or_pod = r["item"]

        if (
            st_or_pod == "topic"
            and self.current_search_mode == self.search_modes.STATIONS
        ):
            logging.info("Ignoring podcast")
            return

        if (
            st_or_pod == "station"
            and self.current_search_mode == self.search_modes.SHOWS
        ):
            logging.info("Ignoring station")
            return

        if st_or_pod == "topic" and (
            self.current_search_mode == self.search_modes.ALL
            or self.current_search_mode == self.search_modes.SHOWS
        ):
            new_date = self._ensure_expected_date_format(r["subtext"])
            if not new_date:
                logging.info(
                    "Ignoring podcast with unknown date format : {0}".format(new_date)
                )
                return
            r["subtext"] = new_date

        self.queue.append(r)

    def _select_one(self, results, keywords, name=""):
        res = None
        res_dict = dict()
        res_names = list()
        for r in results:
            if r["text"] != "By Genre":
                print_nfo("[TuneIn] [{0}] '{1}'.".format(name, r["text"]))
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
                    "[TuneIn] [{0}] '{1}' [{2}] ({3}, {4}kbps, reliability: {5}%).".format(
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
                    "[TuneIn] [{0}] '{1}' [{2}].".format(
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
                print_wrn(
                    "[TuneIn] [{0}] Filtering results: '{1}'.".format(
                        category, phrase
                    )
                )
                for item in self.queue:
                    title = item["text"] if item.get("text") else ""
                    title = title + " " + item["subtext"] if item.get("subtext") else ""
                    if fuzz.partial_ratio(phrase, title) > 50:
                        filtered_queue.append(item)
                if len(filtered_queue):
                    self.queue = filtered_queue


if __name__ == "__main__":
    tiztuneinproxy()

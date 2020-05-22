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

"""@package tizyoutubeproxy
Simple YouTube proxy/wrapper.

Access YouTube to retrieve audio stream URLs and create a playback queue.

"""

import sys
import os
import shutil
import time
import logging
import random
import unicodedata
import re
import getpass
import pafy
import configparser
from multiprocessing.dummy import Process, Queue
from joblib import Memory
from fuzzywuzzy import process
from fuzzywuzzy import fuzz


TMPDIR = "/var/tmp"
CACHE_DIR_PREFIX = os.getenv("SNAP_USER_COMMON") or TMPDIR

YOUTUBE_CACHE_LOCATION = os.path.join(
    CACHE_DIR_PREFIX, "tizonia-" + getpass.getuser() + "-youtube"
)

# One week
YOUTUBE_CACHE_TTL = 604800

if os.path.exists(YOUTUBE_CACHE_LOCATION):
    try:
        with open(YOUTUBE_CACHE_LOCATION + "/.timestamp", "r") as tstamp:
            creation_time = tstamp.read()
            # print("Timestamp in the cache directory {0}".format(creation_time))
            tstamp.close()
            now_time = int(time.time())
            if now_time > int(creation_time) + TTL:
                # print("Cache too old. Deleting cache {0}".format(now_time))
                shutil.rmtree(YOUTUBE_CACHE_LOCATION)
    except Exception:
        # No timestamp in the cache directory, clean it up entirely
        # print("No timestamp in the cache directory, clean it up entirely")
        shutil.rmtree(YOUTUBE_CACHE_LOCATION)

if not os.path.exists(YOUTUBE_CACHE_LOCATION):
    # print("No timestamp directory, creating it")
    os.mkdir(YOUTUBE_CACHE_LOCATION)
    try:
        with open(YOUTUBE_CACHE_LOCATION + "/.timestamp", "a") as tstamp:
            epoch_time = str(int(time.time()))
            tstamp.write(epoch_time + "\n")
            tstamp.close()
            # print("Written timestamp {0}".format(epoch_time))
    except Exception:
        pass

MEMORY = Memory(YOUTUBE_CACHE_LOCATION, compress=9, verbose=0, bytes_limit=10485760)
MEMORY.reduce_size()

API_KEY = "AIzaSyC3iXWKvY6iOC3SoglRAekDXqutQiMuCxc"

NOT_UTF8_ENVIRONMENT = "UTF-8" not in os.environ.get("LANG", "")

WORKER_PROCESSES = 4

STREAM_OBJECT_ACQUISITION_MAX_ATTEMPTS = 5

FORMAT = (
    "[%(asctime)s] [%(levelname)5s] [%(thread)d] "
    "[%(module)s:%(funcName)s:%(lineno)d] - %(message)s"
)

logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get("TIZONIA_YOUTUBEPROXY_DEBUG"):
    logging.basicConfig(format=FORMAT)
    from traceback import print_exception
    from pprint import pprint
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

# This code is here for debugging purposes
def utf8_replace(txt):
    """ Replace unsupported characters in unicode string, returns unicode. """
    sse = sys.stdout.encoding
    txt = txt.encode(sse, "replace").decode("utf8", "ignore")
    return txt


# This code is here for debugging purposes
# def xenc(stuff):
#     """ Replace unsupported characters. """
#     if sys.stdout.isatty():
#         return utf8_replace(stuff) if NOT_UTF8_ENVIRONMENT else stuff

#     else:
#         return stuff.encode("utf8", errors="replace")


# This code is here for debugging purposes
# def xprint(stuff, end=None):
#     """ Compatible print. """
#     print(xenc(stuff), end=end)


# This code is here for debugging purposes
# def dump_stream_info(streams):
#     """ Dump stream info. """

#     fstring = "{0:<7}{1:<8}{2:<7}{3:<15}{4:<10}       "
#     out = []
#     length = len(streams)
#     text = " [Fetching stream info]      >"

#     for num, stream in enumerate(streams):
#         sys.stdout.write(text + "-" * num + ">" + " " * (length - num - 1) + "<\r")
#         sys.stdout.flush()
#         megs = "%3.f" % (stream.get_filesize() / 1024 ** 2) + " MB"
#         qual = "[%s]" % stream.quality
#         out.append(
#             fstring.format(num + 1, stream.mediatype, stream.extension, qual, megs)
#         )

#     sys.stdout.write("\r")
#     xprint(fstring.format("Stream", "Type", "Format", "Quality", " Size"))
#     xprint(fstring.format("------", "----", "------", "-------", " ----"))

#     for line in out:
#         xprint(line)


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
    if "The uploader has not made this video available" in str(exception):
        if os.environ.get("TIZONIA_YOUTUBEPROXY_DEBUG"):
            print_err("[YouTube] (%s) : %s" % (exception_type.__name__, exception))
    else:
        print_err("[YouTube] (%s) : %s" % (exception_type.__name__, exception))

    if os.environ.get("TIZONIA_YOUTUBEPROXY_DEBUG"):
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


def get_track_id_from_json(item):
    """ Try to extract a video Id from a pafy query response """
    fields = [
        "contentDetails/videoId",
        "snippet/resourceId/videoId",
        "id/videoId",
        "id",
    ]
    for field in fields:
        node = item
        for part in field.split("/"):
            if node and isinstance(node, dict):
                node = node.get(part)
        if node:
            # Make sure that what we are returning is a string
            if isinstance(node, str):
                return node
    return ""


def get_tracks_from_json(jsons, howmany=0):
    """ Get search results from pafy's call_gdata response

        :param jsons: The result of calling pafy.call_gdata('search', query)
        :param howmany: The maximum number of tracks to retrieve (0 means, no limit)

    """

    items = jsons.get("items")
    if not items:
        logging.info("got unexpected data or no search results")
        return ()

    # fetch detailed information about items from videos API
    query_string = {
        "part": "contentDetails,statistics,snippet",
        "id": ",".join([get_track_id_from_json(i) for i in items]),
    }

    yt_dt_search = MEMORY.cache(run_youtube_data_search)
    wdata = yt_dt_search("videos", query_string)

    items_vidinfo = wdata.get("items", [])
    # enhance search results by adding information from videos API response
    for searchresult, vidinfoitem in zip(items, items_vidinfo):
        searchresult.update(vidinfoitem)

    # populate list of video objects
    songs = []
    for item in items:

        try:

            ytid = get_track_id_from_json(item)
            snippet = item.get("snippet", {})
            title = snippet.get("title", "").strip()
            info = VideoInfo(ytid=ytid, title=title)

        except Exception as exception:

            logging.info(
                "Error during metadata extraction/instantiation of "
                + "search result {}\n{}".format(ytid, exception)
            )

        songs.append(info)
        if howmany != 0 and len(songs) == howmany:
            break

    # return video objects
    return songs


def generate_search_query(term, api_key):
    """ Return the query string for pafy's call_gdata. """

    query_string = {
        "q": term,
        "maxResults": 50,
        "safeSearch": "none",
        "order": "relevance",
        "part": "id,snippet",
        "type": "video",
        "videoDuration": "any",
        "key": api_key if api_key != "" else API_KEY,
        "videoCategoryId": 10,  # search music
    }

    return query_string


def run_youtube_search(arg):
    return pafy.new(arg)


def run_youtube_data_search(typestr, query):
    return pafy.call_gdata(typestr, query)


def run_youtube_playlist_search(arg):
    return pafy.get_playlist2(arg)


def run_youtube_channel_search(arg):
    return pafy.get_channel(arg)


def obtain_stream(inqueue, outqueue):
    """ Return the stream object after instantiating the pafy object. """

    for stream in iter(inqueue.get, "STOP"):
        x = 0
        audioFound = False
        while not audioFound and x < STREAM_OBJECT_ACQUISITION_MAX_ATTEMPTS:
            x += 1
            try:
                logging.info("index     : %d", stream["q"])
                if not stream.get("v") or not stream.get("a"):
                    logging.info("ytid : %s", stream["i"].ytid)
                    video = stream.get("v")
                    if not video:
                        yt_search = MEMORY.cache(run_youtube_search)
                        video = yt_search(stream["i"].ytid)
                    audio = video.getbestaudio(preftype="webm")
                    if not audio:
                        logging.info("no suitable audio found")
                        continue
                    stream.update({"a": audio, "v": video})

                # streams = stream.get('v').audiostreams[::-1]
                # pprint.pprint(streams)
                # dump_stream_info(streams)

                logging.info("index     : %d", stream["q"])
                logging.info("url       : %s", stream["a"].url)
                logging.info("title     : %s", to_ascii(stream["a"].title))
                logging.info("bitrate   : %s", stream["a"].bitrate)
                logging.info("extension : %s", stream["a"].extension)
                outqueue.put(stream)
                audioFound = True

            except IOError as e:
                if "The uploader has not made this video available" not in str(e):
                    logging.error(
                        "[YouTube] Could not retrieve the audio stream URL for '{}' "
                        "(Attempt {} of {}).".format(
                            to_ascii(stream["i"].ytid),
                            x,
                            STREAM_OBJECT_ACQUISITION_MAX_ATTEMPTS,
                        )
                    )
                else:
                    break


class VideoInfo(object):
    """ Class to represent a YouTube video in the queue.

    """

    def __init__(self, ytid, title):
        """ class members. """
        self.ytid = ytid
        self.title = title


class tizyoutubeproxy(object):
    """A class that accesses YouTube, retrieves stream URLs and creates and manages
    a playback queue.

    """

    def __init__(self, api_key=API_KEY):
        self.queue = list()
        self.queue_index = -1
        self.play_queue_order = list()
        self.play_modes = TizEnumeration(["NORMAL", "SHUFFLE"])
        self.current_play_mode = self.play_modes.NORMAL
        self.now_playing_stream = None
        # Create multiprocess queues
        self.task_queue = Queue()
        self.done_queue = Queue()
        # Workers
        self.workers = list()
        self.api_key = api_key if api_key != "" else API_KEY
        pafy.set_api_key(self.api_key)

    def set_play_mode(self, mode):
        """ Set the playback mode.

        :param mode: current valid values are "NORMAL" and "SHUFFLE"

        """
        self.current_play_mode = getattr(self.play_modes, mode)
        self._update_play_queue_order()

    def enqueue_audio_stream(self, arg):
        """Add the audio stream of a YouTube video to the
        playback queue.

        :param arg: a search string

        """
        logging.info("arg : %s", arg)
        try:
            print_msg("[YouTube] [Audio stream] : '{0}'. ".format(arg))
            self._enqueue_audio_stream(arg)
            self._finalise_play_queue(0, arg)

        except ValueError:
            raise ValueError(str("Video not found : %s" % arg))

    def enqueue_audio_playlist(self, arg):
        """Add all audio streams in a YouTube playlist to the playback queue.

        :param arg: a YouTube playlist id

        """
        logging.info("arg : %s", arg)
        try:
            print_msg("[YouTube] [Audio playlist] : '{0}'. ".format(arg))
            count = len(self.queue)

            yt_pl_search = MEMORY.cache(run_youtube_playlist_search)
            playlist = yt_pl_search(arg)

            if len(playlist) > 0:
                for yt_video in playlist:
                    self._add_to_playback_queue(
                        video=yt_video,
                        info=VideoInfo(ytid=yt_video.videoid, title=yt_video.title),
                    )

            self._finalise_play_queue(count, arg)

        except ValueError:
            raise ValueError(str("Playlist not found : %s" % arg))

    def enqueue_audio_search(self, arg):
        """Search YouTube and add the audio streams to the
        playback queue.

        :param arg: a search string

        """
        logging.info("arg : %s", arg)
        try:
            print_msg("[YouTube] [Audio search] : '{0}'. ".format(arg))
            yt_dt_search = MEMORY.cache(run_youtube_data_search)
            query = generate_search_query(arg, self.api_key)
            wdata = yt_dt_search("search", query)

            wdata2 = wdata
            count = 0
            while True:
                for track_info in get_tracks_from_json(wdata2):
                    self._add_to_playback_queue(info=track_info)
                    count += 1

                if count > 100:
                    break
                if not wdata2.get("nextPageToken"):
                    break
                query["pageToken"] = wdata2["nextPageToken"]
                wdata2 = yt_dt_search("search", query)

            self._finalise_play_queue(0, arg)

        except ValueError:
            raise ValueError(str("Could not find any audio streams : %s" % arg))

    def enqueue_audio_mix(self, arg, feelinglucky=True):
        """Obtain a YouTube mix associated to a given video id or url and add all audio
        streams in the mix playlist to the playback queue.

        :param arg: a YouTube video id

        :param feelinglucky: If True, it will perform another YouTube search to find
        alternatives if the original mix cannot be found.

        """
        logging.info("arg : %s", arg)
        yt_video = None
        try:
            print_msg("[YouTube] [Audio mix] : '{0}'. ".format(arg))
            count = len(self.queue)

            yt_search = MEMORY.cache(run_youtube_search)
            yt_video = yt_search(arg)
            playlist = yt_video.mix
            if len(playlist) > 0:
                for yt_video in playlist:
                    video_id = yt_video.videoid
                    video_title = yt_video.title
                    yt_info = VideoInfo(ytid=video_id, title=video_title)
                    self._add_to_playback_queue(video=yt_video, info=yt_info)

            self._finalise_play_queue(count, arg, deduplicate=True)

        except IndexError:
            if not feelinglucky:
                raise ValueError
            else:
                print_adv(
                    "[YouTube] Could not find a mix for '{0}'. "
                    "Searching YouTube instead. Feeling lucky?.".format(
                        arg.encode("utf-8")
                    )
                )
                if yt_video.title:
                    self.enqueue_audio_search(yt_video.title)
                else:
                    self.enqueue_audio_stream(arg)

    def enqueue_audio_mix_search(self, arg):
        """Obtain a YouTube mix associated to a given textual search and add all the
        audio streams in the mix playlist to the playback queue.

        :param arg: a search string

        """
        logging.info("arg : %s", arg)
        try:
            print_msg("[YouTube] [Audio mix search] : '{0}'. ".format(arg))
            yt_dt_search = MEMORY.cache(run_youtube_data_search)
            wdata = yt_dt_search("search", generate_search_query(arg, self.api_key))

            wdata2 = wdata
            count = len(self.queue)
            for track_info in get_tracks_from_json(wdata2):
                if track_info and track_info.ytid:
                    try:
                        # Sometimes, the mix search does not render a playlist
                        # where the seed stream is found.  We force here that
                        # the stream that we are using to seed the search is in
                        # the queue, at least once.
                        self._enqueue_audio_stream(track_info.ytid)
                        self.enqueue_audio_mix(track_info.ytid, feelinglucky=False)
                        break
                    except ValueError:
                        logging.info("Could not find a mix. Trying another video")

            if count == len(self.queue):
                raise ValueError

        except ValueError:
            raise ValueError(str("Could not find any mixes : %s" % arg))

    def enqueue_audio_channel_uploads(self, arg):
        """Add all audio streams in a YouTube channel to the playback queue.

        :param arg: a YouTube channel url

        """
        logging.info("arg : %s", arg)
        try:
            print_msg("[YouTube] [Audio channel uploads] : '{0}'. ".format(arg))
            count = len(self.queue)

            yt_ch_search = MEMORY.cache(run_youtube_channel_search)
            channel = yt_ch_search(arg)
            if channel:
                for yt_video in channel.uploads:
                    self._add_to_playback_queue(
                        video=yt_video,
                        info=VideoInfo(ytid=yt_video.videoid, title=yt_video.title),
                    )

            self._finalise_play_queue(count, arg)

        except (ValueError, KeyError):
            raise ValueError(str("Channel not found : %s" % arg))

    def enqueue_audio_channel_playlist(self, channel_name, playlist_name):
        """Search a playlist within a channel and if found, adds all the audio streams
        to the playback queue.

        :param arg: a YouTube playlist id

        """
        logging.info("args : %s - %s", channel_name, playlist_name)
        try:
            print_msg(
                "[YouTube] [Audio channel playlist] : '{0} - {1}'. ".format(
                    channel_name, playlist_name
                )
            )
            count = len(self.queue)
            yt_ch_search = MEMORY.cache(run_youtube_channel_search)
            channel = yt_ch_search(channel_name)

            if channel:
                pl_dict = dict()
                pl_titles = list()
                pl_name = ""
                playlist = None
                for pl in channel.playlists:
                    print_nfo("[YouTube] [Playlist] '{0}'.".format(to_ascii(pl.title)))
                    if fuzz.partial_ratio(playlist_name, pl.title) > 50:
                        pl_dict[pl.title] = pl
                        pl_titles.append(pl.title)

                if len(pl_titles) > 1:
                    pl_name = process.extractOne(playlist_name, pl_titles)[0]
                    playlist = pl_dict[pl_name]
                elif len(pl_titles) == 1:
                    pl_name = pl_titles[0]
                    playlist = pl_dict[pl_name]

                if pl_name:
                    if pl_name.lower() != playlist_name.lower():
                        print_adv(
                            "[YouTube] Playlist '{0}' not found. "
                            "Playing '{1}' instead.".format(
                                to_ascii(playlist_name), to_ascii(pl_name)
                            )
                        )
                    for yt_video in playlist:
                        self._add_to_playback_queue(
                            video=yt_video,
                            info=VideoInfo(ytid=yt_video.videoid, title=yt_video.title),
                        )

            self._finalise_play_queue(count, channel_name)

        except ValueError:
            raise ValueError(str("Channel not found : %s" % channel_name))

    def current_audio_stream_title(self):
        """ Retrieve the current stream's title.

        """
        stream = self.now_playing_stream
        title = ""
        if stream:
            title = to_ascii(stream["a"].title)
        return title

    def current_audio_stream_author(self):
        """ Retrieve the current stream's author.

        """
        stream = self.now_playing_stream
        author = ""
        if stream:
            author = to_ascii(stream["v"].author)
        return author

    def current_audio_stream_file_size(self):
        """ Retrieve the current stream's file size.

        """
        stream = self.now_playing_stream
        size = 0
        if stream:
            size = stream["a"].get_filesize()
        return size

    def current_audio_stream_duration(self):
        """ Retrieve the current stream's duration.

        """
        stream = self.now_playing_stream
        duration = ""
        if stream:
            duration = to_ascii(stream["v"].duration)
        return duration

    def current_audio_stream_bitrate(self):
        """ Retrieve the current stream's bitrate.

        """
        stream = self.now_playing_stream
        bitrate = ""
        if stream:
            bitrate = stream["a"].bitrate
        return bitrate

    def current_audio_stream_view_count(self):
        """ Retrieve the current stream's view count.

        """
        stream = self.now_playing_stream
        viewcount = 0
        if stream:
            viewcount = stream["v"].viewcount
        return viewcount

    def current_audio_stream_description(self):
        """ Retrieve the current stream's description.

        """
        stream = self.now_playing_stream
        description = ""
        if stream:
            description = to_ascii(stream["v"].description)
        return description

    def current_audio_stream_file_extension(self):
        """ Retrieve the current stream's file extension.

        """
        stream = self.now_playing_stream
        file_extension = ""
        if stream:
            file_extension = to_ascii(stream["a"].extension)
        return file_extension

    def current_audio_stream_video_id(self):
        """ Retrieve the current stream's video id.

        """
        stream = self.now_playing_stream
        video_id = ""
        if stream:
            video_id = to_ascii(stream["i"].ytid)
        return video_id

    def current_audio_stream_published(self):
        """ Retrieve the current stream's upload date and time.

        """
        stream = self.now_playing_stream
        if stream:
            published = to_ascii(stream["v"].published)
        return published

    def current_audio_stream_queue_index_and_queue_length(self):
        """ Retrieve index in the queue (starting from 1) of the current stream and the
        length of the playback queue.

        """
        logging.info("current_audio_track_queue_index_and_queue_length")
        return self.play_queue_order[self.queue_index] + 1, len(self.queue)

    def clear_queue(self):
        """ Clears the playback queue.

        """
        self.queue = list()
        self.queue_index = -1

    def print_queue(self):
        """ Print the contents of the playback queue.

        """

        for i in range(0, len(self.queue)):
            stream = self.queue[self.play_queue_order[i]]
            order_num = str("#{:0{}d}".format(i + 1, len(str(len(self.queue)))))
            title = stream["a"].title if stream.get("a") else stream["i"].title
            info_str = str(
                "[YouTube] [Stream] [{0}] '{1}'".format(
                    order_num, to_ascii(to_ascii(title))
                )
            )
            print_nfo(info_str + ".")

        print_nfo("[YouTube] [Streams in queue] '{0}'.".format(len(self.queue)))

    def remove_current_url(self):
        """Remove the currently active url from the playback queue.

        """
        logging.info("")
        if len(self.queue) and self.queue_index:
            stream = self.queue[self.queue_index]
            print_nfo(
                "[YouTube] [Stream] '{0}' removed.".format(to_ascii(stream["i"].title))
            )
            del self.queue[self.queue_index]
            self.queue_index -= 1
            if self.queue_index < 0:
                self.queue_index = 0
            self._update_play_queue_order()

    def next_url(self):
        """ Retrieve the url of the next stream in the playback queue.

        """
        logging.info("self.queue_index {}".format(self.queue_index))
        try:
            if len(self.queue):
                self.queue_index += 1
                if (self.queue_index < len(self.queue)) and (self.queue_index >= 0):
                    next_stream = self.queue[self.play_queue_order[self.queue_index]]
                    return self._retrieve_stream_url(
                        next_stream, self.play_queue_order[self.queue_index]
                    ).rstrip()
                else:
                    logging.info("self.queue_index {}".format(self.queue_index))
                    self.queue_index = -1
                    return self.next_url()
            else:
                return ""
        except (KeyError, AttributeError):
            # TODO: We don't remove this for now
            # del self.queue[self.queue_index]
            logging.info("KeyError, or AttributeError exception")
            return self.next_url()
        except (IOError):
            # Remove this video
            del self.queue[self.queue_index]
            logging.info("IOError exception")
            return self.next_url()

    def prev_url(self):
        """ Retrieve the url of the previous stream in the playback queue.

        """
        logging.info("")
        try:
            if len(self.queue):
                self.queue_index -= 1
                if (self.queue_index < len(self.queue)) and (self.queue_index >= 0):
                    prev_stream = self.queue[self.play_queue_order[self.queue_index]]
                    return self._retrieve_stream_url(
                        prev_stream, self.play_queue_order[self.queue_index]
                    ).rstrip()
                else:
                    self.queue_index = len(self.queue)
                    return self.prev_url()
            else:
                return ""
        except (KeyError, AttributeError):
            # TODO: We don't remove this for now
            # del self.queue[self.queue_index]
            logging.info("exception")
            return self.prev_url()
        except (IOError):
            # Remove this video
            del self.queue[self.queue_index]
            logging.info("IOError exception")
            return self.next_url()

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
                    logging.info(
                        "get_url : self.queue_index {}".format(self.queue_index)
                    )
                logging.info(
                    "get_url : play_queue_order {}".format(
                        self.play_queue_order[self.queue_index]
                    )
                )
                stream = self.queue[queue_pos]
                return self._retrieve_stream_url(stream, queue_pos)
            else:
                return ""
        except (KeyError, AttributeError):
            # TODO: We don't remove this for now
            # del self.queue[self.queue_index]
            logging.info("exception")
            return ""

    def _enqueue_audio_stream(self, arg):
        """Add the audio stream of a YouTube video to the
        playback queue.

        :param arg: a search string

        """
        logging.info("arg : %s", arg)
        try:
            yt_search = MEMORY.cache(run_youtube_search)
            yt_video = yt_search(arg)
            yt_audio = yt_video.getbestaudio(preftype="webm")
            if not yt_audio:
                raise ValueError(str("No WebM audio stream for : %s" % arg))

            yt_info = VideoInfo(ytid=arg, title=yt_audio.title)
            self._add_to_playback_queue(audio=yt_audio, video=yt_video, info=yt_info)

        except ValueError:
            raise ValueError(str("Video not found : %s" % arg))

    def _update_play_queue_order(self):
        """ Update the queue playback order.

        A sequential order is applied if the current play mode is "NORMAL" or a
        random order if current play mode is "SHUFFLE"

        """
        total_streams = len(self.queue)
        if total_streams:
            if not len(self.play_queue_order):
                # Create a sequential play order, if empty
                self.play_queue_order = list(range(total_streams))
            if self.current_play_mode == self.play_modes.SHUFFLE:
                random.shuffle(self.play_queue_order)

    def _retrieve_stream_url(self, stream, queue_index):
        """ Retrieve a stream url

        """
        logging.info("")
        try:
            if not len(self.workers):
                for _ in range(WORKER_PROCESSES):
                    proc = Process(
                        target=obtain_stream, args=(self.task_queue, self.done_queue)
                    ).start()
                    self.workers.append(proc)

            while not self.done_queue.empty():
                stream = self.done_queue.get()
                self.queue[stream["q"]] = stream

            stream = self.queue[queue_index]
            if not stream.get("v") or not stream.get("a"):
                logging.info("ytid : %s", stream["i"].ytid)
                video = stream.get("v")
                if not video:
                    yt_search = MEMORY.cache(run_youtube_search)
                    video = yt_search(stream["i"].ytid)
                audio = video.getbestaudio(preftype="webm")
                if not audio:
                    logging.info("no suitable audio found")
                    raise AttributeError()
                stream.update({"a": audio, "v": video})

            # streams = stream.get('v').audiostreams[::-1]
            # pprint.pprint(streams)
            # dump_stream_info(streams)

            self.now_playing_stream = stream
            return stream["a"].url

        except AttributeError:
            logging.info("Could not retrieve the stream url!")
            raise

    def _add_to_playback_queue(self, audio=None, video=None, info=None):
        """ Add to the playback queue. """

        queue_index = len(self.queue)
        self.task_queue.put(dict(a=audio, v=video, i=info, q=queue_index))
        self.queue.append(dict(a=audio, v=video, i=info, q=queue_index))

    def _finalise_play_queue(self, count, arg, deduplicate=False):
        """ Helper function to grou the various actions needed to ready play
        queue.

        """

        if deduplicate:
            seen = set()
            uniq = []
            for item in self.queue:
                ytid = item["i"].ytid
                if ytid not in seen:
                    uniq.append(item)
                    seen.add(ytid)
            self.queue = uniq

        if count == len(self.queue):
            logging.info("no tracks found arg : %s", arg)
            raise ValueError
        self._update_play_queue_order()
        self.print_queue()


if __name__ == "__main__":
    tizyoutubeproxy()

# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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

"""@package tizchromecastproxy
Simple Chromecast proxy/wrapper.

Access a Chromecast device to initiate and manage audio streaming sessions..

"""

import socket
import select
import time
import sys
import os
import logging
import unicodedata
import pychromecast
import configparser
from pychromecast.controllers.media import (
    STREAM_TYPE_UNKNOWN,
    STREAM_TYPE_BUFFERED,
    STREAM_TYPE_LIVE,
    MEDIA_PLAYER_STATE_PLAYING,
    MEDIA_PLAYER_STATE_BUFFERING,
    MEDIA_PLAYER_STATE_PAUSED,
    MEDIA_PLAYER_STATE_IDLE,
    MEDIA_PLAYER_STATE_UNKNOWN,
)
from pychromecast.error import PyChromecastError
from pychromecast.config import APP_MEDIA_RECEIVER

# For use during debugging
# from pprint import pprint

DEFAULT_THUMB = "https://avatars0.githubusercontent.com/u/3161606?v=3&s=400"
FORMAT = (
    "[%(asctime)s] [%(levelname)5s] [%(thread)d] "
    "[%(module)s:%(funcName)s:%(lineno)d] - %(message)s"
)

logging.captureWarnings(True)
logging.getLogger().setLevel(logging.DEBUG)

if os.environ.get("TIZONIA_CHROMECASTPROXY_DEBUG"):
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
    print((color + msg + _Colors.ENDC))


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

    print_err("[Chromecast] (%s) : %s" % (exception_type.__name__, exception))
    if os.environ.get("TIZONIA_CHROMECASTPROXY_DEBUG"):
        print_exception(exception_type, exception, traceback)


sys.excepthook = exception_handler


def to_ascii(msg):
    """Unicode to ascii helper.

    """

    if sys.version[0] == "2":
        return unicodedata.normalize("NFKD", str(msg)).encode("ASCII", "ignore")
    return msg


def ensure_ip_addr(ip_or_name):
    """Helper function to obtain an ip address if the argument is a hostname

    """

    try:
        socket.inet_aton(ip_or_name)
        # A valid ip
        return ip_or_name
    except socket.error:
        # Not a valid ip
        pass
    try:
        ip = socket.gethostbyname(ip_or_name)
        # A valid hostname
        return ip
    except socket.gaierror:
        # Not a valid hostname
        pass
    chromecasts = pychromecast.get_chromecasts()
    for ch in chromecasts:
        if ip_or_name.lower() in ch.name.lower():
            return ch.uri.split(":", 1)[0]

    return ip_or_name


class tizchromecastproxy(object):
    """A class that interfaces with a Chromecast device to initiate and manage
    audio streaming sessions.

    """

    def __init__(self, name_or_ip):
        self.ip_addr = ensure_ip_addr(name_or_ip)
        self.active = False
        self.cast = None
        self.cast_status_listener = None
        self.media_status_listener = None

    def _do_activate(self):
        self.cast = pychromecast.Chromecast(self.ip_addr, timeout=0.5, blocking=False)
        self.cast.register_status_listener(self)
        self.cast.media_controller.register_status_listener(self)
        self.cast.start()
        self.cast.wait()
        if self.cast.media_controller.status.player_is_playing:
            try:
                self.cast.media_controller.stop()
                self.cast.quit_app()
            except Exception as e:
                exc_type, value, traceback = sys.exc_info()
                print_exception(exc_type, value, traceback)

    def activate(self, cast_status_listener, media_status_listener):
        print_nfo("[Chromecast] [{0}] [Activating]".format(to_ascii(self.ip_addr)))
        self.cast_status_listener = cast_status_listener
        self.media_status_listener = media_status_listener
        self._do_activate()
        self.active = True
        print_wrn("[Chromecast] [{0}] [Active]".format(to_ascii(self.ip_addr)))

    def deactivate(self):
        self.active = False
        try:
            self.cast.quit_app()
        except Exception as e:
            exc_type, value, traceback = sys.exc_info()
            print_exception(exc_type, value, traceback)

    def poll_socket(self, polltime_ms):
        print_nfo(
            "[Chromecast] [{0}] [poll_socket start]".format(to_ascii(self.ip_addr))
        )
        polltime_s = polltime_ms / 1000
        sock = self.cast.socket_client.get_socket()
        if sock and sock.fileno() != -1:
            can_read, _, _ = select.select([sock], [], [], 0.1)
            # can_read = True
            if can_read:
                print_nfo(
                    "[Chromecast] [{0}] [poll_socket can_read]".format(
                        to_ascii(self.ip_addr)
                    )
                )
                # Received something on the socket, gets handled with run_once()
                try:
                    self.cast.socket_client.run_once()
                    print_nfo(
                        "[Chromecast] [{0}] [poll_socket read_once]".format(
                            to_ascii(self.ip_addr)
                        )
                    )
                except Exception as exception:
                    pass
        print_wrn("[Chromecast] [{0}] [poll_socket end]".format(to_ascii(self.ip_addr)))

    def media_load(
        self,
        url,
        content_type,
        title=None,
        thumb=DEFAULT_THUMB,
        current_time=0,
        autoplay=True,
        stream_type=STREAM_TYPE_UNKNOWN,
    ):
        print_nfo(
            "[Chromecast] [{0}] [media_load start]".format(to_ascii(self.ip_addr))
        )
        mc = self.cast.media_controller
        if not mc.is_active:
            self._do_activate()
        st = mc.status
        try:
            if not thumb or thumb == "":
                thum = DEFAULT_THUMB
            if st.player_is_playing:
                self.cast.media_controller.stop()
                time.sleep(1)
            mc.play_media(
                url, content_type, title, thumb, current_time, autoplay, stream_type
            )
        except Exception as exception:
            print_err("Unable to load stream")
            exc_type, value, traceback = sys.exc_info()
            print_exception(exc_type, value, traceback)

        print_wrn("[Chromecast] [{0}] [media_load end]".format(to_ascii(self.ip_addr)))

    def media_play(self):
        if self.cast.media_controller.is_active:
            self.cast.media_controller.play()

    def media_pause(self):
        if self.cast.media_controller.is_active:
            self.cast.media_controller.pause()

    def media_stop(self):
        if self.cast.media_controller.is_active:
            self.cast.media_controller.stop()

    def media_vol(self, volume):
        vol = volume / 100
        try:
            self.cast.set_volume(vol)
        except PyChromecastError:
            pass

    def media_vol_up(self):
        vol = round(self.cast.status.volume_level, 1)
        try:
            self.cast.set_volume(vol + 0.1)
        except PyChromecastError:
            pass

    def media_vol_down(self):
        vol = round(self.cast.status.volume_level, 1)
        try:
            self.cast.set_volume(vol - 0.1)
        except PyChromecastError:
            pass

    def media_mute(self):
        try:
            self.cast.set_volume_muted(True)
        except PyChromecastError:
            pass

    def media_unmute(self):
        try:
            self.cast.set_volume_muted(False)
        except PyChromecastError:
            pass

    def new_cast_status(self, status):
        """

        """

        # CastStatus(is_active_input=None, is_stand_by=None, volume_level=0.8999999761581421, volume_muted=False, app_id=u'CC1AD845', display_name=u'Default Media Receiver', namespaces=[u'urn:x-cast:com.google.cast.debugoverlay', u'urn:x-cast:com.google.cast.broadcast', u'urn:x-cast:com.google.cast.media'], session_id=u'2f63312e-4777-454f-acc7-8be72572c7c8', transport_id=u'2f63312e-4777-454f-acc7-8be72572c7c8', status_text=u'Now Casting: Tizonia Audio Stream')
        print_nfo(
            "[Chromecast] [{0}] [new_cast_status start]".format(to_ascii(self.ip_addr))
        )
        if status:
            try:
                if not status.app_id:
                    print_nfo(
                        "[Chromecast] [UNKNOWN] [new_cast_status]".format(
                            to_ascii(self.ip_addr)
                        )
                    )
                    self.cast_status_listener(to_ascii("UNKNOWN"), status.volume_level)
                elif (
                    status.app_id == APP_MEDIA_RECEIVER
                    and "Default Media Receiver" in status.status_text
                ):
                    print_nfo(
                        "[Chromecast] [READY_TO_CAST] [new_cast_status]".format(
                            to_ascii(self.ip_addr)
                        )
                    )
                    self.cast_status_listener(
                        to_ascii("READY_TO_CAST"), status.volume_level
                    )
                elif (
                    status.app_id == APP_MEDIA_RECEIVER
                    and "casting" in status.status_text.lower()
                ):
                    print_nfo(
                        "[Chromecast] [NOW_CASTING] [new_cast_status]".format(
                            to_ascii(self.ip_addr)
                        )
                    )
                    self.cast_status_listener(
                        to_ascii("NOW_CASTING"), status.volume_level
                    )
                else:
                    print_nfo(
                        "[Chromecast] [UNKNOWN] [new_cast_status]".format(
                            to_ascii(self.ip_addr)
                        )
                    )
                    self.cast_status_listener(to_ascii("UNKNOWN"), status.volume_level)
            except Exception as exception:
                logging.info("Unable to deliver cast status callback %s", exception)
        print_wrn(
            "[Chromecast] [{0}] [new_cast_status end]".format(to_ascii(self.ip_addr))
        )

    def new_media_status(self, status):
        """

        """

        # <MediaStatus {'player_state': u'BUFFERING', 'volume_level': 1, 'images': [MediaImage(url=u'https://avatars0.githubusercontent.com/u/3161606?v=3&s=400', height=None, width=None)], 'media_custom_data': {}, 'duration': None, 'current_time': 0, 'playback_rate': 1, 'title': u'Tizonia Audio Stream', 'media_session_id': 4, 'volume_muted': False, 'supports_skip_forward': False, 'track': None, 'season': None, 'idle_reason': None, 'stream_type': u'LIVE', 'supports_stream_mute': True, 'supports_stream_volume': True, 'content_type': u'audio/mpeg', 'metadata_type': None, 'subtitle_tracks': {}, 'album_name': None, 'series_title': None, 'album_artist': None, 'media_metadata': {u'images': [{u'url': u'https://avatars0.githubusercontent.com/u/3161606?v=3&s=400'}], u'thumb': u'https://avatars0.githubusercontent.com/u/3161606?v=3&s=400', u'title': u'Tizonia Audio Stream'}, 'episode': None, 'artist': None, 'supported_media_commands': 15, 'supports_seek': True, 'current_subtitle_tracks': [], 'content_id': u'http://streams.radiobob.de/bob-acdc/mp3-192/dirble/', 'supports_skip_backward': False, 'supports_pause': True}>

        print_nfo(
            "[Chromecast] [{0}] [new_media_status start]".format(to_ascii(self.ip_addr))
        )
        if not self.active:
            print_wrn("new_media_status: not active!")
            return
        if status:
            try:
                print_nfo(
                    "[Chromecast] [{0}] [new_media_status : {1}]".format(
                        to_ascii(self.ip_addr), to_ascii(status.player_state)
                    )
                )
                self.media_status_listener(
                    to_ascii(status.player_state), status.volume_level
                )
            except Exception as exception:
                logging.info("Unable to deliver media status callback %s", exception)
        print_wrn(
            "[Chromecast] [{0}] [new_media_status end]".format(to_ascii(self.ip_addr))
        )


if __name__ == "__main__":
    tizchromecastproxy()

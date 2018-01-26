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

"""@package tizchromecastproxy
Simple Chromecast proxy/wrapper.

Access a Chromecast device to initiate and manage audio streaming sessions..

"""

from __future__ import unicode_literals
from __future__ import division

import select
import sys
import logging
import unicodedata
import pychromecast
from pychromecast.controllers.media import (
    STREAM_TYPE_UNKNOWN,
    STREAM_TYPE_BUFFERED,
    STREAM_TYPE_LIVE,
    MEDIA_PLAYER_STATE_PLAYING,
    MEDIA_PLAYER_STATE_BUFFERING,
    MEDIA_PLAYER_STATE_PAUSED,
    MEDIA_PLAYER_STATE_IDLE,
    MEDIA_PLAYER_STATE_UNKNOWN
)
from pychromecast.error import (
    PyChromecastError)
from pychromecast.config import (
    APP_MEDIA_RECEIVER
)

# For use during debugging
import pprint
from traceback import print_exception

DEFAULT_THUMB="https://avatars0.githubusercontent.com/u/3161606?v=3&s=400"
FORMAT = '[%(asctime)s] [%(levelname)5s] [%(thread)d] ' \
         '[%(module)s:%(funcName)s:%(lineno)d] - %(message)s'

logging.captureWarnings(True)
#logging.getLogger().addHandler(logging.NullHandler())
logging.basicConfig(format=FORMAT)
logging.getLogger().setLevel(logging.DEBUG)

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

    print_err("[Chromecast] (%s) : %s" % (exception_type.__name__, exception))
    # del traceback # unused
    print_exception(exception_type, exception, traceback)

sys.excepthook = exception_handler

def to_ascii(msg):
    """Unicode to ascii helper.

    """

    return unicodedata.normalize('NFKD', unicode(msg)).encode('ASCII', 'ignore')


class tizchromecastproxy(object):
    """A class that interfaces with a Chromecast device to initiate and manage
    audio streaming sessions.

    """
    def __init__(self, name_or_ip):
        self.name_or_ip = name_or_ip
        self.cast = pychromecast.Chromecast(name_or_ip, blocking=False)
        self.cast_status_listener = None
        self.media_status_listener = None
        self.cast.register_status_listener(self)
        self.cast.media_controller.register_status_listener(self)

    def activate(self, cast_status_listener, media_status_listener):
        self.cast_status_listener = cast_status_listener
        self.media_status_listener = media_status_listener
        print_nfo("[Chromecast] [{0}] [Activating]" \
                  .format(to_ascii(self.name_or_ip)))
        self.cast.play_media(
            DEFAULT_THUMB, pychromecast.STREAM_TYPE_BUFFERED)

    def deactivate(self):
        self.cast.quit_app()

    def poll_socket(self, polltime_ms):
        polltime_s = polltime_ms / 1000;
        can_read, _, _ = select.select([self.cast.socket_client.get_socket()], [], [], polltime_s)
        if can_read:
            # Received something on the socket, gets handled with run_once()
            self.cast.socket_client.run_once()

    def media_load(self, url, content_type, title=None,
                   thumb=DEFAULT_THUMB,
                   current_time=0, autoplay=True,
                   stream_type=STREAM_TYPE_BUFFERED):
        print_nfo("[Chromecast] [{0}] [Loading stream]" \
                  .format(to_ascii(self.name_or_ip)))
        logging.info("proxy : Loading a new stream")
        mc = self.cast.media_controller
        st = mc.status
        try:
            mc.play_media(url, content_type, title,
                          thumb, current_time, autoplay,
                          stream_type)
        except Exception as exception:
            print_err('Unable to load stream')
        else:
            mc.block_until_active()

    def media_play(self):
        self.cast.media_controller.play()

    def media_pause(self):
        self.cast.media_controller.pause()

    def media_stop(self):
        self.cast.media_controller.stop()

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
        if status:
            logging.info("new_cast_status: %r" % (status,))
            try:
                if not status.app_id:
                    self.cast_status_listener(to_ascii(u'UNKNOWN'))
                elif status.app_id == APP_MEDIA_RECEIVER \
                     and status.status_text == u'Ready To Cast':
                    self.cast_status_listener(to_ascii(u'READY_TO_CAST'))
                elif status.app_id == APP_MEDIA_RECEIVER \
                     and u'Now Casting' in status.status_text:
                    self.cast_status_listener(to_ascii(u'NOW_CASTING'))
            except Exception as exception:
                logging.info('Unable to deliver cast status callback %s', \
                             exception)

    def new_media_status(self, status):
        """

        """

        # <MediaStatus {'player_state': u'BUFFERING', 'volume_level': 1, 'images': [MediaImage(url=u'https://avatars0.githubusercontent.com/u/3161606?v=3&s=400', height=None, width=None)], 'media_custom_data': {}, 'duration': None, 'current_time': 0, 'playback_rate': 1, 'title': u'Tizonia Audio Stream', 'media_session_id': 4, 'volume_muted': False, 'supports_skip_forward': False, 'track': None, 'season': None, 'idle_reason': None, 'stream_type': u'LIVE', 'supports_stream_mute': True, 'supports_stream_volume': True, 'content_type': u'audio/mpeg', 'metadata_type': None, 'subtitle_tracks': {}, 'album_name': None, 'series_title': None, 'album_artist': None, 'media_metadata': {u'images': [{u'url': u'https://avatars0.githubusercontent.com/u/3161606?v=3&s=400'}], u'thumb': u'https://avatars0.githubusercontent.com/u/3161606?v=3&s=400', u'title': u'Tizonia Audio Stream'}, 'episode': None, 'artist': None, 'supported_media_commands': 15, 'supports_seek': True, 'current_subtitle_tracks': [], 'content_id': u'http://streams.radiobob.de/bob-acdc/mp3-192/dirble/', 'supports_skip_backward': False, 'supports_pause': True}>

        if status:
            logging.info("new_media_status: %r" % (status,))
            try:
                self.media_status_listener(to_ascii(status.player_state))
            except Exception as exception:
                logging.info('Unable to deliver media status callback %s', \
                             exception)

if __name__ == "__main__":
    tizchromecastproxy()

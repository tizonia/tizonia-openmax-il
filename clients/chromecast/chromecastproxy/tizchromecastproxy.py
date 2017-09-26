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

import sys
import os
import logging
import collections
import unicodedata
import time
import ctypes
from multiprocessing import Process, JoinableQueue
from Queue import Empty
import select
import pychromecast
from pychromecast.controllers.media import STREAM_TYPE_UNKNOWN
from pychromecast.controllers.media import STREAM_TYPE_BUFFERED
from pychromecast.controllers.media import STREAM_TYPE_LIVE
from abc import abstractmethod, abstractproperty

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

SYS_gettid = 186
libc = ctypes.cdll.LoadLibrary('libc.so.6')

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
    #del traceback # unused
    print_exception(exception_type, exception, traceback)

sys.excepthook = exception_handler

def to_ascii(msg):
    """Unicode to ascii helper.

    """

    return unicodedata.normalize('NFKD', unicode(msg)).encode('ASCII', 'ignore')

class ChromecastCmdIf(object):
    """
    Abstract class for commands that will be processed by the ChromecastWorker
    worker thread
    """

    @abstractmethod
    def run(self, worker):
        """
        Runs the command.
        """
class ChromecastCmdStart(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        tid = libc.syscall(SYS_gettid)
        logging.info("cmd start [%d] : creating chromecast object", tid)
        worker.cast = pychromecast.Chromecast(worker.name_or_ip)
        worker.cast.wait(5.0)
        worker.cast.register_status_listener(worker)
        worker.cast.media_controller.register_status_listener(worker)

class ChromecastCmdMediaLoad(ChromecastCmdIf):
    """

    """

    def __init__ (self, url, content_type, title=None, thumb=None,
                  current_time=0, autoplay=True,
                  stream_type=STREAM_TYPE_LIVE):
        self.url = url
        self.content_type = content_type
        self.title = title
        self.thumb = thumb
        self.current_time = current_time
        self.autoplay = autoplay
        self.stream_type = stream_type

    def run (self, worker):
        logging.info("cmd load")
        mc = worker.cast.media_controller
        st = mc.status
        try:
            if st.player_is_playing:
                mc.stop()
            mc.play_media(self.url, self.content_type, self.title,
                          self.thumb, self.current_time, self.autoplay,
                          self.stream_type)
        except Exception as exception:
            print_err('Unable to load stream')
#         else:
#             mc.block_until_active()


class ChromecastCmdMediaPlay(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd play")
        worker.cast.media_controller.play()

class ChromecastCmdMediaPause(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd pause")
        worker.cast.media_controller.pause()

class ChromecastCmdMediaStop(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd stop")
        worker.cast.media_controller.stop()

class ChromecastCmdMediaVolUp(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd vol up")
        vol = round(worker.cast.status.volume_level, 1)
        worker.cast.set_volume(vol + 0.1)

class ChromecastCmdMediaVolDown(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd vol down")
        vol = round(worker.cast.status.volume_level, 1)
        worker.cast.set_volume(vol - 0.1)

class ChromecastCmdMediaMute(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd vol mute")
        worker.cast.set_volume_muted(True)

class ChromecastCmdMediaUnmute(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd vol unmute")
        worker.cast.set_volume_muted(False)

class ChromecastWorker(Process):
    """

    """
    def __init__ (self, queue, name_or_ip, cast_status_listener,
                  media_status_listener, *args, **kwargs):
        Process.__init__(self, *args, **kwargs)
        self.queue = queue
        self.name_or_ip = name_or_ip
        self.cast = None
        self.cast_status_listener = cast_status_listener
        self.media_status_listener = media_status_listener

    def start (self):
        self.cast = pychromecast.Chromecast(self.name_or_ip)
        self.cast.wait(5.0)
        self.cast.register_status_listener(self)
        self.cast.media_controller.register_status_listener(self)
        super(ChromecastWorker, self).start()

    def stop (self):
        tid = libc.syscall(SYS_gettid)
        logging.info("worker [%d] : Stopping", tid)
        self.queue.put(None)
        logging.info("worker [%d] : joining queue", tid)
        self.queue.join()
        logging.info("worker [%d] : joined queue", tid)

    def run (self):
        polltime = 0.1
        tid = libc.syscall(SYS_gettid)
        while True:
            try:
                qsize = self.queue.qsize()
                cmd = self.queue.get(True, 0.01)
                if cmd is None:
                    logging.info("worker [%d] : breaking", tid)
                    self.queue.task_done()
                    break
                else:
                    logging.info("worker [%d] : running cmd %s", \
                                 tid, cmd.__class__.__name__)
                    cmd.run(self)
                    self.queue.task_done()
                    logging.info("worker [%d] : DONE running cmd %s", \
                                 tid, cmd.__class__.__name__)

            except Empty:
                pass

            if self.cast:
                sk = self.cast.socket_client
                can_read, _, _ = select.select([sk.get_socket()],
                                               [], [], polltime)
                if can_read:
                    try:
                        sk.run_once()
                    except Exception as e:
                        logging.info("worker [%d] : run_once exception : %s", \
                                     tid, e)
                        pass

        if self.cast:
            try:
                time.sleep(3)
                logging.info("worker [%d]: disconnecting", tid)
                self.cast.quit_app()
                logging.info("worker [%d]: disconnected", tid)
            except Exception as e:
                logging.info("worker [%d]: cast disconnect exception : %s", \
                             tid, e)
                pass
        logging.info("worker [%d]: TERMINATED", tid)

        return

    def new_cast_status(self, status):
        """

        """

        # CastStatus(is_active_input=None, is_stand_by=None, volume_level=0.8999999761581421, volume_muted=False, app_id=u'CC1AD845', display_name=u'Default Media Receiver', namespaces=[u'urn:x-cast:com.google.cast.debugoverlay', u'urn:x-cast:com.google.cast.broadcast', u'urn:x-cast:com.google.cast.media'], session_id=u'2f63312e-4777-454f-acc7-8be72572c7c8', transport_id=u'2f63312e-4777-454f-acc7-8be72572c7c8', status_text=u'Now Casting: Tizonia Audio Stream')
        if status:
            logging.info("new_cast_status: %r" % (status,))
            try:
                self.cast_status_listener(to_ascii(status.status_text))
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


class tizchromecastproxy(object):
    """A class that interfaces with a Chromecast device to initiate and manage
    audio streaming sessions.

    """
    def __init__(self, name_or_ip):
        self.queue = JoinableQueue()
        self.worker = None
        self.name_or_ip = name_or_ip
        self.cast_status_listener = None
        self.media_status_listener = None

    def start(self, cast_status_listener, media_status_listener):
        tid = libc.syscall(SYS_gettid)
        logging.info("proxy : [%d] starting worker", tid)
        self.cast_status_listener = cast_status_listener
        self.media_status_listener = media_status_listener
        self.worker = ChromecastWorker(self.queue, self.name_or_ip,
                                       self.cast_status_listener,
                                       self.media_status_listener)
        #self.queue.put(ChromecastCmdStart())
        self.worker.start()
        logging.info("proxy : [%d] started worker", tid)

    def stop(self):
        tid = libc.syscall(SYS_gettid)
        logging.info("proxy [%d] : stopping worker", tid)
        self.worker.stop()
        logging.info("proxy [%d] : joining worker", tid)
        self.worker.join()
        logging.info("proxy [%d] : joined worker", tid)

    def media_load(self, url, content_type, title=None,
                   thumb=DEFAULT_THUMB,
                   current_time=0, autoplay=True,
                   stream_type=STREAM_TYPE_LIVE):
        logging.info("proxy : Loading a new stream")
        self.queue.put(ChromecastCmdMediaLoad(url, content_type, title, thumb,
                                         current_time, autoplay,
                                         stream_type))
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_play(self):
        self.queue.put(ChromecastCmdMediaPlay())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_pause(self):
        self.queue.put(ChromecastCmdMediaPause())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_stop(self):
        self.queue.put(ChromecastCmdMediaStop())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_vol_up(self):
        self.queue.put(ChromecastCmdMediaVolUp())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_vol_down(self):
        self.queue.put(ChromecastCmdMediaVolDown())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_mute(self):
        self.queue.put(ChromecastCmdMediaMute())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_unmute(self):
        self.queue.put(ChromecastCmdMediaUnmute())
        logging.info("proxy : queue size %d", self.queue.qsize())

if __name__ == "__main__":
    tizchromecastproxy()

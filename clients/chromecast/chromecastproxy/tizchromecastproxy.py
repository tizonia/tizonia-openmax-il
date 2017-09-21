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
from multiprocessing import Process, Queue
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

def info(title):
    print title
    print 'module name:', __name__
    if hasattr(os, 'getppid'):  # only available on Unix
        logging.info("parent process: %d", os.getppid())
    logging.info("process id: %d", os.getpid())

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

class ChromecastCmdLoad(ChromecastCmdIf):
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
            pprint.pprint(st)
            if not st.player_is_playing:
                mc.stop()
            mc.play_media(self.url, self.content_type, self.title,
                          self.thumb, self.current_time, self.autoplay,
                          self.stream_type)
        except Exception as exception:
            print_err('Unable to load stream')
#         else:
#             mc.block_until_active()


class ChromecastCmdPlay(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd play")
        worker.cast.media_controller.play()

class ChromecastCmdPause(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd pause")
        worker.cast.media_controller.pause()

class ChromecastCmdStop(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd stop")
        worker.cast.media_controller.stop()

class ChromecastCmdVolUp(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd vol up")
        vol = round(worker.cast.status.volume_level, 1)
        worker.cast.set_volume(vol + 0.1)

class ChromecastCmdVolDown(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd vol down")
        vol = round(worker.cast.status.volume_level, 1)
        worker.cast.set_volume(vol - 0.1)

class ChromecastCmdVolMute(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        logging.info("cmd vol mute")
        worker.cast.set_volume_muted()

class ChromecastWorker(Process):
    """

    """
    def __init__ (self, queue, name_or_ip, status_listener, *args, **kwargs):
        Process.__init__(self, *args, **kwargs)
        self.queue = queue
        self.name_or_ip = name_or_ip
        self.cast = None
        self.mc = None
        self.status_listener = status_listener

    def start (self):
        logging.info("Creating the chromecast worker thread")
        self.cast = pychromecast.Chromecast(self.name_or_ip)
        self.mc = self.cast.media_controller
        self.mc.register_status_listener(self)
        pprint.pprint(self.mc)
        super(ChromecastWorker, self).start()

    def stop (self):
        logging.info("Stopping the chromecast worker thread")
        self.queue.put(None)
        self.queue.join()
        if self.cast:
            self.cast.media_controller.tear_down()
            self.cast.quit_app()

    def run (self):
        polltime = 0.1
        while True:
            qsize = self.queue.qsize()
            logging.info("worker : queue size %d", qsize)
            logging.info("worker : get from queue")
            try:
                cmd = self.queue.get(False)
                if cmd is None:
                    break
                else:
                    logging.info("worker: running cmd %s", cmd.__class__.__name__)
                    cmd.run(self)
            except Empty:
                logging.info("worker: queue empty")
                pass

            if self.cast:
                sk = self.cast.socket_client
                can_read, _, _ = select.select([sk.get_socket()],
                                               [], [], polltime)
                if can_read:
                    logging.info("worker: can read")
                    sk.run_once()

        return

    def new_media_status(self, status):
        """

        """
        logging.info("worker: calling client")
        self.status_listener("called from Python")

class tizchromecastproxy(object):
    """A class that interfaces with a Chromecast device to initiate and manage
    audio streaming sessions.

    """
    def __init__(self, name_or_ip):
        self.queue = Queue()
        self.worker = None
        self.name_or_ip = name_or_ip

    def start(self, status_listener):
        info('start')
        logging.info("Starting worker")
        self.worker = ChromecastWorker(self.queue, self.name_or_ip,
                                       status_listener)
        self.worker.start()

    def stop(self):
        self.worker.stop()

    def media_load(self, url, content_type, title=None,
                   thumb=DEFAULT_THUMB,
                   current_time=0, autoplay=True,
                   stream_type=STREAM_TYPE_LIVE):
        info('load')
        logging.info("Loading a new stream")
        self.queue.put(ChromecastCmdLoad(url, content_type, title, thumb,
                                         current_time, autoplay,
                                         stream_type))
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_play(self):
        info('play')
        self.queue.put(ChromecastCmdPlay())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_pause(self):
        self.queue.put(ChromecastCmdPause())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_stop(self):
        self.queue.put(ChromecastCmdStop())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_vol_up(self):
        self.queue.put(ChromecastCmdVolUp())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_vol_down(self):
        self.queue.put(ChromecastCmdVolDown())
        logging.info("proxy : queue size %d", self.queue.qsize())

    def media_vol_mute(self):
        self.queue.put(ChromecastCmdVolMute())
        logging.info("proxy : queue size %d", self.queue.qsize())

if __name__ == "__main__":
    tizchromecastproxy()

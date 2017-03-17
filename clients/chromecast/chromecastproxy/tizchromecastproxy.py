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
import logging
import random
import chromecast
import collections
import unicodedata
import threading
import Queue
import pychromecast
from requests.exceptions import HTTPError
from operator import itemgetter

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

    print_err("[Chromecast] (%s) : %s" % (exception_type.__name__, exception))
    del traceback # unused
    #print_exception(exception_type, exception, traceback)

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
    def run(self, manager):
        """
        Runs the command.
        """

class ChromecastCmdLoad(ChromecastCmdIf):
    """

    """

    def __init__ (self, url, content_type, title=None, thumb=None,
                  current_time=0, autoplay=True,
                  stream_type=STREAM_TYPE_BUFFERED):
        self.url = url
        self.content_type = content_type
        self.title = title
        self.thumb = thumb
        self.current_time = current_time
        self.autoplay = autoplay
        self.stream_type = stream_type

    def run (self, worker):
        worker.cast.play_media(self.url, self.content_type, self.title,
                               self.thumb, self.current_time, self.autoplay,
                               self.stream_type)


class ChromecastCmdPlay(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        worker.cast.media_controller.play()

class ChromecastCmdPause(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        worker.cast.media_controller.pause()

class ChromecastCmdStop(ChromecastCmdIf):
    """

    """
    def run (self, worker):
        worker.cast.media_controller.stop()

class ChromecastWorker(threading.Thread):
    """

    """
    def __init__ (self, name_or_ip, *args, **kwargs):
        threading.Thread.__init__(self, *args, **kwargs)
        self.queue = Queue.Queue(0)
        self.name_or_ip = name_or_ip
        self.cast = cast = pychromecast.Chromecast(self.name_or_ip)

    def load (self, url, content_type, title=None, thumb=None,
              current_time=0, autoplay=True,
              stream_type=STREAM_TYPE_BUFFERED):
        if self.cast:
            self.queue.put(ChromecastCmdLoad(url, content_type, title, thumb,
                                             current_time, autoplay, stream_type))

    def run (self):
        polltime = 0.1
        while True:
            if self.cast:
                can_read, _, _ = select.select([self.cast.socket_client.get_socket()], [], [], polltime)
                if can_read:
                    self.cast.socket_client.run_once()
                do_actions(self.cast, t)

            if not self.queue.empty():
                cmd = self.queue.get()
                if cmd is None:
                    if self.cast:
                        cast.media_controller.tear_down()
                    break
                cmd.run(self)
                self.queue.task_done()

        self.queue.task_done()
        return

    def unload (self):
        self.queue.put(None)
        self.queue.join()

    def play(self):
        """ Send the PLAY command. """
        if self.cast:
            self.queue.put(ChromecastCmdPlay())

    def pause(self):
        """ Send the PAUSE command. """
        if self.cast:
            self.queue.put(ChromecastCmdPause())

    def stop(self):
        """ Send the STOP command. """
        if self.cast:
            self.queue.put(ChromecastCmdStop())

class tizchromecastproxy(object):
    """A class that interfaces with a Chromecast device to initiate and manage
    audio streaming sessions.

    """
    def __init__(self, name_or_ip):
        self.worker = ChromecastWorker(name_or_ip)

    def load(url, content_type, title=None, thumb=None,
             current_time=0, autoplay=True,
             stream_type=STREAM_TYPE_BUFFERED)
        self.worker.load(url, content_type, title, thumb,
                         current_time, autoplay, stream_type)

    def unload()
        self.worker.unload()

if __name__ == "__main__":
    tizchromecastproxy()

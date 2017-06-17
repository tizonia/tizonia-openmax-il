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


class Artist(object):

    """
    :param uri: artist URI
    :type uri: string
    :param name: artist name
    :type name: string
    """

    def __init__(self, uri, name):
        self.uri = uri
        self.name = name

class Album(object):

    """
    :param uri: album URI
    :type uri: string
    :param name: album name
    :type name: string
    :param artists: album artists
    :type artists: list of :class:`Artist`
    :param date: album release date (YYYY or YYYY-MM-DD)
    :type date: string

    """

    def __init__(self, uri, name, artists, date):
        self.uri = uri
        self.name = name
        self.artists = artists
        self.date = date


class Track(object):

    """
    :param uri: track URI
    :type uri: string
    :param name: track name
    :type name: string
    :param artists: track artists
    :type artists: list of :class:`Artist`
    :param album: track album
    :type album: :class:`Album`
    :param length: track length in milliseconds
    :type length: integer or :class:`None` if there is no duration
    """

    def __init__(self, uri, name, artists, album, length):
        self.uri = uri
        self.name = name
        self.artists = artists
        self.album = album
        self.length = length


class Playlist(object):

    """
    :param uri: playlist URI
    :type uri: string
    :param name: playlist name
    :type name: string
    """

    def __init__(self, uri, name, artists, album, length):
        self.uri = uri
        self.name = name

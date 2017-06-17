# -*- coding: utf-8 -*-
# Original Author: Konstantin Batura <https://github.com/rusty-dev>
#
# Portions Copyright (C) 2017 Aratelia Limited - Juan A. Rubio
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

import logging
import random
import re
import string
import requests
import simplejson as json
import hashlib
import pprint

from io import BytesIO
from operator import xor
from struct import pack
from Crypto.Cipher import AES, Blowfish
from eyed3.mp3.headers import Mp3Header, findHeader, timePerFrame
from tizdeezermodels import Album, Artist, Track, Playlist
from array import array
from tizmopidydeezerutils import get_md5

logger = logging.getLogger(__name__)


def load_artist(data):
    return Artist(
        name=data['ART_NAME'],
        uri='deezer:artist:%s' % data['ART_ID']
    )


def load_album(data, artist):
    return Album(
        name=data['ALB_TITLE'],
        uri='deezer:album:%s' % data['ALB_ID'],
        date=data.get('DIGITAL_RELEASE_DATE', None),
        artists=[artist],
    )


def load_track(data, album, artist):
    return Track(
        name=data['SNG_TITLE'],
        uri='deezer:track:%s' % data['SNG_ID'],
        length=int(data['DURATION']) * 1000,
        album=album,
        artists=[artist]
    )


def load_tracklist(tracklist):
    tracks = []
    for track in tracklist:
        artist = load_artist(track)
        album = load_album(track, artist)
        tracks.append(load_track(track, album, artist))
    return tracks


def format_playlist_title(title, fans, songs):
    return u'{} (♥{} ♫{})'.format(title, fans, songs)


class DeezerClient(object):
    def __init__(self, user_id):
        super(DeezerClient, self).__init__()
        self._user_id = user_id
        self.session = requests.session()
        self._api_token = None

    @property
    def api_token(self):
        if self._api_token is not None:
            return self._api_token

        data = self.session.get('http://www.deezer.com').text
        try:
            self._api_token = re.search(r'<input id="checkForm" name="checkForm" type="hidden" value="([\w~\-.]+)">', data).group(1)
            return self._api_token
        except:
            return None

    def _api_call(self, method, **params):
        get_query = {
            'api_version': '1.0',
            'api_token': self.api_token,
            'input': 3,
            'cid': self.get_cid()
        }
        # apparently, it's possible to batch multiple methodos in one request, but nobody uses that...
        post_query = [{
            'method': method,
            'params': params
        }]
        endpoint = "http://www.deezer.com/ajax/gw-light.php?"
        response = self.session.post(
            endpoint,
            params=get_query,
            data=json.dumps(post_query, separators=(',', ':'))
        )
        data = json.loads(response.text)
        # on critical errors, it returns a dict instead of list, go figure.
        if isinstance(data, list):
            data = data[0]

        return data

    def api_call(self, method, **params):
        for _ in range(2):
            data = self._api_call(method, **params)
            if data['error']:
                logger.error('Error during deezer api call, method: %s, params: %s, error: %s', method, params, data['error'])
                # invalidate current api token if it's outdated
                if data['error'].get('VALID_TOKEN_REQUIRED', None):
                    self._api_token = None
                    continue
            break

        return data['results']

    def search(self, query):
        data = self.api_call('deezer.pageSearch', query=query, top_tracks=True, start=0, nb=40)

        artists = [load_artist(artist) for artist in data['ARTIST']['data']]
        albums = [load_album(album, load_artist(album['ARTISTS'][0])) for album in data['ALBUM']['data']]

        tracks = load_tracklist(data['TRACK']['data'])

        return artists, albums, tracks

    def lookup_track(self, track_id):
        track_id = int(track_id)
        data = self.get_track(track_id)
        artist = load_artist(data)
        album = load_album(data, artist)

        return [load_track(data, album, artist)]

    def lookup_album(self, album_id):
        data = self.api_call('song.getListByAlbum', alb_id=album_id, nb=500)

        tracks = load_tracklist(data['data'])

        return tracks

    def lookup_artist(self, artist_id):
        data = self.api_call('deezer.pageArtist', art_id=artist_id, lang='en')
        tracklist = []

        for album in data['ALBUMS']['data']:
            tracklist.extend(album['SONGS']['data'])

        tracks = load_tracklist(tracklist)

        return tracks

    def lookup_radio(self, radio_id):
        data = self.api_call('radio.getSongs', radio_id=radio_id)

        return load_tracklist(data['data'])

    def lookup_user_radio(self, user_id):
        data = self.api_call('radio.getUserRadio', user_id=user_id)

        return load_tracklist(data['data'])

    def lookup_playlist_raw(self, playlist_id):
        data = self.api_call('playlist.getSongs', playlist_id=playlist_id, start=0, nb=1000)

        return data['data']

    def lookup_playlist(self, playlist_id):
        return load_tracklist(self.lookup_playlist_raw(playlist_id))

    def browse_artists(self):
        data = self.api_call('deezer.pageProfile', user_id=self._user_id, tab="artists", nb=1000)

        artists = []
        for artist in data['TAB']['artists']['data']:
            artists.append(Artist(
                name=artist['ART_NAME'],
                uri='deezer:artist:%s' % artist['ART_ID']
            ))

        artists.sort(key=lambda artist: artist.name)

        return artists

    def browse_albums(self):
        data = self.api_call('deezer.pageProfile', user_id=self._user_id, tab="albums", nb=1000)

        albums = []
        for album in data['TAB']['albums']['data']:
            albums.append(Album(
                name='%s - %s' % (album['ART_NAME'], album['ALB_TITLE']),
                uri='deezer:album:%s' % album['ALB_ID']
            ))

        albums.sort(key=lambda album: album.name)

        return albums

    def browse_playlists(self):
        data = self.api_call(
            'deezer.pageProfile',
            user_id=self._user_id,
            tab='playlists',
            nb=40)

        return data['TAB']['playlists']['data']

    def browse_flow(self):
        tracks = []
        for track in self.lookup_user_radio(self._user_id):
            tracks.append(Track(
                name=track.name,
                uri=track.uri
            ))
        return tracks

    def browse_mixes(self):
        data = self.api_call('radio.getSortedList')
        mixes = []
        for radio in data['data']:
            mixes.append(Playlist(
                name=radio['TITLE'],
                uri='deezer:radio:%s' % radio['RADIO_ID'])
            )

        return mixes

    def browse_top_playlists(self):
        data = self.api_call('deezer.pageTops', type='playlist', genre_id=0, start=0, nb=100, lang='en')
        playlists = []
        for playlist in data['ITEMS']['data']:
            playlists.append(Playlist(
                name=format_playlist_title(playlist['TITLE'], playlist['NB_FAN'], playlist['NB_SONG']),
                uri='deezer:playlist:%s' % playlist['PLAYLIST_ID'])
            )
        return playlists

    def browse_moods(self):
        data = self.api_call('deezer.pageMoods', type='playlist', genre_id=0, start=0, nb=100)
        playlists = []
        for playlist in data['ITEMS']['data']:
            playlists.append(Playlist(
                name=format_playlist_title(playlist['TITLE'], playlist['NB_FAN'], playlist['NB_SONG']),
                uri='deezer:playlist:%s' % playlist['PLAYLIST_ID'])
            )
        return playlists

    @staticmethod
    def get_cid():
        source = string.digits + string.ascii_lowercase
        return ''.join(random.choice(source) for _ in xrange(18))

    def get_track(self, track_id):
        if not track_id:
            return None

        logger.info('Single track lookup, track_id=%s' % track_id)
        data = self.api_call('song.getData', sng_id=track_id)

        if 'FALLBACK' in data:
            data.update(data['FALLBACK'])

        return data

    def get_track_cipher(self, track_id):
        """ Get track-specific cipher from `track_id` """
        track_md5 = get_md5(str(track_id).lstrip('-'))
        key_parts = map(lambda v: array('B', v), ('g4el58wc0zvf9na1', track_md5[:16], track_md5[16:]))
        blowfish_key = b''.join(chr(reduce(xor, x)) for x in zip(*key_parts))
        IV = pack('B' * 8, *range(8))

        def track_cipher():
            return Blowfish.new(blowfish_key, mode=Blowfish.MODE_CBC, IV=IV)

        return track_cipher

    def get_track_url(self, data):
        join = u'\u00a4'.join

        proxy = data['MD5_ORIGIN'][0]
        if data['FILESIZE_MP3_320']:
            track_format = 3
        elif data['FILESIZE_MP3_256']:
            track_format = 5
        else:
            track_format = 1
        payload = join(map(str, [data['MD5_ORIGIN'], track_format, data['SNG_ID'], data['MEDIA_VERSION']]))
        payloadHash = get_md5(payload)

        def pad(s, BS=16):
            return s + (BS - len(s) % BS) * chr(BS - len(s) % BS)
        reference = pad(join([payloadHash, payload, '']).encode('latin-1'))
        cipher = AES.new('jo6aey6haid2Teih', mode=AES.MODE_ECB)
        reference = cipher.encrypt(reference).encode('hex').lower()

        return "http://e-cdn-proxy-{}.deezer.com/mobile/1/{}".format(proxy, reference)

    def _stream(self, track_cipher, track_url):
        logger.info('Streaming track: %s' % track_url)

        def decyption_stream():
            chunk_size = 2048
            inp = self.session.get(track_url, stream=True)
            i = 0
            for chunk in inp.iter_content(chunk_size):
                if not chunk:
                    continue
                if i % 3 == 0 and len(chunk) == chunk_size:
                    cipher = track_cipher()  # reset cipher state each time.
                    chunk = cipher.decrypt(chunk)
                i += 1
                yield chunk

        yield None  # yield None for priming.
        chunks = decyption_stream()
        send_frames = 1024  # amount frames to sent on each iteration

        # read mp3 header
        header = None
        first_chunk = b''
        while not header:
            first_chunk += next(chunks)
            header_buffer = BytesIO(first_chunk)
            header_pos, header, header_bytes = findHeader(header_buffer)
            del header_buffer

        h = Mp3Header(header)
        ms_per_frame = timePerFrame(h, False) * 1000
        send_size = send_frames * h.frame_length
        cache = first_chunk[header_pos + 4:]  # remove header from stream data.
        last_pos = 0
        pos = 0

        while True:
            pos_end = pos + send_size
            while len(cache) < pos_end:
                try:
                    cache += chunks.next()
                except StopIteration:
                    break

            send_buffer = cache[pos:pos_end]
            last_pos = min(pos_end, len(cache))
            if not send_buffer:
                # stream end
                send_buffer = None

            ms_pos = (yield send_buffer)
            if ms_pos is None:
                pos = last_pos
            elif ms_pos == 0:
                pos = 0
            else:
                # calculate new steam position from given MS value
                pos = int((ms_pos / ms_per_frame) * h.frame_length)

    def stream(self, track_id):
        """ Return coroutine with seeking capabilities: some_stream.send(30000) """
        track_data = self.get_track(track_id)
        pprint.pprint(track_data)
        track_cipher = self.get_track_cipher(track_data['SNG_ID'])
        track_url = self.get_track_url(track_data)
        return self._stream(track_cipher, track_url)

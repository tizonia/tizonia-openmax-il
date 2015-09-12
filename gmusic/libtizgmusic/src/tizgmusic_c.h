/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   tizgmusic_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Google Play Music client library (c wrapper)
 *
 *
 */

#ifndef TIZGMUSIC_C_H
#define TIZGMUSIC_C_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @defgroup tizgmusic A Google Play Music client library
*
* A C library to access the Google Play Music streaming service (using Simon
* Weber's 'gmusicapi' python library).
*
* @ingroup Tizonia
*/

/**
 * The gmusic opaque structure
 * @ingroup tizgmusic
 */
typedef struct tiz_gmusic tiz_gmusic_t;
typedef /*@null@ */ tiz_gmusic_t *tiz_gmusic_ptr_t;

/**
 * Initialize the gmusic handle.
 *
 * @ingroup tizgmusic
 *
 * @param app_gmusic A pointer to the gmusic handle which will be initialised.
 * @param ap_user A Google email account.
 * @param ap_pass The password associated to the Google account.
 * @param ap_device_id A 16-character string containing a device id
 * associated to the Google Play Music account.
 *
 * @return 0 on success.
 */
int tiz_gmusic_init (/*@null@ */ tiz_gmusic_ptr_t *app_gmusic,
                     const char *ap_user, const char *ap_pass,
                     const char *ap_device_id);
/**
 * Add the tracks of the specified album to the playback queue.
 *
 * After calling this methods, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_album The album name
 * @param a_all_access_search If true, All Access albums are
 * included. Otherwise, only albums from the user's library are considered.
 *
 * @return 0 on success
 */
int tiz_gmusic_play_album (tiz_gmusic_t *ap_gmusic, const char *ap_album,
                           const bool a_all_access_search);

/**
 * Add the tracks of the specified artist to the playback queue.
 *
 * After calling this methods, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @note Only tracks in the user's library are added. All Access tracks are
 * not included.
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_album The artist name
 * @param a_all_access_search If true, All Access artists are
 * included. Otherwise, only artists from the user's library are considered.
 *
 * @return 0 on success
 */
int tiz_gmusic_play_artist (tiz_gmusic_t *ap_gmusic, const char *ap_artist,
                            const bool a_all_access_search);

/**
 * Add the tracks of the specified artist to the playback queue.
 *
 * After calling this methods, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @note Only tracks in the user's library are added. All Access tracks are
 * not included.
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_album The artist name
 *
 * @return 0 on success
 */
int tiz_gmusic_play_playlist (tiz_gmusic_t *ap_gmusic, const char *ap_playlist);

/**
 * Add the tracks of the specified station to the playback queue.
 *
 * After calling this methods, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @note All Access subscription is required.
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_album The station name
 *
 * @return 0 on success
 */
int tiz_gmusic_play_station (tiz_gmusic_t *ap_gmusic, const char *ap_station);

/**
* Add All Access promoted tracks to the playback queue.
*
* After calling this methods, the various tiz_gmusic_get* methods can be
* used to interact with the playback queue.
*
* @note All Access subscription is required.
*
* @param ap_gmusic The gmusic handle.
*
* @return 0 on success
*/
int tiz_gmusic_play_promoted_tracks (tiz_gmusic_t *ap_gmusic);

/**
 * Clear the playback queue.
 *
 * @param ap_gmusic The gmusic handle.
 */
void tiz_gmusic_clear_queue (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the next track url
 *
 * The the playback queue pointer moves one position forwards.
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_next_url (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the previous track url.
 *
 * The the playback queue pointer moves one position backwards.
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_prev_url (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's artist.
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_artist (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's title.
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_title (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's album.
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_album (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's duration.
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_duration (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's track number.
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_track_number (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the number of tracks in the current song's album.
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_tracks_in_album (
    tiz_gmusic_t *ap_gmusic);

/**
 * Destroy the gmusic handle.
 *
 * @param ap_gmusic The gmusic handle.
 */
void tiz_gmusic_destroy (tiz_gmusic_t *ap_gmusic);

#ifdef __cplusplus
}
#endif

#endif  // TIZGMUSIC_C_H

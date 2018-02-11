/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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

#include <stdbool.h>

/**
* @defgroup libtizgmusic 'libtizgmusic' : A Google Play Music client library
*
* A C library to access the Google Play Music streaming service (using Simon
* Weber's 'gmusicapi' python library).
*
* @ingroup Tizonia
*/

/**
 * The gmusic opaque structure
 * @ingroup libtizgmusic
 */
typedef struct tiz_gmusic tiz_gmusic_t;
typedef /*@null@ */ tiz_gmusic_t *tiz_gmusic_ptr_t;

/**
 * Various playback modes that control the playback queue.
 * @ingroup libtizgmusic
 */
typedef enum tiz_gmusic_playback_mode {
  ETIZGmusicPlaybackModeNormal,
  ETIZGmusicPlaybackModeShuffle,
  ETIZGmusicPlaybackModeMax
} tiz_gmusic_playback_mode_t;

/**
 * Initialize the gmusic handle.
 *
 * @ingroup libtizgmusic
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
 * Set the playback mode.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
void tiz_gmusic_set_playback_mode (tiz_gmusic_t *ap_gmusic,
                                   const tiz_gmusic_playback_mode_t mode);

/**
 * Add the tracks corresponding to the specified search term to the playback
 * queue.
 *
 * After calling this method, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_tracks The tracks search term.
 * @param a_unlimited_search If true, Google Play Unlimited trackss are
 * included. Otherwise, only trackss from the user's library are considered.
 *
 * @return 0 on success
 */
int tiz_gmusic_play_tracks (tiz_gmusic_t *ap_gmusic, const char *ap_tracks,
                            const bool a_unlimited_search);

/**
 * Add the tracks of the specified album to the playback queue.
 *
 * After calling this method, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_album The album name
 * @param a_unlimited_search If true, Google Play Unlimited albums are
 * included. Otherwise, only albums from the user's library are considered.
 *
 * @return 0 on success
 */
int tiz_gmusic_play_album (tiz_gmusic_t *ap_gmusic, const char *ap_album,
                           const bool a_unlimited_search);

/**
 * Add the tracks of the specified artist to the playback queue.
 *
 * After calling this method, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_artist The artist name
 * @param a_unlimited_search If true, Google Play Unlimited artists are
 * included. Otherwise, only artists from the user's library are considered.
 *
 * @return 0 on success
 */
int tiz_gmusic_play_artist (tiz_gmusic_t *ap_gmusic, const char *ap_artist,
                            const bool a_unlimited_search);

/**
 * Add the tracks of the specified playlist to the playback queue.
 *
 * After calling this method, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_playlist The playlist name
 * @param a_unlimited_search If true, Google Play Unlimited playlists are
 * included. Otherwise, only playlists from the user's library are considered.
 *
 * @return 0 on success
 */
int tiz_gmusic_play_playlist (tiz_gmusic_t *ap_gmusic, const char *ap_playlist,
                              const bool a_unlimited_search);

/**
 * Add the tracks of the specified station to the playback queue.
 *
 * After calling this method, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizgmusic
 *
 * @note An Google Play Unlimited subscription is required.
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_station The station name
 *
 * @return 0 on success
 */
int tiz_gmusic_play_station (tiz_gmusic_t *ap_gmusic, const char *ap_station);

/**
 * Add the tracks of the specified genre to the playback queue.
 *
 * After calling this method, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizgmusic
 *
 * @note An Google Play Unlimited subscription is required.
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_genre The genre
 *
 * @return 0 on success
 */
int tiz_gmusic_play_genre (tiz_gmusic_t *ap_gmusic, const char *ap_genre);

/**
 * Add the tracks of the specified situation to the playback queue.
 *
 * After calling this method, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizgmusic
 *
 * @note An Google Play Unlimited subscription is required.
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_situation The situation
 *
 * @return 0 on success
 */
int tiz_gmusic_play_situation (tiz_gmusic_t *ap_gmusic,
                               const char *ap_situation);

/**
 * Add the tracks of the specified podcast to the playback queue.
 *
 * After calling this method, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizgmusic
 *
 * @note Currently only available in the US and Canada (free tier, no
 * subscription required).
 *
 * @param ap_gmusic The gmusic handle.
 * @param ap_podcast The podcast
 *
 * @return 0 on success
 */
int tiz_gmusic_play_podcast (tiz_gmusic_t *ap_gmusic, const char *ap_podcast);

/**
* Add Google Play Unlimited promoted tracks to the playback queue.
*
* After calling this method, the various tiz_gmusic_get* methods can be
* used to interact with the playback queue.
*
 * @ingroup libtizgmusic
 *
* @note An Google Play Unlimited subscription is required.
*
* @param ap_gmusic The gmusic handle.
*
* @return 0 on success
*/
int tiz_gmusic_play_promoted_tracks (tiz_gmusic_t *ap_gmusic);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
void tiz_gmusic_clear_queue (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the next track url
 *
 * The the playback queue pointer moves one position forwards.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_next_url (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the previous track url.
 *
 * The the playback queue pointer moves one position backwards.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_prev_url (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's artist.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_artist (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's title.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_title (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's album.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_album (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's duration.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_duration (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's track number.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_track_number (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the number of tracks in the current song's album.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_tracks_in_album (
    tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's publication year.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_year (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's genre.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_genre (tiz_gmusic_t *ap_gmusic);

/**
 * Retrieve the current song's album art.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
const char *tiz_gmusic_get_current_song_album_art (tiz_gmusic_t *ap_gmusic);

/**
 * Destroy the gmusic handle.
 *
 * @ingroup libtizgmusic
 *
 * @param ap_gmusic The gmusic handle.
 */
void tiz_gmusic_destroy (tiz_gmusic_t *ap_gmusic);

#ifdef __cplusplus
}
#endif

#endif  // TIZGMUSIC_C_H

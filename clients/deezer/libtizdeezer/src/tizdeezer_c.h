/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizdeezer_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Deezer client library (c wrapper)
 *
 *
 */

#ifndef TIZDEEZER_C_H
#define TIZDEEZER_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
* @defgroup libtizdeezer 'libtizdeezer' : Tizonia's Deezer client
* library
*
* A C library to access the Deezer streaming service.
*
* @ingroup Tizonia
*/

/**
 * The deezer opaque structure
 * @ingroup libtizdeezer
 */
typedef struct tiz_deezer tiz_deezer_t;
typedef /*@null@ */ tiz_deezer_t *tiz_deezer_ptr_t;

/**
 * Various playback modes that control the playback queue.
 * @ingroup libtizdeezer
 */
typedef enum tiz_deezer_playback_mode {
  ETIZDeezerPlaybackModeNormal,
  ETIZDeezerPlaybackModeShuffle,
  ETIZDeezerPlaybackModeMax
} tiz_deezer_playback_mode_t;

/**
 * Initialize the deezer handle.
 *
 * @ingroup libtizdeezer
 *
 * @param app_deezer A pointer to the deezer handle which will be
 * initialised.
 * @param ap_user A Deezer email account.
 *
 * @return 0 on success.
 */
int tiz_deezer_init (/*@null@ */ tiz_deezer_ptr_t *app_deezer,
                     const char *ap_user);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 */
void tiz_deezer_set_playback_mode (tiz_deezer_t *ap_deezer,
                                   const tiz_deezer_playback_mode_t mode);

/**
 * Add the tracks of the specified search term to the playback queue.
 *
 * After calling this method, the various tiz_deezer_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 * @param ap_tracks The tracks search term.
 *
 * @return 0 on success
 */
int tiz_deezer_play_tracks (tiz_deezer_t *ap_deezer, const char *ap_tracks);

/**
 * Add the tracks of the specified album to the playback queue.
 *
 * After calling this method, the various tiz_deezer_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 * @param ap_album The album name.
 *
 * @return 0 on success
 */
int tiz_deezer_play_album (tiz_deezer_t *ap_deezer, const char *ap_album);

/**
 * Add the tracks of the specified artist to the playback queue.
 *
 * After calling this method, the various tiz_deezer_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 * @param ap_artist The artist name.
 *
 * @return 0 on success
 */
int tiz_deezer_play_artist (tiz_deezer_t *ap_deezer, const char *ap_artist);

/**
 * Add the tracks of the specified mix to the playback queue.
 *
 * After calling this method, the various tiz_deezer_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 * @param ap_mix The mix name.
 *
 * @return 0 on success
 */
int tiz_deezer_play_mix (tiz_deezer_t *ap_deezer, const char *ap_mix);

/**
 * Add the tracks of the specified playlist to the playback queue.
 *
 * After calling this method, the various tiz_deezer_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 * @param ap_playlist The playlist name.
 *
 * @return 0 on success
 */
int tiz_deezer_play_playlist (tiz_deezer_t *ap_deezer, const char *ap_playlist);

/**
 * Add the tracks of the specified user_flow to the playback queue.
 *
 * After calling this method, the various tiz_deezer_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 * @param ap_user_flow The user_flow name.
 *
 * @return 0 on success
 */
int tiz_deezer_play_user_flow (tiz_deezer_t *ap_deezer);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 */
void tiz_deezer_clear_queue (tiz_deezer_t *ap_deezer);

/**
 * Skip to the next track in the queue.
 *
 * The the playback queue pointer moves one position forwards.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 *
 * @return 0 on success
 */
int tiz_deezer_next_track (tiz_deezer_t *ap_deezer);

/**
 * Skip to the previous track in the queue.
 *
 * The the playback queue pointer moves one position backwards.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 *
 * @return 0 on success
 */
int tiz_deezer_prev_track (tiz_deezer_t *ap_deezer);

/**
 * Retrieve a buffer of MP3 data.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 *
 * @return 0 on success
 */
size_t tiz_deezer_get_mp3_data (tiz_deezer_t *ap_deezer,
                                unsigned char **ap_data);

/**
 * Retrieve the current track's title.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 */
const char *tiz_deezer_get_current_track_title (tiz_deezer_t *ap_deezer);

/**
 * Retrieve the current track's artist.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 */
const char *tiz_deezer_get_current_track_artist (tiz_deezer_t *ap_deezer);

/**
* Retrieve the current track's album.
*
* @ingroup libtizdeezer
*
* @param ap_deezer The deezer handle.
*/
const char *tiz_deezer_get_current_track_album (tiz_deezer_t *ap_deezer);

/**
 * Retrieve the current track's duration.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 */
const char *tiz_deezer_get_current_track_duration (tiz_deezer_t *ap_deezer);

/**
 * Retrieve the current track's file_size in MB.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 */
const char *tiz_deezer_get_current_track_file_size_mb (tiz_deezer_t *ap_deezer);

/**
 * Retrieve the current track's file_size in bytes.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 */
int tiz_deezer_get_current_track_file_size_bytes (tiz_deezer_t *ap_deezer);

/**
 * Destroy the deezer handle.
 *
 * @ingroup libtizdeezer
 *
 * @param ap_deezer The deezer handle.
 */
void tiz_deezer_destroy (tiz_deezer_t *ap_deezer);

#ifdef __cplusplus
}
#endif

#endif  // TIZDEEZER_C_H

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
 * @file   tizsoundcloud_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple SoundCloud client library (c wrapper)
 *
 *
 */

#ifndef TIZSOUNDCLOUD_C_H
#define TIZSOUNDCLOUD_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
* @defgroup tizsoundcloud A SoundCloud client library
*
* A C library to access the SoundCloud streaming service.
*
* @ingroup Tizonia
*/

/**
 * The soundcloud opaque structure
 * @ingroup tizsoundcloud
 */
typedef struct tiz_scloud tiz_scloud_t;
typedef /*@null@ */ tiz_scloud_t *tiz_scloud_ptr_t;

/**
 * Various playback modes that control the playback queue.
 * @ingroup tizsoundcloud
 */
typedef enum tiz_scloud_playback_mode
{
  ETIZScloudPlaybackModeNormal,
  ETIZScloudPlaybackModeShuffle,
  ETIZScloudPlaybackModeMax
} tiz_scloud_playback_mode_t;

/**
 * Initialize the soundcloud handle.
 *
 * @ingroup tizsoundcloud
 *
 * @param app_scloud A pointer to the soundcloud handle which will be initialised.
 * @param ap_user A SoundCloud email account.
 * @param ap_pass The password associated to the SoundCloud account.
 *
 * @return 0 on success.
 */
int tiz_scloud_init (/*@null@ */ tiz_scloud_ptr_t *app_scloud,
                     const char *ap_user, const char *ap_pass);

/**
 * Clear the playback queue.
 *
 * @param ap_scloud The soundcloud handle.
 */
void tiz_scloud_set_playback_mode (tiz_scloud_t *ap_scloud,
                                   const tiz_scloud_playback_mode_t mode);

/**
* Add the tracks in the user's streamto the playback queue.
*
* After calling this method, the various tiz_scloud_get* methods can be
* used to interact with the playback queue.
*
* @param ap_scloud The soundcloud handle.
*
* @return 0 on success
*/
int tiz_scloud_play_stream (tiz_scloud_t *ap_scloud);

/**
 * Add the tracks of the specified creator/artist to the playback queue.
 *
 * After calling this method, the various tiz_scloud_get* methods can be
 * used to interact with the playback queue.
 *
 * @param ap_scloud The soundcloud handle.
 * @param ap_creator The creator/artist name
 *
 * @return 0 on success
 */
int tiz_scloud_play_creator (tiz_scloud_t *ap_scloud, const char *ap_creator);

/**
 * Add the tracks of the specified playlist to the playback queue.
 *
 * After calling this method, the various tiz_scloud_get* methods can be
 * used to interact with the playback queue.
 *
 * @param ap_scloud The soundcloud handle.
 * @param ap_playlist The user name
 *
 * @return 0 on success
 */
int tiz_scloud_play_playlist (tiz_scloud_t *ap_scloud, const char *ap_playlist);

/**
 * Clear the playback queue.
 *
 * @param ap_scloud The soundcloud handle.
 */
void tiz_scloud_clear_queue (tiz_scloud_t *ap_scloud);

/**
 * Retrieve the next track url
 *
 * The the playback queue pointer moves one position forwards.
 *
 * @param ap_scloud The soundcloud handle.
 */
const char *tiz_scloud_get_next_url (tiz_scloud_t *ap_scloud);

/**
 * Retrieve the previous track url.
 *
 * The the playback queue pointer moves one position backwards.
 *
 * @param ap_scloud The soundcloud handle.
 */
const char *tiz_scloud_get_prev_url (tiz_scloud_t *ap_scloud);

/**
 * Retrieve the current track's uploader/creator/artist.
 *
 * @param ap_scloud The soundcloud handle.
 */
const char *tiz_scloud_get_current_track_user (tiz_scloud_t *ap_scloud);

/**
 * Retrieve the current track's title.
 *
 * @param ap_scloud The soundcloud handle.
 */
const char *tiz_scloud_get_current_track_title (tiz_scloud_t *ap_scloud);

/**
 * Retrieve the current track's duration.
 *
 * @param ap_scloud The soundcloud handle.
 */
const char *tiz_scloud_get_current_track_duration (tiz_scloud_t *ap_scloud);

/**
 * Retrieve the current track's publication year.
 *
 * @param ap_scloud The soundcloud handle.
 */
const char *tiz_scloud_get_current_track_year (tiz_scloud_t *ap_scloud);

/**
 * Retrieve the current track's permalink.
 *
 * @param ap_scloud The soundcloud handle.
 */
const char *tiz_scloud_get_current_track_permalink (tiz_scloud_t *ap_scloud);

/**
 * Retrieve the current track's license.
 *
 * @param ap_scloud The soundcloud handle.
 */
const char *tiz_scloud_get_current_track_license (tiz_scloud_t *ap_scloud);

/**
 * Destroy the soundcloud handle.
 *
 * @param ap_scloud The soundcloud handle.
 */
void tiz_scloud_destroy (tiz_scloud_t *ap_scloud);

#ifdef __cplusplus
}
#endif

#endif  // TIZSOUNDCLOUD_C_H

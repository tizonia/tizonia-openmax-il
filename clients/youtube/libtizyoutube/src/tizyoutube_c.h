/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizyoutube_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple YouTube audio client library (c wrapper)
 *
 *
 */

#ifndef TIZYOUTUBE_C_H
#define TIZYOUTUBE_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
* @defgroup libtizyoutube 'libtizyoutube' : Tizonia's YouTube audio client
* library
*
* A C library to access audio from the YouTube streaming service.
*
* @ingroup Tizonia
*/

/**
 * The youtube opaque structure
 * @ingroup libtizyoutube
 */
typedef struct tiz_youtube tiz_youtube_t;
typedef /*@null@ */ tiz_youtube_t *tiz_youtube_ptr_t;

/**
 * Various playback modes that control the playback queue.
 * @ingroup libtizyoutube
 */
typedef enum tiz_youtube_playback_mode {
  ETIZYouTubePlaybackModeNormal,
  ETIZYouTubePlaybackModeShuffle,
  ETIZYouTubePlaybackModeMax
} tiz_youtube_playback_mode_t;

/**
 * Initialize the tiz_youtube handle.
 *
 * @ingroup libtizyoutube
 *
 * @param app_youtube A pointer to the tiz_youtube handle which will be
 * initialised.
 * @param ap_api_key A YouTube email account.
 *
 * @return 0 on success.
 */
int tiz_youtube_init (/*@null@ */ tiz_youtube_ptr_t *app_youtube,
                      const char *ap_api_key);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 */
void tiz_youtube_set_playback_mode (tiz_youtube_t *ap_youtube,
                                    const tiz_youtube_playback_mode_t mode);

/**
 * Add a YouTube audio stream to the playback queue.
 *
 * After calling this method, the various tiz_youtube_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 * @param ap_url_or_id A YouTube video full url or video id.
 *
 * @return 0 on success
 */
int tiz_youtube_play_audio_stream (tiz_youtube_t *ap_youtube,
                                   const char *ap_url_or_id);

/**
 * Add all the audio streams of a YouTube playlist to the playback queue.
 *
 * After calling this method, the various tiz_youtube_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 * @param ap_url_or_id A playlist full url or playlist id.
 *
 * @return 0 on success
 */
int tiz_youtube_play_audio_playlist (tiz_youtube_t *ap_youtube,
                                     const char *ap_url_or_id);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 */
void tiz_youtube_clear_queue (tiz_youtube_t *ap_youtube);

/**
 * Retrieve the next stream url
 *
 * The playback queue pointer moves one position forwards.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 * @param a_remove_current_url If true, delete the current url from the
 * playback queue before moving to the next url.
 *
 * @return The next url in the playback queue or NULL if the playback queue is
 * empty.
 */
const char *tiz_youtube_get_next_url (tiz_youtube_t *ap_youtube,
                                      const bool a_remove_current_url);

/**
 * Retrieve the previous stream url.
 *
 * The playback queue pointer moves one position backwards.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 * @param a_remove_current_url If true, delete the current url from the
 * playback queue before moving to the previous url.
 *
 * @return The previous url in the playback queue or NULL if the playback queue
 * is empty.
 */
const char *tiz_youtube_get_prev_url (tiz_youtube_t *ap_youtube,
                                      const bool a_remove_current_url);

/**
 * Destroy the tiz_youtube handle.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 */
void tiz_youtube_destroy (tiz_youtube_t *ap_youtube);

#ifdef __cplusplus
}
#endif

#endif  // TIZYOUTUBE_C_H

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
  ETIZYoutubePlaybackModeNormal,
  ETIZYoutubePlaybackModeShuffle,
  ETIZYoutubePlaybackModeMax
} tiz_youtube_playback_mode_t;

/**
 * Initialize the tiz_youtube handle.
 *
 * @ingroup libtizyoutube
 *
 * @param app_youtube A pointer to the tiz_youtube handle which will be
 * initialised.
 *
 * @return 0 on success.
 */
int tiz_youtube_init (/*@null@ */ tiz_youtube_ptr_t *app_youtube);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 */
void tiz_youtube_clear_queue (tiz_youtube_t *ap_youtube);

/**
 * Retrieve the index in the playback queue of the stream currently selected.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_index (tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current length of playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_queue_length (tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current queue progress (e.g. '5 of 17', where 5 is the current
 * stream index, and 17 is the total streams in the queue).
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_queue_progress (tiz_youtube_t *ap_youtube);

/**
 * Set the playback mode (normal, shuffle).
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
 * Retrieve the YouTube mix associated to given video id or url andd all the
 * audio streams of the mix to the playback queue.
 *
 * After calling this method, the various tiz_youtube_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 * @param ap_url_or_id A playlist url or playlist id.
 *
 * @return 0 on success
 */
int tiz_youtube_play_audio_mix (tiz_youtube_t *ap_youtube,
                                const char *ap_url_or_id);

/**
 * Add all the audio streams of a YouTube search to the playback queue.
 *
 * After calling this method, the various tiz_youtube_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 * @param ap_search A search term.
 *
 * @return 0 on success
 */
int tiz_youtube_play_audio_search (tiz_youtube_t *ap_youtube,
                                   const char *ap_search);

/**
 * Retrieve the YouTube mix associated to given search term and add all the
 * audio streams of the mix to the playback queue.
 *
 * After calling this method, the various tiz_youtube_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 * @param ap_search A search term.
 *
 * @return 0 on success
 */
int tiz_youtube_play_audio_mix_search (tiz_youtube_t *ap_youtube,
                                       const char *ap_search);

/**
 * Retrieve all video uploads from a YouTube channel and add all the
 * audio streams to the playback queue.
 *
 * After calling this method, the various tiz_youtube_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_youtube The tiz_youtube handle.
 * @param ap_channel A channel url.
 *
 * @return 0 on success
 */
int tiz_youtube_play_audio_channel_uploads (tiz_youtube_t *ap_youtube,
                                            const char *ap_channel);

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
 * Retrieve the current audio stream's title.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_title (
    tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current audio stream's author.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_author (
    tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current audio stream's file size.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_file_size (
    tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current audio stream's duration.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_duration (
    tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current audio stream's bitrate.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_bitrate (
    tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current audio stream's view count.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_view_count (
    tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current audio stream's description.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_description (
    tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current audio stream's file extension.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_file_extension (
    tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current streams video id.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_video_id (
    tiz_youtube_t *ap_youtube);

/**
 * Retrieve the current streams upload date and time.
 *
 * @ingroup libtizyoutube
 *
 * @param ap_gmusic The tiz_youtube handle.
 */
const char *tiz_youtube_get_current_audio_stream_published (
    tiz_youtube_t *ap_youtube);

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

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
 * @file   tizpandora_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Pandora client library (c wrapper)
 *
 *
 */

#ifndef TIZPANDORA_C_H
#define TIZPANDORA_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @defgroup libtizpandora 'libtizpandora' : Tizonia's Pandora client
 * library
 *
 * A C library to access audio streams from Pandora.
 *
 * @ingroup Tizonia
 */

/**
 * The pandora opaque structure
 * @ingroup libtizpandora
 */
typedef struct tiz_pandora tiz_pandora_t;
typedef /*@null@ */ tiz_pandora_t *tiz_pandora_ptr_t;

/**
 * Various playback modes that control the playback queue.
 * @ingroup libtizpandora
 */
typedef enum tiz_pandora_playback_mode {
  ETIZPandoraPlaybackModeNormal,
  ETIZPandoraPlaybackModeShuffle,
  ETIZPandoraPlaybackModeMax
} tiz_pandora_playback_mode_t;

/**
 * Initialize the tiz_pandora handle.
 *
 * @ingroup libtizpandora
 *
 * @param app_pandora A pointer to the tiz_pandora handle which will be
 * initialised.
 * @param ap_base_url The Pandora server base url.
 * @param ap_auth_token The Pandora account auth token.
 *
 * @return 0 on success.
 */
int tiz_pandora_init (/*@null@ */ tiz_pandora_ptr_t *app_pandora,
                   const char *ap_base_url, const char *ap_auth_token);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
void tiz_pandora_clear_queue (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the index in the playback queue of the stream currently selected.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_track_index (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current length of playback queue.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_queue_length (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current queue progress (e.g. '5 of 17', where 5 is the current
 * stream index, and 17 is the total streams in the queue).
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_queue_progress (tiz_pandora_t *ap_pandora);

/**
 * Set the playback mode (normal, shuffle).
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
void tiz_pandora_set_playback_mode (tiz_pandora_t *ap_pandora,
                                 const tiz_pandora_playback_mode_t mode);

/**
 * Add a Pandora stream to the playback queue.
 *
 * After calling this method, the various tiz_pandora_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 * @param ap_track_name An search term.
 *
 * @return 0 on success
 */
int tiz_pandora_play_tracks (tiz_pandora_t *ap_pandora, const char *ap_track_name);

/**
 * Add a Pandora stream to the playback queue.
 *
 * After calling this method, the various tiz_pandora_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 * @param ap_artist_name An search term.
 *
 * @return 0 on success
 */
int tiz_pandora_play_artist (tiz_pandora_t *ap_pandora,
                                const char *ap_artist_name);

/**
 * Add all the streams of a Pandora playlist to the playback queue.
 *
 * After calling this method, the various tiz_pandora_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 * @param ap_album A playlist full url or playlist id.
 *
 * @return 0 on success
 */
int tiz_pandora_play_album (tiz_pandora_t *ap_pandora, const char *ap_album);

/**
 * Add all the streams of a Pandora playlist to the playback queue.
 *
 * After calling this method, the various tiz_pandora_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 * @param ap_playlist A playlist full url or playlist id.
 *
 * @return 0 on success
 */
int tiz_pandora_play_playlist (tiz_pandora_t *ap_pandora, const char *ap_playlist);

/**
 * Retrieve the next stream url
 *
 * The playback queue pointer moves one position forwards.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 * @param a_remove_current_url If true, delete the current url from the
 * playback queue before moving to the next url.
 *
 * @return The next url in the playback queue or NULL if the playback queue is
 * empty.
 */
const char *tiz_pandora_get_next_url (tiz_pandora_t *ap_pandora,
                                   const bool a_remove_current_url);

/**
 * Retrieve the previous stream url.
 *
 * The playback queue pointer moves one position backwards.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 * @param a_remove_current_url If true, delete the current url from the
 * playback queue before moving to the previous url.
 *
 * @return The previous url in the playback queue or NULL if the playback queue
 * is empty.
 */
const char *tiz_pandora_get_prev_url (tiz_pandora_t *ap_pandora,
                                   const bool a_remove_current_url);

/**
 * Retrieve the current track's title.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_track_title (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current track's artist.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_track_artist (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current track's album.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_track_album (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current track's publication year.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_track_year (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current track's file size.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_track_file_size (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current track's file size (as int).
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
int tiz_pandora_get_current_track_file_size_as_int (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current track's duration.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_track_duration (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current track's bitrate.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_track_bitrate (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current track's codec id.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
const char *tiz_pandora_get_current_track_codec (tiz_pandora_t *ap_pandora);

/**
 * Retrieve the current track's album art.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The pandora handle.
 */
const char *tiz_pandora_get_current_track_album_art (tiz_pandora_t *ap_pandora);

/**
 * Destroy the tiz_pandora handle.
 *
 * @ingroup libtizpandora
 *
 * @param ap_pandora The tiz_pandora handle.
 */
void tiz_pandora_destroy (tiz_pandora_t *ap_pandora);

#ifdef __cplusplus
}
#endif

#endif  // TIZPANDORA_C_H

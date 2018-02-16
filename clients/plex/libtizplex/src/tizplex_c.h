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
 * @file   tizplex_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Plex audio client library (c wrapper)
 *
 *
 */

#ifndef TIZPLEX_C_H
#define TIZPLEX_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @defgroup libtizplex 'libtizplex' : Tizonia's Plex audio client
 * library
 *
 * A C library to access audio from a Plex server.
 *
 * @ingroup Tizonia
 */

/**
 * The plex opaque structure
 * @ingroup libtizplex
 */
typedef struct tiz_plex tiz_plex_t;
typedef /*@null@ */ tiz_plex_t *tiz_plex_ptr_t;

/**
 * Various playback modes that control the playback queue.
 * @ingroup libtizplex
 */
typedef enum tiz_plex_playback_mode {
  ETIZPlexPlaybackModeNormal,
  ETIZPlexPlaybackModeShuffle,
  ETIZPlexPlaybackModeMax
} tiz_plex_playback_mode_t;

/**
 * Initialize the tiz_plex handle.
 *
 * @ingroup libtizplex
 *
 * @param app_plex A pointer to the tiz_plex handle which will be
 * initialised.
 * @param ap_base_url The Plex server base url.
 * @param ap_auth_token The Plex account auth token.
 *
 * @return 0 on success.
 */
int tiz_plex_init (/*@null@ */ tiz_plex_ptr_t *app_plex,
                   const char *ap_base_url, const char *ap_auth_token);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
void tiz_plex_clear_queue (tiz_plex_t *ap_plex);

/**
 * Retrieve the index in the playback queue of the stream currently selected.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_audio_track_index (tiz_plex_t *ap_plex);

/**
 * Retrieve the current length of playback queue.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_queue_length (tiz_plex_t *ap_plex);

/**
 * Retrieve the current queue progress (e.g. '5 of 17', where 5 is the current
 * stream index, and 17 is the total streams in the queue).
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_queue_progress (tiz_plex_t *ap_plex);

/**
 * Set the playback mode (normal, shuffle).
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
void tiz_plex_set_playback_mode (tiz_plex_t *ap_plex,
                                 const tiz_plex_playback_mode_t mode);

/**
 * Add a Plex audio stream to the playback queue.
 *
 * After calling this method, the various tiz_plex_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 * @param ap_track_name An search term.
 *
 * @return 0 on success
 */
int tiz_plex_play_audio_tracks (tiz_plex_t *ap_plex, const char *ap_track_name);

/**
 * Add a Plex audio stream to the playback queue.
 *
 * After calling this method, the various tiz_plex_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 * @param ap_artist_name An search term.
 *
 * @return 0 on success
 */
int tiz_plex_play_audio_artist (tiz_plex_t *ap_plex,
                                const char *ap_artist_name);

/**
 * Add all the audio streams of a Plex playlist to the playback queue.
 *
 * After calling this method, the various tiz_plex_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 * @param ap_album A playlist full url or playlist id.
 *
 * @return 0 on success
 */
int tiz_plex_play_audio_album (tiz_plex_t *ap_plex, const char *ap_album);

/**
 * Add all the audio streams of a Plex playlist to the playback queue.
 *
 * After calling this method, the various tiz_plex_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 * @param ap_playlist A playlist full url or playlist id.
 *
 * @return 0 on success
 */
int tiz_plex_play_audio_playlist (tiz_plex_t *ap_plex, const char *ap_playlist);

/**
 * Retrieve the next stream url
 *
 * The playback queue pointer moves one position forwards.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 * @param a_remove_current_url If true, delete the current url from the
 * playback queue before moving to the next url.
 *
 * @return The next url in the playback queue or NULL if the playback queue is
 * empty.
 */
const char *tiz_plex_get_next_url (tiz_plex_t *ap_plex,
                                   const bool a_remove_current_url);

/**
 * Retrieve the previous stream url.
 *
 * The playback queue pointer moves one position backwards.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 * @param a_remove_current_url If true, delete the current url from the
 * playback queue before moving to the previous url.
 *
 * @return The previous url in the playback queue or NULL if the playback queue
 * is empty.
 */
const char *tiz_plex_get_prev_url (tiz_plex_t *ap_plex,
                                   const bool a_remove_current_url);

/**
 * Retrieve the current audio track's title.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_audio_track_title (tiz_plex_t *ap_plex);

/**
 * Retrieve the current audio track's artist.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_audio_track_artist (tiz_plex_t *ap_plex);

/**
 * Retrieve the current audio track's album.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_audio_track_album (tiz_plex_t *ap_plex);

/**
 * Retrieve the current audio track's publication year.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_audio_track_year (tiz_plex_t *ap_plex);

/**
 * Retrieve the current audio track's file size.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_audio_track_file_size (tiz_plex_t *ap_plex);

/**
 * Retrieve the current audio track's duration.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_audio_track_duration (tiz_plex_t *ap_plex);

/**
 * Retrieve the current audio track's bitrate.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_audio_track_bitrate (tiz_plex_t *ap_plex);

/**
 * Retrieve the current audio track's codec id.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
const char *tiz_plex_get_current_audio_track_codec (tiz_plex_t *ap_plex);

/**
 * Retrieve the current track's album art.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The plex handle.
 */
const char *tiz_plex_get_current_audio_track_album_art (tiz_plex_t *ap_plex);

/**
 * Destroy the tiz_plex handle.
 *
 * @ingroup libtizplex
 *
 * @param ap_plex The tiz_plex handle.
 */
void tiz_plex_destroy (tiz_plex_t *ap_plex);

#ifdef __cplusplus
}
#endif

#endif  // TIZPLEX_C_H

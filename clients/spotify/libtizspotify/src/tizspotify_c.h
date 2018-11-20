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
 * @file   tizspotify_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Spotify Web client library (c wrapper)
 *
 *
 */

#ifndef TIZSPOTIFY_C_H
#define TIZSPOTIFY_C_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

  /**
   * @defgroup libtizspotify 'libtizspotify' : Tizonia's Spotify Web client
   * library
   *
   * A C library to access metadata from Spotify.
   *
   * @ingroup Tizonia
   */

  /**
   * The spotify opaque structure
   * @ingroup libtizspotify
   */
  typedef struct tiz_spotify tiz_spotify_t;
  typedef /*@null@ */ tiz_spotify_t *tiz_spotify_ptr_t;

  /**
   * Various playback modes that control the playback queue.
   * @ingroup libtizspotify
   */
  typedef enum tiz_spotify_playback_mode
  {
    ETIZSpotifyPlaybackModeNormal,
    ETIZSpotifyPlaybackModeShuffle,
    ETIZSpotifyPlaybackModeMax
  } tiz_spotify_playback_mode_t;

  /**
   * Initialize the tiz_spotify handle.
   *
   * @ingroup libtizspotify
   *
   * @param app_spotify A pointer to the tiz_spotify handle which will be
   * initialised.
   *
   * @return 0 on success.
   */
  int tiz_spotify_init (/*@null@ */ tiz_spotify_ptr_t *app_spotify);

  /**
   * Clear the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  void tiz_spotify_clear_queue (tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the index in the playback queue of the stream currently selected.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_track_index (tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current length of the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_queue_length (tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current length of the playback queue as integer.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  int tiz_spotify_get_current_queue_length_as_int (tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current queue progress (e.g. '5 of 17', where 5 is the current
   * stream index, and 17 is the total streams in the queue).
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_queue_progress (
      tiz_spotify_t *ap_spotify);

  /**
   * Set the playback mode (normal, shuffle).
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  void tiz_spotify_set_playback_mode (tiz_spotify_t *ap_spotify,
                                      const tiz_spotify_playback_mode_t mode);

  /**
   * Search tracks in Spotify and add them to the playback queue.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_track_name A track name or search term.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_tracks (tiz_spotify_t *ap_spotify,
                               const char *ap_track_name);

  /**
   * Search Spotify for an artist and add the artist's tracks to the playback
   * queue.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_artist_name An artist name or search term.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_artist (tiz_spotify_t *ap_spotify,
                               const char *ap_artist_name);

  /**
   * Search Spotify for an album and add all its tracks to the playback queue.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_album An album name or search term.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_album (tiz_spotify_t *ap_spotify, const char *ap_album);

  /**
   * Searh Spotify for a playlist and add all its tracks to the playback queue.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_playlist A playlist name or search term.
   * @param ap_owner The Spotify username of the owner of the playlist.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_playlist (tiz_spotify_t *ap_spotify,
                                 const char *ap_playlist, const char *ap_owner);

  /**
   * Retrieve the next stream uri.
   *
   * The playback queue pointer moves one position forwards.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param a_remove_current_uri If true, delete the current uri from the
   * playback queue before moving to the next uri.
   *
   * @return The next uri in the playback queue or NULL if the playback queue is
   * empty.
   */
  const char *tiz_spotify_get_next_uri (tiz_spotify_t *ap_spotify,
                                        const bool a_remove_current_uri);

  /**
   * Retrieve the previous stream uri.
   *
   * The playback queue pointer moves one position backwards.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param a_remove_current_uri If true, delete the current uri from the
   * playback queue before moving to the previous uri.
   *
   * @return The previous uri in the playback queue or NULL if the playback
   * queue is empty.
   */
  const char *tiz_spotify_get_prev_uri (tiz_spotify_t *ap_spotify,
                                        const bool a_remove_current_uri);

  /**
   * Retrieve the current track's title.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_track_title (tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current track's artist.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_track_artist (tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current track's album.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_track_album (tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current track's publication date.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_track_release_date (
      tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current track's duration.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_track_duration (
      tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current track's album art.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The spotify handle.
   */
  const char *tiz_spotify_get_current_track_album_art (
      tiz_spotify_t *ap_spotify);

  /**
   * Destroy the tiz_spotify handle.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  void tiz_spotify_destroy (tiz_spotify_t *ap_spotify);

#ifdef __cplusplus
}
#endif

#endif  // TIZSPOTIFY_C_H

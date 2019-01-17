/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
   * Search Spotify for a playlist and add all its tracks to the playback queue.
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
   * Add a track to the playback queue using a Spotify track ID,
   * URI, or URL.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_track_id A Spotify track ID, URI, or URL.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_track_by_id (tiz_spotify_t *ap_spotify,
                                    const char *ap_track_id);

  /**
   * Add an artist's tracks to the playback queue using a Spotify artist ID,
   * URI, or URL.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_artist_id A Spotify artist ID, URI, or URL.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_artist_by_id (tiz_spotify_t *ap_spotify,
                                     const char *ap_artist_id);

  /**
   * Add an album tracks to the playback queue using a Spotify album ID,
   * URI, or URL.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_album_id A Spotify album ID, URI, or URL.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_album_by_id (tiz_spotify_t *ap_spotify,
                                    const char *ap_album_id);

  /**
   * Add a playlist tracks to the playback queue using a Spotify playlist ID,
   * URI, or URL.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_playlist_id A Spotify playlist ID, URI, or URL.
   * @param ap_owner The Spotify username of the owner of the playlist.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_playlist_by_id (tiz_spotify_t *ap_spotify,
                                       const char *ap_playlist_id,
                                       const char *ap_owner);

  /**
   * Search Spotify for artists related to a given artist and add their top
   * tracks to the playback queue.
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
  int tiz_spotify_play_related_artists (tiz_spotify_t *ap_spotify,
                                        const char *ap_artist_name);

  /**
   * Search Spotify for a featured playlist and add its top tracks to the
   * playback queue.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_playlist_name A playlist name or search term.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_featured_playlist (tiz_spotify_t *ap_spotify,
                                          const char *ap_playlist_name);

  /**
   * Search Spotify for a newly released album and add its top tracks to the
   * playback queue.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_album_name A album name or search term.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_new_releases (tiz_spotify_t *ap_spotify,
                                     const char *ap_album_name);

  /**
   * Find Spotify recommendations by track ID, URI, or URL and add tracks to
   * the playback queue.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_track_id A Spotify track ID, URI, or URL.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_recommendations_by_track_id (tiz_spotify_t *ap_spotify,
                                                    const char *ap_track_id);

  /**
   * Find Spotify recommendations by artist ID, URI, or URL and add artists to
   * the playback queue.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_artist_id A Spotify artist ID, URI, or URL.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_recommendations_by_artist_id (tiz_spotify_t *ap_spotify,
                                                     const char *ap_artist_id);

  /**
   * Find Spotify recommendations by genre and add artists to the playback
   * queue.
   *
   * After calling this method, the various tiz_spotify_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   * @param ap_artist_id A genre name or search term.
   *
   * @return 0 on success
   */
  int tiz_spotify_play_recommendations_by_genre (tiz_spotify_t *ap_spotify,
                                                 const char *ap_genre);

  /**
   * Retrieve the next stream url.
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
   * Retrieve the previous stream url.
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
   * Retrieve the Spotify URI of the current track.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The spotify handle.
   */
  const char *tiz_spotify_get_current_track_uri (
      tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current track's artist URI.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_track_artist_uri (tiz_spotify_t *ap_spotify);

  /**
   * Retrieve the current track's album URI.
   *
   * @ingroup libtizspotify
   *
   * @param ap_spotify The tiz_spotify handle.
   */
  const char *tiz_spotify_get_current_track_album_uri (tiz_spotify_t *ap_spotify);

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

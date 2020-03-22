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
 * @file   tiziheart_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Iheart client library (c wrapper)
 *
 *
 */

#ifndef TIZIHEART_C_H
#define TIZIHEART_C_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

  /**
   * @defgroup libtiziheart 'libtiziheart' : Tizonia's Iheart client
   * library
   *
   * A C library to access the Iheart streaming service.
   *
   * @ingroup Tizonia
   */

  /**
   * The iheart opaque structure
   * @ingroup libtiziheart
   */
  typedef struct tiz_iheart tiz_iheart_t;
  typedef /*@null@ */ tiz_iheart_t *tiz_iheart_ptr_t;

  /**
   * Various playback modes that control the playback queue.
   * @ingroup libtiziheart
   */
  typedef enum tiz_iheart_playback_mode
  {
    ETIZIheartPlaybackModeNormal,
    ETIZIheartPlaybackModeShuffle,
    ETIZIheartPlaybackModeMax
  } tiz_iheart_playback_mode_t;

  /**
   * Various search modes that act as a way to filter the elements that are
   * added to the queue.
   *
   * @ingroup libtiziheart
   */
  typedef enum tiz_iheart_search_mode
  {
    ETIZIheartSearchModeAll,
    ETIZIheartSearchModeStations,
    ETIZIheartSearchModeShows,
    ETIZIheartSearchModeMax
  } tiz_iheart_search_mode_t;

  /**
   * Initialize the iheart handle.
   *
   * @ingroup libtiziheart
   *
   * @param app_iheart A pointer to the iheart handle which will be
   * initialised.
   *
   * @return 0 on success.
   */
  int tiz_iheart_init (/*@null@ */ tiz_iheart_ptr_t *app_iheart);

  /**
   * Set the playback mode.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   * @param a_mode The playback mode.
   */
  void tiz_iheart_set_playback_mode (tiz_iheart_t *ap_iheart,
                                     const tiz_iheart_playback_mode_t a_mode);

  /**
   * Set the search mode.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   * @param a_mode The search mode.
   */
  void tiz_iheart_set_search_mode (tiz_iheart_t *ap_iheart,
                                   const tiz_iheart_search_mode_t mode);

  /**
   * Add radio stations/shows matching the query string to the
   * playback queue.
   *
   * After calling this method, the various tiz_iheart_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   * @param ap_query The main query string.
   * @param ap_keywords1 First set of optional keywords.
   * @param ap_keywords2 Second set of optional keywords.
   * @param ap_keywords3 Third set of optional keywords.
   *
   * @return 0 on success
   */
  int tiz_iheart_play_radios (tiz_iheart_t *ap_iheart, const char *ap_query,
                              const char *ap_keywords1,
                              const char *ap_keywords2,
                              const char *ap_keywords3);

  /**
   * Clear the playback queue.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  void tiz_iheart_clear_queue (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the index in the playback queue of the stream currently selected.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The tiz_iheart handle.
   */
  const char *tiz_iheart_get_current_radio_index (
      tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current length of playback queue.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The tiz_iheart handle.
   */
  const char *tiz_iheart_get_current_queue_length (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current length of playback queue.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The tiz_iheart handle.
   */
  int tiz_iheart_get_current_queue_length_as_int (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current queue progress (e.g. '5 of 17', where 5 is the current
   * stream index, and 17 is the total streams in the queue).
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The tiz_iheart handle.
   */
  const char *tiz_iheart_get_current_queue_progress (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the next station url
   *
   * The playback queue pointer moves one position forwards.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   * @param a_remove_current_url If true, delete the current url from the
   * playback queue before moving to the next url.
   *
   * @return The next url in the playback queue or NULL if the playback queue is
   * empty.
   */
  const char *tiz_iheart_get_next_url (tiz_iheart_t *ap_iheart,
                                       const bool a_remove_current_url);

  /**
   * Retrieve the previous station url.
   *
   * The playback queue pointer moves one position backwards.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   * @param a_remove_current_url If true, delete the current url from the
   * playback queue before moving to the previous url.
   *
   * @return The previous url in the playback queue or NULL if the playback
   * queue is empty.
   */
  const char *tiz_iheart_get_prev_url (tiz_iheart_t *ap_iheart,
                                       const bool a_remove_current_url);

  /**
   * Retrieve the current station's name.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  const char *tiz_iheart_get_current_radio_name (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current station's description.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  const char *tiz_iheart_get_current_radio_description (
      tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current station's reliability.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  const char *tiz_iheart_get_current_radio_reliability (
      tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the type of the current radio (usually 'station' or 'show').
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  const char *tiz_iheart_get_current_radio_type (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current station's website.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  const char *tiz_iheart_get_current_radio_website (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current station's bitrate.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  const char *tiz_iheart_get_current_radio_bitrate (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current station's format.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  const char *tiz_iheart_get_current_radio_format (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current station's stream url.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  const char *tiz_iheart_get_current_radio_stream_url (tiz_iheart_t *ap_iheart);

  /**
   * Retrieve the current station's thumbnail image url.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */
  const char *tiz_iheart_get_current_radio_thumbnail_url (
      tiz_iheart_t *ap_iheart);

  /**
   * Destroy the iheart handle.
   *
   * @ingroup libtiziheart
   *
   * @param ap_iheart The iheart handle.
   */

  void tiz_iheart_destroy (tiz_iheart_t *ap_iheart);

#ifdef __cplusplus
}
#endif

#endif  // TIZIHEART_C_H

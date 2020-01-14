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
 * @file   tiztunein_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Tunein client library (c wrapper)
 *
 *
 */

#ifndef TIZTUNEIN_C_H
#define TIZTUNEIN_C_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

  /**
   * @defgroup libtiztunein 'libtiztunein' : Tizonia's Tunein client
   * library
   *
   * A C library to access the Tunein streaming service.
   *
   * @ingroup Tizonia
   */

  /**
   * The tunein opaque structure
   * @ingroup libtiztunein
   */
  typedef struct tiz_tunein tiz_tunein_t;
  typedef /*@null@ */ tiz_tunein_t *tiz_tunein_ptr_t;

  /**
   * Various playback modes that control the playback queue.
   * @ingroup libtiztunein
   */
  typedef enum tiz_tunein_playback_mode
  {
    ETIZTuneinPlaybackModeNormal,
    ETIZTuneinPlaybackModeShuffle,
    ETIZTuneinPlaybackModeMax
  } tiz_tunein_playback_mode_t;

  /**
   * Various search modes that act as a way to filter the elements that are
   * added to the queue.
   *
   * @ingroup libtiztunein
   */
  typedef enum tiz_tunein_search_mode
  {
    ETIZTuneinSearchModeAll,
    ETIZTuneinSearchModeStations,
    ETIZTuneinSearchModeShows,
    ETIZTuneinSearchModeMax
  } tiz_tunein_search_mode_t;

  /**
   * Initialize the tunein handle.
   *
   * @ingroup libtiztunein
   *
   * @param app_tunein A pointer to the tunein handle which will be
   * initialised.
   *
   * @return 0 on success.
   */
  int tiz_tunein_init (/*@null@ */ tiz_tunein_ptr_t *app_tunein);

  /**
   * Set the playback mode.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   * @param a_mode The playback mode.
   */
  void tiz_tunein_set_playback_mode (tiz_tunein_t *ap_tunein,
                                     const tiz_tunein_playback_mode_t a_mode);

  /**
   * Set the search mode.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   * @param a_mode The search mode.
   */
  void tiz_tunein_set_search_mode (tiz_tunein_t *ap_tunein,
                                   const tiz_tunein_search_mode_t mode);

  /**
   * Add radio stations/shows matching the query string to the
   * playback queue.
   *
   * After calling this method, the various tiz_tunein_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   * @param ap_query The main query string.
   * @param ap_keywords1 First set of optional keywords.
   * @param ap_keywords2 Second set of optional keywords.
   * @param ap_keywords3 Third set of optional keywords.
   *
   * @return 0 on success
   */
  int tiz_tunein_play_radios (tiz_tunein_t *ap_tunein, const char *ap_query,
                              const char *ap_keywords1,
                              const char *ap_keywords2,
                              const char *ap_keywords3);

  /**
   * Search Tunein for a category and add matching stations to the playback
   * queue.
   *
   * After calling this method, the various tiz_tunein_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   * @param ap_category The category.
   * @param ap_keywords1 First set of optional keywords.
   * @param ap_keywords2 Second set of optional keywords.
   * @param ap_keywords3 Third set of optional keywords.
   *
   * @return 0 on success
   */
  int tiz_tunein_play_category (tiz_tunein_t *ap_tunein,
                                const char *ap_category,
                                const char *ap_keywords1,
                                const char *ap_keywords2,
                                const char *ap_keywords3);

  /**
   * Clear the playback queue.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  void tiz_tunein_clear_queue (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the index in the playback queue of the stream currently selected.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tiz_tunein handle.
   */
  const char *tiz_tunein_get_current_radio_index (
      tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current length of playback queue.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tiz_tunein handle.
   */
  const char *tiz_tunein_get_current_queue_length (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current length of playback queue.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tiz_tunein handle.
   */
  int tiz_tunein_get_current_queue_length_as_int (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current queue progress (e.g. '5 of 17', where 5 is the current
   * stream index, and 17 is the total streams in the queue).
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tiz_tunein handle.
   */
  const char *tiz_tunein_get_current_queue_progress (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the next station url
   *
   * The playback queue pointer moves one position forwards.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   * @param a_remove_current_url If true, delete the current url from the
   * playback queue before moving to the next url.
   *
   * @return The next url in the playback queue or NULL if the playback queue is
   * empty.
   */
  const char *tiz_tunein_get_next_url (tiz_tunein_t *ap_tunein,
                                       const bool a_remove_current_url);

  /**
   * Retrieve the previous station url.
   *
   * The playback queue pointer moves one position backwards.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   * @param a_remove_current_url If true, delete the current url from the
   * playback queue before moving to the previous url.
   *
   * @return The previous url in the playback queue or NULL if the playback
   * queue is empty.
   */
  const char *tiz_tunein_get_prev_url (tiz_tunein_t *ap_tunein,
                                       const bool a_remove_current_url);

  /**
   * Retrieve the current station's name.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_name (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current station's description.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_description (
      tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current station's reliability.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_reliability (
      tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the type of the current radio (usually 'station' or 'show').
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_type (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current station's website.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_website (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current station's bitrate.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_bitrate (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current station's format.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_format (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current station's stream url.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_stream_url (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current station's thumbnail image url.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_thumbnail_url (
      tiz_tunein_t *ap_tunein);

  /**
   * Destroy the tunein handle.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */

  void tiz_tunein_destroy (tiz_tunein_t *ap_tunein);

#ifdef __cplusplus
}
#endif

#endif  // TIZTUNEIN_C_H

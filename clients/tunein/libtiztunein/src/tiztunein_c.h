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
   * Clear the playback queue.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  void tiz_tunein_set_playback_mode (tiz_tunein_t *ap_tunein,
                                     const tiz_tunein_playback_mode_t mode);

  /**
   * Add radio stations or podcast shows matching the query string to the
   * playback queue.
   *
   * After calling this method, the various tiz_tunein_get* methods can be
   * used to interact with the playback queue.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   * @param ap_query The query string.
   *
   * @return 0 on success
   */
  int tiz_tunein_play_radios (tiz_tunein_t *ap_tunein, const char *ap_query);

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
   *
   * @return 0 on success
   */
  int tiz_tunein_play_category (tiz_tunein_t *ap_tunein,
                                const char *ap_category);

  /**
   * Clear the playback queue.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  void tiz_tunein_clear_queue (tiz_tunein_t *ap_tunein);

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
   * Retrieve the current station's country.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_country (tiz_tunein_t *ap_tunein);

  /**
   * Retrieve the current station's category.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_category (tiz_tunein_t *ap_tunein);

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
   * Retrieve the current station's stream url.
   *
   * @ingroup libtiztunein
   *
   * @param ap_tunein The tunein handle.
   */
  const char *tiz_tunein_get_current_radio_stream_url (
      tiz_tunein_t *ap_tunein);

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

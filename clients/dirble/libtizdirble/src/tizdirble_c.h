/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizdirble_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Dirble client library (c wrapper)
 *
 *
 */

#ifndef TIZDIRBLE_C_H
#define TIZDIRBLE_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
* @defgroup libtizdirble 'libtizdirble' : Tizonia's Dirble client
* library
*
* A C library to access the Dirble streaming service.
*
* @ingroup Tizonia
*/

/**
 * The dirble opaque structure
 * @ingroup libtizdirble
 */
typedef struct tiz_dirble tiz_dirble_t;
typedef /*@null@ */ tiz_dirble_t *tiz_dirble_ptr_t;

/**
 * Various playback modes that control the playback queue.
 * @ingroup libtizdirble
 */
typedef enum tiz_dirble_playback_mode
{
  ETIZDirblePlaybackModeNormal,
  ETIZDirblePlaybackModeShuffle,
  ETIZDirblePlaybackModeMax
} tiz_dirble_playback_mode_t;

/**
 * Initialize the dirble handle.
 *
 * @ingroup libtizdirble
 *
 * @param app_dirble A pointer to the dirble handle which will be
 * initialised.
 * @param ap_api_key A Dirble email account.
 *
 * @return 0 on success.
 */
int tiz_dirble_init (/*@null@ */ tiz_dirble_ptr_t *app_dirble,
                     const char *ap_api_key);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 */
void tiz_dirble_set_playback_mode (tiz_dirble_t *ap_dirble,
                                   const tiz_dirble_playback_mode_t mode);

/**
 * Add popular stations to the playback queue.
 *
 * After calling this method, the various tiz_dirble_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 *
 * @return 0 on success
 */
int tiz_dirble_play_popular_stations (tiz_dirble_t *ap_dirble);

/**
 * Search Dirble for stations matching the query string and add them to the
 * playback queue.
 *
 * After calling this method, the various tiz_dirble_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 * @param ap_query The query string.
 *
 * @return 0 on success
 */
int tiz_dirble_play_stations (tiz_dirble_t *ap_dirble, const char *ap_query);

/**
 * Search Dirble for a category and add matching stations to the playback
 * queue.
 *
 * After calling this method, the various tiz_dirble_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 * @param ap_category The category.
 *
 * @return 0 on success
 */
int tiz_dirble_play_category (tiz_dirble_t *ap_dirble, const char *ap_category);

/**
* Search Dirble for a country code and add its stations to the playback queue.
*
* After calling this method, the various tiz_dirble_get* methods can be
* used to interact with the playback queue.
*
* @ingroup libtizdirble
*
* @param ap_dirble The dirble handle.
* @param ap_country_code A two-letter country code.
*
* @return 0 on success
*/
int tiz_dirble_play_country (tiz_dirble_t *ap_dirble,
                             const char *ap_country_code);

/**
 * Clear the playback queue.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 */
void tiz_dirble_clear_queue (tiz_dirble_t *ap_dirble);

/**
 * Retrieve the next station url
 *
 * The playback queue pointer moves one position forwards.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 * @param a_remove_current_url If true, delete the current url from the
 * playback queue before moving to the next url.
 *
 * @return The next url in the playback queue or NULL if the playback queue is
 * empty.
 */
const char *tiz_dirble_get_next_url (tiz_dirble_t *ap_dirble,
                                     const bool a_remove_current_url);

/**
 * Retrieve the previous station url.
 *
 * The playback queue pointer moves one position backwards.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 * @param a_remove_current_url If true, delete the current url from the
 * playback queue before moving to the previous url.
 *
 * @return The previous url in the playback queue or NULL if the playback queue
 * is empty.
 */
const char *tiz_dirble_get_prev_url (tiz_dirble_t *ap_dirble,
                                     const bool a_remove_current_url);

/**
 * Retrieve the current station's name.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 */
const char *tiz_dirble_get_current_station_name (tiz_dirble_t *ap_dirble);

/**
* Retrieve the current station's country.
*
* @ingroup libtizdirble
*
* @param ap_dirble The dirble handle.
*/
const char *tiz_dirble_get_current_station_country (tiz_dirble_t *ap_dirble);

/**
 * Retrieve the current station's category.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 */
const char *tiz_dirble_get_current_station_category (tiz_dirble_t *ap_dirble);

/**
 * Retrieve the current station's website.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 */
const char *tiz_dirble_get_current_station_website (tiz_dirble_t *ap_dirble);

/**
 * Retrieve the current station's bitrate.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 */
const char *tiz_dirble_get_current_station_bitrate (tiz_dirble_t *ap_dirble);

/**
 * Retrieve the current station's stream url.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 */
const char *tiz_dirble_get_current_station_stream_url (tiz_dirble_t *ap_dirble);

/**
 * Destroy the dirble handle.
 *
 * @ingroup libtizdirble
 *
 * @param ap_dirble The dirble handle.
 */

void tiz_dirble_destroy (tiz_dirble_t *ap_dirble);

#ifdef __cplusplus
}
#endif

#endif  // TIZDIRBLE_C_H

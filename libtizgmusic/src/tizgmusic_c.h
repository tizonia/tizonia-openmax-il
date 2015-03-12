/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgmusic_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Google Music client library (c wrapper)
 *
 *
 */

#ifndef TIZGMUSIC_C_H
#define TIZGMUSIC_C_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @defgroup googlemusic A Google Music client library
*
* A C library to access the Google Music streaming service (based on Simon
* Weber's 'Unofficial Google Music API' python library).
*
* @ingroup Tizonia
*/

/**
 * The gmusic opaque structure
 * @ingroup googlemusic
 */
typedef struct tiz_gmusic tiz_gmusic_t;
typedef /*@null@ */ tiz_gmusic_t *tiz_gmusic_ptr_t;

/**
 * Initialize the gmusic handle.
 *
 * @ingroup googlemusic
 *
 * @param app_gmusic A pointer to the gmusic handle which will be initialised.
 * @param ap_user A Google email account
 * @param ap_pass The password associated to the Google account.
 * @param ap_device_id A 16-character string containing the device id
 * associated to the Google Music account.
 *
 * @return 0 on success.
 */
int tiz_gmusic_init (/*@null@ */ tiz_gmusic_ptr_t *app_gmusic,
                     const char *ap_user, const char *ap_pass,
                     const char *ap_device_id);
int tiz_gmusic_enqueue_album (tiz_gmusic_t *ap_gmusic, const char *ap_album);
int tiz_gmusic_enqueue_artist (tiz_gmusic_t *ap_gmusic, const char *ap_artist);
void tiz_gmusic_clear_queue (tiz_gmusic_t *ap_gmusic);
const char *tiz_gmusic_get_next_url (tiz_gmusic_t *ap_gmusic);
const char *tiz_gmusic_get_current_song_artist (tiz_gmusic_t *ap_gmusic);
const char *tiz_gmusic_get_current_song_title (tiz_gmusic_t *ap_gmusic);
void tiz_gmusic_destroy (tiz_gmusic_t *ap_gmusic);

#ifdef __cplusplus
}
#endif

#endif  // TIZGMUSIC_C_H

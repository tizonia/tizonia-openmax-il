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
 * @file   tizchromecast_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Chromecast client library (c wrapper)
 *
 *
 */

#ifndef TIZCHROMECAST_C_H
#define TIZCHROMECAST_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
* @defgroup libtizchromecast 'libtizchromecast' : Tizonia's Chromecast client
* library
*
* A C library to access and control a Chromecast streaming device.
*
* @ingroup Tizonia
*/

/**
 * The chromecast opaque structure
 * @ingroup libtizchromecast
 */
typedef struct tiz_chromecast tiz_chromecast_t;
typedef /*@null@ */ tiz_chromecast_t *tiz_chromecast_ptr_t;

/**
 * This callback is invoked when the.
 *
 * @param ap_arg The client data structure.
 *
 */
typedef void (*tiz_chromecast_status_cback_f) (void * ap_user_data);

/**
 * Initialize the chromecast handle.
 *
 * @ingroup libtizchromecast
 *
 * @param app_chromecast A pointer to the chromecast handle which will be
 * initialised.
 * @param ap_name_or_ip A Chromecast device name or ip address.
 * @param apf_media_status A callback for media status information.
 *
 * @return 0 on success.
 */
int tiz_chromecast_init (/*@null@ */ tiz_chromecast_ptr_t *app_chromecast,
                         const char *ap_name_or_ip,
                         tiz_chromecast_status_cback_f apf_status_cb,
                         void * ap_user_data);

/**
 * Poll and read any events received on the chromecast socket.
 *
 * @ingroup libtizchromecast
 *
 * @param app_chromecast The Tizonia Chromecast handle.
 * @param a_poll_time_ms The polling time, in milliseconds.
 *
 * @return 0 on success.
 */
  int tiz_chromecast_poll (tiz_chromecast_t *ap_chromecast, int a_poll_time_ms);

/**
 * Loads a new audio stream URL into the Chromecast media player.
 *
 * After calling this method, the various tiz_gmusic_get* methods can be
 * used to interact with the playback queue.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 * @param ap_url The stream url.
 * @param ap_content_type MIME content type of the media being played.
 * @param ap_title The title of the media being played.
 *
 * @return 0 on success
 */
int tiz_chromecast_load_url (tiz_chromecast_t *ap_chromecast,
                             const char *ap_url, const char *ap_content_type,
                             const char *ap_title);

/**
 * Resume playback of media that has been previously paused. Playback is
 * continued from the current time position.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return 0 on success
 */
int tiz_chromecast_play (tiz_chromecast_t *ap_chromecast);

/**
 * Stop playback of media. Triggers a STATUS event notification to all sender
 * applications.
 *
 * After this command the content will no longer be loaded.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return 0 on success
 */
int tiz_chromecast_stop (tiz_chromecast_t *ap_chromecast);

/**
 * Pause playback of the content that was loaded with the load call.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return 0 on success
 */
int tiz_chromecast_pause (tiz_chromecast_t *ap_chromecast);

/**
 * Increase playback volumen.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return 0 on success
 */
int tiz_chromecast_volume_up (tiz_chromecast_t *ap_chromecast);

/**
 * Decrease playback volume.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return 0 on success
 */
int tiz_chromecast_volume_down (tiz_chromecast_t *ap_chromecast);

/**
 * Mute playback.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return 0 on success
 */
int tiz_chromecast_mute (tiz_chromecast_t *ap_chromecast);

/**
 * Unmute playback.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return 0 on success
 */
int tiz_chromecast_unmute (tiz_chromecast_t *ap_chromecast);

/**
 * Destroy the chromecast handle.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The chromecast handle.
 */
void tiz_chromecast_destroy (tiz_chromecast_t *ap_chromecast);

#ifdef __cplusplus
}
#endif

#endif  // TIZCHROMECAST_C_H

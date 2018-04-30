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
 * @file   tizchromecast_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Chromecast client library (c wrapper)
 *
 *
 */

#ifndef TIZCHROMECAST_C_H
#define TIZCHROMECAST_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tizchromecastctx_c.h"
#include "tizchromecasttypes.h"

/**
 * @defgroup libtizchromecast 'libtizchromecast' : Tizonia's Chromecast client
 * library.
 *
 * A C/C++ library to access and control a Chromecast device.
 *
 * @ingroup Tizonia
 */

/**
 * The Tizonia Chromecast opaque handle.
 * @ingroup libtizchromecast
 */
typedef struct tiz_chromecast tiz_chromecast_t;
typedef /*@null@ */ tiz_chromecast_t *tiz_chromecast_ptr_t;

/**
 * Initialize the tiz_chromecast_t handle.
 *
 * @ingroup libtizchromecast
 *
 * @param app_chromecast A pointer to the Tizonia Chromecast handle to be
 * initialised.
 * @param p_cc_ctx A pointer to an already initialised global context.
 * @param ap_name_or_ip A Chromecast device's name or ip address.
 * @param ap_cbacks A structure with callbacks to inform the client of the
 * various events (cast and media status changes and error events).
 * @param ap_user_data User's data that is returned in each callback.
 *
 * @return ETizCcErrorNoError on success.
 */
tiz_chromecast_error_t tiz_chromecast_init (
    /*@null@ */ tiz_chromecast_ptr_t *app_chromecast,
    const tiz_chromecast_ctx_t *p_cc_ctx_, const char *ap_name_or_ip,
    const tiz_chromecast_callbacks_t *ap_cbacks, void *ap_user_data);

/**
 * Poll to read any events received on the chromecast socket. This function
 * needs to be called periodically (e.g. from the client's event loop).
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 * @param a_poll_time_ms The polling time, in milliseconds.
 *
 * @return ETizCcErrorNoError on success.
 */
tiz_chromecast_error_t tiz_chromecast_poll (tiz_chromecast_t *ap_chromecast,
                                            int a_poll_time_ms);

/**
 * Load a new audio stream URL on the Chromecast device's default media
 * application.
 *
 * After calling this method, the various playback methods (pause, stop,
 * volume, etc) can be used to interact with the media application in the
 * Chromecast device.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 * @param ap_url The stream's URL.
 * @param ap_content_type MIME content type of the stream being loaded.
 * @param ap_title The title of the stream being loaded.
 * @param ap_album_art The stream's album art URL.
 *
 * @return ETizCcErrorNoError on success
 */
tiz_chromecast_error_t tiz_chromecast_load_url (tiz_chromecast_t *ap_chromecast,
                                                const char *ap_url,
                                                const char *ap_content_type,
                                                const char *ap_title,
                                                const char *ap_album_art);

/**
 * Resume media playback.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return ETizCcErrorNoError on success
 */
tiz_chromecast_error_t tiz_chromecast_play (tiz_chromecast_t *ap_chromecast);

/**
 * Stop media playback.
 *
 * After this command the content will no longer remain loaded.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return ETizCcErrorNoError on success
 */
tiz_chromecast_error_t tiz_chromecast_stop (tiz_chromecast_t *ap_chromecast);

/**
 * Pause media playback.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return ETizCcErrorNoError on success
 */
tiz_chromecast_error_t tiz_chromecast_pause (tiz_chromecast_t *ap_chromecast);

/**
 * Set a volume level (0-100).
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 * @param a_volume The volume level (0-100).
 *
 * @return ETizCcErrorNoError on success
 */
tiz_chromecast_error_t tiz_chromecast_volume (tiz_chromecast_t *ap_chromecast,
                                              int a_volume);

/**
 * Increase playback volumen.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return ETizCcErrorNoError on success
 */
tiz_chromecast_error_t tiz_chromecast_volume_up (
    tiz_chromecast_t *ap_chromecast);

/**
 * Decrease playback volume.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return ETizCcErrorNoError on success
 */
tiz_chromecast_error_t tiz_chromecast_volume_down (
    tiz_chromecast_t *ap_chromecast);

/**
 * Mute playback.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return ETizCcErrorNoError on success
 */
tiz_chromecast_error_t tiz_chromecast_mute (tiz_chromecast_t *ap_chromecast);

/**
 * Unmute playback.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 *
 * @return ETizCcErrorNoError on success
 */
tiz_chromecast_error_t tiz_chromecast_unmute (tiz_chromecast_t *ap_chromecast);

/**
 * Destroy the Tizonia Chromecast object.
 *
 * @ingroup libtizchromecast
 *
 * @param ap_chromecast The Tizonia Chromecast handle.
 */
void tiz_chromecast_destroy (tiz_chromecast_t *ap_chromecast);

/**
 * Return an error code string.
 *
 * @ingroup libtizchromecast
 *
 * @param error The Tizonia Chromecast error code.
 */
const char *tiz_chromecast_error_str (const tiz_chromecast_error_t error);

#ifdef __cplusplus
}
#endif

#endif  // TIZCHROMECAST_C_H

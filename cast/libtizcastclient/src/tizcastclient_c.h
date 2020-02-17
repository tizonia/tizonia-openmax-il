/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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
 * @file   tizcastclient_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia's Chromecast daemon - client library (c wrapper)
 *
 *
 */

#ifndef TIZCASTCLIENT_C_H
#define TIZCASTCLIENT_C_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
* @defgroup libtizcastclient 'libtizcastclient' : Tizonia's Chromecast daemon
* client library
*
* This library implements a client to access Tizonia's Chromecast daemon.
*
* @ingroup cast
*/

#include <OMX_Core.h>

#include <tizcasttypes.h>
#include "tizcastclienttypes.h"

tiz_cast_error_t
tiz_cast_client_init (tiz_cast_ptr_t * app_cast,
                      const char * ap_device_name_or_ip,
                      const OMX_UUIDTYPE * ap_uuid,
                      const tiz_cast_client_callbacks_t * ap_cbacks,
                      OMX_PTR ap_data);

tiz_cast_error_t
tiz_cast_client_destroy (tiz_cast_t * ap_cast);

OMX_S32
tiz_cast_client_get_version (const tiz_cast_t * ap_cast);

tiz_cast_error_t
tiz_cast_client_load_url (const tiz_cast_t * ap_cast, const char * url,
                          const char * mime_type, const char * title,
                          const char * album_art);

tiz_cast_error_t
tiz_cast_client_play (const tiz_cast_t * ap_cast);

tiz_cast_error_t
tiz_cast_client_stop (const tiz_cast_t * ap_cast);

tiz_cast_error_t
tiz_cast_client_pause (const tiz_cast_t * ap_cast);

tiz_cast_error_t
tiz_cast_client_volume_set (const tiz_cast_t * ap_cast, int a_volume);

tiz_cast_error_t
tiz_cast_client_volume_up (const tiz_cast_t * ap_cast);

tiz_cast_error_t
tiz_cast_client_volume_down (const tiz_cast_t * ap_cast);

tiz_cast_error_t
tiz_cast_client_mute (const tiz_cast_t * ap_cast);

tiz_cast_error_t
tiz_cast_client_unmute (const tiz_cast_t * ap_cast);

const char *
tiz_cast_client_cast_status_str (const tiz_cast_client_cast_status_t status);

const char *
tiz_cast_client_media_status_str (const tiz_cast_client_media_status_t status);

const char *
tiz_cast_client_error_status_str (const tiz_cast_client_error_status_t status);

#ifdef __cplusplus
}
#endif

#endif  // TIZCASTCLIENT_C_H

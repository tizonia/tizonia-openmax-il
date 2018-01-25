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
 * @file   tizcastclienttypes.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Chromecast daemon - client library (API types)
 *
 *
 */

#ifndef TIZCASTCLIENTTYPES_H
#define TIZCASTCLIENTTYPES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct tiz_cast tiz_cast_t;
typedef /*@null@ */ tiz_cast_t * tiz_cast_ptr_t;

/**
 * Callback function to signal a when the Chromecast device 'cast' status has
 * changed.
 */
typedef void (*tiz_cast_client_cast_status_cb_f) (void * ap_user_data);

/**
 * Callback function to signal a when the Chromecast device 'media' status has
 * changed.
 */
typedef void (*tiz_cast_client_media_status_cb_f) (void * ap_user_data);

typedef struct tiz_cast_client_callbacks
{
  tiz_cast_client_cast_status_cb_f pf_cast_status;
  tiz_cast_client_media_status_cb_f pf_media_status;
} tiz_cast_client_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif  // TIZCASTCLIENTTYPES_H

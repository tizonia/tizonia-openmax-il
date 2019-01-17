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
 * @file   tizchromecasttypes.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Chromecast client library (c wrapper)
 *
 *
 */

#ifndef TIZCHROMECASTTYPES_H
#define TIZCHROMECASTTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tiz_chromecast_error {
  ETizCcErrorNoError = 0,
  ETizCcErrorConnectionError = -1
} tiz_chromecast_error_t;

typedef enum tiz_chromecast_cast_status {
  ETizCcCastStatusUnknown,
  ETizCcCastStatusReadyToCast,
  ETizCcCastStatusNowCasting
} tiz_chromecast_cast_status_t;

/**
 * Callback invoked when the Chromecast device changes its device or media
 * statuses.
 *
 * @param ap_user_data Client-provided data structure.
 * @param a_status The 'cast' status.
 */
typedef void (*tiz_chromecast_cast_status_cb_f) (
    void *ap_user_data, tiz_chromecast_cast_status_t a_status, int a_volume);

typedef enum tiz_chromecast_media_status {
  ETizCcMediaStatusUnknown,
  ETizCcMediaStatusIdle,
  ETizCcMediaStatusBuffering,
  ETizCcMediaStatusPaused,
  ETizCcMediaStatusPlaying
} tiz_chromecast_media_status_t;

/**
 * Callback invoked when the Chromecast device changes its device or media
 * statuses.
 *
 * @param ap_user_data Client-provided data structure.
 * @param a_status The 'media' status.
 */
typedef void (*tiz_chromecast_media_status_cb_f) (
    void *ap_user_data, tiz_chromecast_media_status_t a_status, int a_volume);

typedef struct tiz_chromecast_callbacks
{
  tiz_chromecast_cast_status_cb_f pf_cast_status;
  tiz_chromecast_media_status_cb_f pf_media_status;
} tiz_chromecast_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif  // TIZCHROMECASTTYPES_H

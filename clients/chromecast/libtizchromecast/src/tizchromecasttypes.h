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

enum tiz_chromecast_status
{
  ETizCcStatusCastUnknown,
  ETizCcStatusCastReadyToCast,
  ETizCcStatusMediaIdle,
  ETizCcStatusMediaBuffering,
  ETizCcStatusMediaPlaying,
};
typedef enum tiz_chromecast_status tiz_chromecast_status_t;

/**
 * Callback invoked when the Chromecast device changes its device or media
 * statuses.
 *
 * @param ap_user_data Client-provided data structure.
 * @param a_status The 'cast' or 'media' status.
 */
typedef void (*tiz_chromecast_status_cback_f) (
    void *ap_user_data, tiz_chromecast_status_t a_status);

#ifdef __cplusplus
}
#endif

#endif  // TIZCHROMECASTTYPES_H

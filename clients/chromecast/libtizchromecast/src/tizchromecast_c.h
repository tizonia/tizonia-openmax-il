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
* A C library to access the Chromecast streaming service.
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
 * Initialize the chromecast handle.
 *
 * @ingroup libtizchromecast
 *
 * @param app_chromecast A pointer to the chromecast handle which will be
 * initialised.
 * @param ap_name_or_ip A Chromecast device name or ip address.
 *
 * @return 0 on success.
 */
int tiz_chromecast_init (/*@null@ */ tiz_chromecast_ptr_t *app_chromecast,
                         const char *ap_name_or_ip);

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

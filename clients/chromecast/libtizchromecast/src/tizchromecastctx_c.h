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
 * @file   tizchromecastctx_c.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Chromecast client library (c wrapper)
 *
 *
 */

#ifndef TIZCHROMECASTCTX_C_H
#define TIZCHROMECASTCTX_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "tizchromecasttypes.h"

/**
 * @defgroup libtizchromecastctx 'libtizchromecastctx' : Tizonia's Chromecast
 * client library
 *
 * A C library to access and control a Chromecast streaming device.
 *
 * @ingroup Tizonia
 */

/**
 * The chromecast opaque structure
 * @ingroup libtizchromecastctx
 */
typedef struct tiz_chromecast_ctx tiz_chromecast_ctx_t;
typedef /*@null@ */ tiz_chromecast_ctx_t *tiz_chromecast_ctx_ptr_t;

/**
 * Initialize the chromecast context handle.
 *
 * @ingroup libtizchromecastctx
 *
 * @param app_cc_ctx A pointer to the chromecast handle which will be
 * initialised.
 *
 * @return 0 on success.
 */
int tiz_chromecast_ctx_init (
    /*@null@ */ tiz_chromecast_ctx_ptr_t *app_cc_ctx);

/**
 * Destroy the chromecast context handle.
 *
 * @ingroup libtizchromecastctx
 *
 * @param app_cc_ctx The chromecast context handle.
 */
void tiz_chromecast_ctx_destroy (
    /*@null@ */ tiz_chromecast_ctx_ptr_t *app_cc_ctx);

#ifdef __cplusplus
}
#endif

#endif  // TIZCHROMECASTCTX_C_H

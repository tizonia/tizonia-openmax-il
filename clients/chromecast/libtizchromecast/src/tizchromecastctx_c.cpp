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
 * @file   tizchromecast_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Chromecast client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "tizchromecast.hpp"
#include "tizchromecast_c.h"
#include "tizchromecastctx.hpp"
#include "tizchromecastctxtypes.h"

// Forward declarations
void tiz_chromecast_ctx_destroy (tiz_chromecast_ctx_ptr_t *app_cc_ctx);

static int chromecast_ctx_alloc_data (tiz_chromecast_ctx_t *ap_cc_ctx)
{
  int rc = 0;
  assert (ap_cc_ctx);
  try
    {
      ap_cc_ctx->p_ctx_ = new tizchromecastctx ();
    }
  catch (...)
    {
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_chromecast_ctx_init (tiz_chromecast_ctx_ptr_t *app_cc_ctx)
{
  tiz_chromecast_ctx_t *p_cc_ctx = NULL;
  int rc = 0;

  assert (app_cc_ctx);

  if ((p_cc_ctx
       = (tiz_chromecast_ctx_t *)calloc (1, sizeof (tiz_chromecast_ctx_t))))
    {
      if (chromecast_ctx_alloc_data (p_cc_ctx))
        {
          tiz_chromecast_ctx_destroy (&p_cc_ctx);
        }
    }

  *app_cc_ctx = p_cc_ctx;

  return rc;
}

extern "C" void tiz_chromecast_ctx_destroy (
    tiz_chromecast_ctx_ptr_t *app_cc_ctx)
{
  if (app_cc_ctx)
    {
      delete ((*app_cc_ctx)->p_ctx_);
      free (*app_cc_ctx);
      *app_cc_ctx = NULL;
    }
}

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
#include "tizchromecastctx_c.h"
#include "tizchromecastctxtypes.h"

typedef struct tiz_chromecast_error_str
{
  tiz_chromecast_error_t status;
  const char *str;
} tiz_chromecast_error_str_t;

static const tiz_chromecast_error_str_t tiz_chromecast_error_str_tbl[]
    = { { ETizCcErrorNoError, (const char *)"NoError" },
        { ETizCcErrorConnectionError, (const char *)"ConnectionError" } };

struct tiz_chromecast
{
  tizchromecast *p_proxy_;
};

static void chromecast_free_data (tiz_chromecast_t *ap_chromecast)
{
  if (ap_chromecast)
    {
      delete ap_chromecast->p_proxy_;
      ap_chromecast->p_proxy_ = NULL;
    }
}

static tiz_chromecast_error_t chromecast_alloc_data (
    tiz_chromecast_t *ap_chromecast, const tiz_chromecast_ctx_t * p_cc_ctx,
    const char *ap_name_or_ip, const tiz_chromecast_callbacks_t *ap_cbacks,
    void *ap_user_data)
{
  tiz_chromecast_error_t rc = ETizCcErrorNoError;
  assert (ap_chromecast);
  try
    {
      ap_chromecast->p_proxy_ = new tizchromecast (
          *(p_cc_ctx->p_ctx_), ap_name_or_ip, ap_cbacks, ap_user_data);
    }
  catch (...)
    {
      chromecast_free_data (ap_chromecast);
      rc = ETizCcErrorConnectionError;
    }
  return rc;
}

extern "C" tiz_chromecast_error_t tiz_chromecast_init (
    tiz_chromecast_ptr_t *app_chromecast,
    const tiz_chromecast_ctx_t * p_cc_ctx_, const char *ap_name_or_ip,
    const tiz_chromecast_callbacks_t *ap_cbacks, void *ap_user_data)
{
  tiz_chromecast_t *p_chromecast = NULL;
  tiz_chromecast_error_t rc = ETizCcErrorConnectionError;

  assert (app_chromecast);
  assert (ap_name_or_ip);

  if ((p_chromecast
       = (tiz_chromecast_t *)calloc (1, sizeof (tiz_chromecast_t))))
    {
      if (ETizCcErrorNoError
          == chromecast_alloc_data (p_chromecast, p_cc_ctx_, ap_name_or_ip,
                                    ap_cbacks, ap_user_data))
        {
          tizchromecast *p_cc = p_chromecast->p_proxy_;
          assert (p_cc);
          if (ETizCcErrorNoError == p_cc->init ()
              && ETizCcErrorNoError == p_cc->start ())
            {
              // all good
              rc = ETizCcErrorNoError;
            }
        }

      if (ETizCcErrorNoError != rc)
        {
          chromecast_free_data (p_chromecast);
          free (p_chromecast);
          p_chromecast = NULL;
        }
    }

  *app_chromecast = p_chromecast;

  return rc;
}

extern "C" tiz_chromecast_error_t tiz_chromecast_poll (
    tiz_chromecast_t *ap_chromecast, int a_poll_time_ms)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->poll_socket (a_poll_time_ms);
}

extern "C" tiz_chromecast_error_t tiz_chromecast_load_url (
    tiz_chromecast_t *ap_chromecast, const char *ap_url,
    const char *ap_content_type, const char *ap_title, const char *ap_album_art)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_load (ap_url, ap_content_type, ap_title,
                                              ap_album_art);
}

extern "C" tiz_chromecast_error_t tiz_chromecast_play (
    tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_play ();
}

extern "C" tiz_chromecast_error_t tiz_chromecast_stop (
    tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_stop ();
}

extern "C" tiz_chromecast_error_t tiz_chromecast_pause (
    tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_pause ();
}

extern "C" tiz_chromecast_error_t tiz_chromecast_volume (
    tiz_chromecast_t *ap_chromecast, int a_volume)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_volume (a_volume);
}

extern "C" tiz_chromecast_error_t tiz_chromecast_volume_up (
    tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_volume_up ();
}

extern "C" tiz_chromecast_error_t tiz_chromecast_volume_down (
    tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_volume_down ();
}

extern "C" tiz_chromecast_error_t tiz_chromecast_mute (
    tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_mute ();
}

extern "C" tiz_chromecast_error_t tiz_chromecast_unmute (
    tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_unmute ();
}

extern "C" void tiz_chromecast_destroy (tiz_chromecast_t *ap_chromecast)
{
  if (ap_chromecast)
    {
      tizchromecast *p_cc = ap_chromecast->p_proxy_;
      if (p_cc)
        {
          p_cc->stop ();
          p_cc->deinit ();
        }
      chromecast_free_data (ap_chromecast);
      free (ap_chromecast);
    }
}

extern "C" const char *tiz_chromecast_error_str (
    const tiz_chromecast_error_t error)
{
  const size_t count = sizeof (tiz_chromecast_error_str_tbl)
                       / sizeof (tiz_chromecast_error_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_chromecast_error_str_tbl[i].status == error)
        {
          return tiz_chromecast_error_str_tbl[i].str;
        }
    }

  return (const char *)"Unknown Chromecast 'media' status";
}

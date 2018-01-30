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

static int chromecast_alloc_data (tiz_chromecast_t *ap_chromecast,
                                  const char *ap_name_or_ip,
                                  const tiz_chromecast_callbacks_t *ap_cbacks,
                                  void *ap_user_data)
{
  int rc = 0;
  assert (ap_chromecast);
  try
    {
      ap_chromecast->p_proxy_
          = new tizchromecast (ap_name_or_ip, ap_cbacks, ap_user_data);
    }
  catch (...)
    {
      chromecast_free_data (ap_chromecast);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_chromecast_init (tiz_chromecast_ptr_t *app_chromecast,
                                    const char *ap_name_or_ip,
                                    const tiz_chromecast_callbacks_t *ap_cbacks,
                                    void *ap_user_data)
{
  tiz_chromecast_t *p_chromecast = NULL;
  int rc = 1;

  assert (app_chromecast);
  assert (ap_name_or_ip);

  if ((p_chromecast
       = (tiz_chromecast_t *)calloc (1, sizeof (tiz_chromecast_t))))
    {
      if (!chromecast_alloc_data (p_chromecast, ap_name_or_ip, ap_cbacks,
                                  ap_user_data))
        {
          tizchromecast *p_cc = p_chromecast->p_proxy_;
          assert (p_cc);
          if (!p_cc->init () && !p_cc->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          chromecast_free_data (p_chromecast);
          free (p_chromecast);
          p_chromecast = NULL;
        }
    }

  *app_chromecast = p_chromecast;

  return rc;
}

extern "C" int tiz_chromecast_poll (tiz_chromecast_t *ap_chromecast,
                                    int a_poll_time_ms)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->poll_socket (a_poll_time_ms);
}

extern "C" int tiz_chromecast_load_url (tiz_chromecast_t *ap_chromecast,
                                        const char *ap_url,
                                        const char *ap_content_type,
                                        const char *ap_title, const char *ap_album_art)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_load (ap_url, ap_content_type, ap_title,
                                              ap_album_art);
}

extern "C" int tiz_chromecast_play (tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_play ();
}

extern "C" int tiz_chromecast_stop (tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_stop ();
}

extern "C" int tiz_chromecast_pause (tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_pause ();
}

extern "C" int tiz_chromecast_volume (tiz_chromecast_t *ap_chromecast,
                                      int a_volume)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_volume (a_volume);
}

extern "C" int tiz_chromecast_volume_up (tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_volume_up ();
}

extern "C" int tiz_chromecast_volume_down (tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_volume_down ();
}

extern "C" int tiz_chromecast_mute (tiz_chromecast_t *ap_chromecast)
{
  assert (ap_chromecast);
  assert (ap_chromecast->p_proxy_);
  return ap_chromecast->p_proxy_->media_mute ();
}

extern "C" int tiz_chromecast_unmute (tiz_chromecast_t *ap_chromecast)
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

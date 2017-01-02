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
 * @file   tizdirble_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Dirble client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>

#include "tizdirble.hpp"
#include "tizdirble_c.h"

struct tiz_dirble
{
  tizdirble *p_proxy_;
};

static void dirble_free_data (tiz_dirble_t *ap_dirble)
{
  if (ap_dirble)
    {
      delete ap_dirble->p_proxy_;
      ap_dirble->p_proxy_ = NULL;
    }
}

static int dirble_alloc_data (tiz_dirble_t *ap_dirble, const char *ap_api_key)
{
  int rc = 0;
  assert (ap_dirble);
  try
    {
      ap_dirble->p_proxy_ = new tizdirble (ap_api_key);
    }
  catch (...)
    {
      dirble_free_data (ap_dirble);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_dirble_init (tiz_dirble_ptr_t *app_dirble,
                                const char *ap_api_key)
{
  tiz_dirble_t *p_dirble = NULL;
  int rc = 1;

  assert (app_dirble);
  assert (ap_api_key);

  if ((p_dirble = (tiz_dirble_t *)calloc (1, sizeof(tiz_dirble_t))))
    {
      if (!dirble_alloc_data (p_dirble, ap_api_key))
        {
          tizdirble *p_gm = p_dirble->p_proxy_;
          assert (p_gm);
          if (!p_gm->init () && !p_gm->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          dirble_free_data (p_dirble);
          free (p_dirble);
          p_dirble = NULL;
        }
    }

  *app_dirble = p_dirble;

  return rc;
}

extern "C" void tiz_dirble_set_playback_mode (
    tiz_dirble_t *ap_dirble, const tiz_dirble_playback_mode_t mode)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->set_playback_mode (
      static_cast< tizdirble::playback_mode >(mode));
}

extern "C" int tiz_dirble_play_popular_stations (tiz_dirble_t *ap_dirble)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->play_popular_stations ();
}

extern "C" int tiz_dirble_play_stations (tiz_dirble_t *ap_dirble,
                                         const char *ap_query)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->play_stations (ap_query);
}

extern "C" int tiz_dirble_play_category (tiz_dirble_t *ap_dirble,
                                         const char *ap_category)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->play_category (ap_category);
}

extern "C" int tiz_dirble_play_country (tiz_dirble_t *ap_dirble,
                                        const char *ap_country_code)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->play_country (ap_country_code);
}

extern "C" void tiz_dirble_clear_queue (tiz_dirble_t *ap_dirble)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  ap_dirble->p_proxy_->clear_queue ();
}

extern "C" const char *tiz_dirble_get_next_url (tiz_dirble_t *ap_dirble,
                                                const bool a_remove_current_url)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->get_next_url (a_remove_current_url);
}

extern "C" const char *tiz_dirble_get_prev_url (tiz_dirble_t *ap_dirble,
                                                const bool a_remove_current_url)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->get_prev_url (a_remove_current_url);
}

extern "C" const char *tiz_dirble_get_current_station_name (
    tiz_dirble_t *ap_dirble)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->get_current_station_name ();
}

extern "C" const char *tiz_dirble_get_current_station_country (
    tiz_dirble_t *ap_dirble)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->get_current_station_country ();
}

extern "C" const char *tiz_dirble_get_current_station_category (
    tiz_dirble_t *ap_dirble)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->get_current_station_category ();
}

extern "C" const char *tiz_dirble_get_current_station_website (
    tiz_dirble_t *ap_dirble)
{
  assert (ap_dirble);
  assert (ap_dirble->p_proxy_);
  return ap_dirble->p_proxy_->get_current_station_website ();
}

extern "C" void tiz_dirble_destroy (tiz_dirble_t *ap_dirble)
{
  if (ap_dirble)
    {
      tizdirble *p_gm = ap_dirble->p_proxy_;
      if (p_gm)
        {
          p_gm->stop ();
          p_gm->deinit ();
        }
      dirble_free_data (ap_dirble);
      free (ap_dirble);
    }
}

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
 * @file   tiztunein_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Tunein client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "tiztunein.hpp"
#include "tiztunein_c.h"

struct tiz_tunein
{
  tiztunein *p_proxy_;
};

static void tunein_free_data (tiz_tunein_t *ap_tunein)
{
  if (ap_tunein)
    {
      delete ap_tunein->p_proxy_;
      ap_tunein->p_proxy_ = NULL;
    }
}

static int tunein_alloc_data (tiz_tunein_t *ap_tunein)
{
  int rc = 0;
  assert (ap_tunein);
  try
    {
      ap_tunein->p_proxy_ = new tiztunein ();
    }
  catch (...)
    {
      tunein_free_data (ap_tunein);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_tunein_init (tiz_tunein_ptr_t *app_tunein)
{
  tiz_tunein_t *p_tunein = NULL;
  int rc = 1;

  assert (app_tunein);

  if ((p_tunein = (tiz_tunein_t *)calloc (1, sizeof (tiz_tunein_t))))
    {
      if (!tunein_alloc_data (p_tunein))
        {
          tiztunein *p_gm = p_tunein->p_proxy_;
          assert (p_gm);
          if (!p_gm->init () && !p_gm->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          tunein_free_data (p_tunein);
          free (p_tunein);
          p_tunein = NULL;
        }
    }

  *app_tunein = p_tunein;

  return rc;
}

extern "C" void tiz_tunein_set_playback_mode (
    tiz_tunein_t *ap_tunein, const tiz_tunein_playback_mode_t mode)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->set_playback_mode (
      static_cast< tiztunein::playback_mode > (mode));
}

extern "C" void tiz_tunein_set_search_mode (tiz_tunein_t *ap_tunein,
                                            const tiz_tunein_search_mode_t mode)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->set_search_mode (
      static_cast< tiztunein::search_mode > (mode));
}

extern "C" int tiz_tunein_play_radios (tiz_tunein_t *ap_tunein,
                                       const char *ap_query,
                                       const char *ap_keywords1,
                                       const char *ap_keywords2,
                                       const char *ap_keywords3)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->play_radios (ap_query, ap_keywords1, ap_keywords2,
                                           ap_keywords3);
}

extern "C" int tiz_tunein_play_category (tiz_tunein_t *ap_tunein,
                                         const char *ap_category,
                                         const char *ap_keywords1,
                                         const char *ap_keywords2,
                                         const char *ap_keywords3)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->play_category (ap_category, ap_keywords1,
                                             ap_keywords2, ap_keywords3);
}

extern "C" void tiz_tunein_clear_queue (tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  ap_tunein->p_proxy_->clear_queue ();
}

extern "C" void tiz_tunein_print_queue (tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  ap_tunein->p_proxy_->print_queue ();
}

extern "C" const char *tiz_tunein_get_current_radio_index (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_index ();
}

extern "C" const char *tiz_tunein_get_current_queue_length (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_queue_length ();
}

extern "C" int tiz_tunein_get_current_queue_length_as_int (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_queue_length_as_int ();
}

extern "C" const char *tiz_tunein_get_current_queue_progress (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_queue_progress ();
}

extern "C" const char *tiz_tunein_get_url (tiz_tunein_t *ap_tunein,
                                           const int a_position)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_url (a_position);
}

extern "C" const char *tiz_tunein_get_next_url (tiz_tunein_t *ap_tunein,
                                                const bool a_remove_current_url)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_next_url (a_remove_current_url);
}

extern "C" const char *tiz_tunein_get_prev_url (tiz_tunein_t *ap_tunein,
                                                const bool a_remove_current_url)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_prev_url (a_remove_current_url);
}

extern "C" const char *tiz_tunein_get_current_radio_name (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_name ();
}

extern "C" const char *tiz_tunein_get_current_radio_description (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_description ();
}

extern "C" const char *tiz_tunein_get_current_radio_reliability (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_reliability ();
}

extern "C" const char *tiz_tunein_get_current_radio_type (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_type ();
}

extern "C" const char *tiz_tunein_get_current_radio_website (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_website ();
}

extern "C" const char *tiz_tunein_get_current_radio_bitrate (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_bitrate ();
}

extern "C" const char *tiz_tunein_get_current_radio_format (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_format ();
}

extern "C" const char *tiz_tunein_get_current_radio_stream_url (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_stream_url ();
}

extern "C" const char *tiz_tunein_get_current_radio_thumbnail_url (
    tiz_tunein_t *ap_tunein)
{
  assert (ap_tunein);
  assert (ap_tunein->p_proxy_);
  return ap_tunein->p_proxy_->get_current_radio_thumbnail_url ();
}

extern "C" void tiz_tunein_destroy (tiz_tunein_t *ap_tunein)
{
  if (ap_tunein)
    {
      tiztunein *p_gm = ap_tunein->p_proxy_;
      if (p_gm)
        {
          p_gm->stop ();
          p_gm->deinit ();
        }
      tunein_free_data (ap_tunein);
      free (ap_tunein);
    }
}

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
 * @file   tiziheart_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Iheart client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "tiziheart.hpp"
#include "tiziheart_c.h"

struct tiz_iheart
{
  tiziheart *p_proxy_;
};

static void iheart_free_data (tiz_iheart_t *ap_iheart)
{
  if (ap_iheart)
    {
      delete ap_iheart->p_proxy_;
      ap_iheart->p_proxy_ = NULL;
    }
}

static int iheart_alloc_data (tiz_iheart_t *ap_iheart)
{
  int rc = 0;
  assert (ap_iheart);
  try
    {
      ap_iheart->p_proxy_ = new tiziheart ();
    }
  catch (...)
    {
      iheart_free_data (ap_iheart);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_iheart_init (tiz_iheart_ptr_t *app_iheart)
{
  tiz_iheart_t *p_iheart = NULL;
  int rc = 1;

  assert (app_iheart);

  if ((p_iheart = (tiz_iheart_t *)calloc (1, sizeof (tiz_iheart_t))))
    {
      if (!iheart_alloc_data (p_iheart))
        {
          tiziheart *p_gm = p_iheart->p_proxy_;
          assert (p_gm);
          if (!p_gm->init () && !p_gm->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          iheart_free_data (p_iheart);
          free (p_iheart);
          p_iheart = NULL;
        }
    }

  *app_iheart = p_iheart;

  return rc;
}

extern "C" void tiz_iheart_set_playback_mode (
    tiz_iheart_t *ap_iheart, const tiz_iheart_playback_mode_t mode)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->set_playback_mode (
      static_cast< tiziheart::playback_mode > (mode));
}

extern "C" int tiz_iheart_play_radios (tiz_iheart_t *ap_iheart,
                                       const char *ap_query,
                                       const char *ap_keywords1,
                                       const char *ap_keywords2,
                                       const char *ap_keywords3)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->play_radios (ap_query, ap_keywords1, ap_keywords2,
                                           ap_keywords3);
}

extern "C" void tiz_iheart_clear_queue (tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  ap_iheart->p_proxy_->clear_queue ();
}

extern "C" void tiz_iheart_print_queue (tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  ap_iheart->p_proxy_->print_queue ();
}

extern "C" const char *tiz_iheart_get_current_radio_index (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_radio_index ();
}

extern "C" const char *tiz_iheart_get_current_queue_length (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_queue_length ();
}

extern "C" int tiz_iheart_get_current_queue_length_as_int (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_queue_length_as_int ();
}

extern "C" const char *tiz_iheart_get_current_queue_progress (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_queue_progress ();
}

extern "C" const char *tiz_iheart_get_url (tiz_iheart_t *ap_iheart,
                                           const int a_position)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_url (a_position);
}

extern "C" const char *tiz_iheart_get_next_url (tiz_iheart_t *ap_iheart,
                                                const bool a_remove_current_url)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_next_url (a_remove_current_url);
}

extern "C" const char *tiz_iheart_get_prev_url (tiz_iheart_t *ap_iheart,
                                                const bool a_remove_current_url)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_prev_url (a_remove_current_url);
}

extern "C" const char *tiz_iheart_get_current_radio_name (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_radio_name ();
}

extern "C" const char *tiz_iheart_get_current_radio_description (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_radio_description ();
}

extern "C" const char *tiz_iheart_get_current_radio_city (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_radio_city ();
}

extern "C" const char *tiz_iheart_get_current_radio_state (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_radio_state ();
}

extern "C" const char *tiz_iheart_get_current_radio_audio_encoding (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_radio_audio_encoding ();
}

extern "C" const char *tiz_iheart_get_current_radio_website_url (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_radio_website_url ();
}

extern "C" const char *tiz_iheart_get_current_radio_stream_url (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_radio_stream_url ();
}

extern "C" const char *tiz_iheart_get_current_radio_thumbnail_url (
    tiz_iheart_t *ap_iheart)
{
  assert (ap_iheart);
  assert (ap_iheart->p_proxy_);
  return ap_iheart->p_proxy_->get_current_radio_thumbnail_url ();
}

extern "C" void tiz_iheart_destroy (tiz_iheart_t *ap_iheart)
{
  if (ap_iheart)
    {
      tiziheart *p_gm = ap_iheart->p_proxy_;
      if (p_gm)
        {
          p_gm->stop ();
          p_gm->deinit ();
        }
      iheart_free_data (ap_iheart);
      free (ap_iheart);
    }
}

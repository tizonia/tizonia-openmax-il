/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizyoutube_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple YouTube audio client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>

#include "tizyoutube.hpp"
#include "tizyoutube_c.h"

struct tiz_youtube
{
  tizyoutube *p_proxy_;
};

static void youtube_free_data (tiz_youtube_t *ap_youtube)
{
  if (ap_youtube)
    {
      delete ap_youtube->p_proxy_;
      ap_youtube->p_proxy_ = NULL;
    }
}

static int youtube_alloc_data (tiz_youtube_t *ap_youtube, const char *ap_api_key)
{
  int rc = 0;
  assert (ap_youtube);
  try
    {
      ap_youtube->p_proxy_ = new tizyoutube (ap_api_key);
    }
  catch (...)
    {
      youtube_free_data (ap_youtube);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_youtube_init (tiz_youtube_ptr_t *app_youtube,
                                const char *ap_api_key)
{
  tiz_youtube_t *p_youtube = NULL;
  int rc = 1;

  assert (app_youtube);
  assert (ap_api_key);

  if ((p_youtube = (tiz_youtube_t *)calloc (1, sizeof(tiz_youtube_t))))
    {
      if (!youtube_alloc_data (p_youtube, ap_api_key))
        {
          tizyoutube *p_gm = p_youtube->p_proxy_;
          assert (p_gm);
          if (!p_gm->init () && !p_gm->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          youtube_free_data (p_youtube);
          free (p_youtube);
          p_youtube = NULL;
        }
    }

  *app_youtube = p_youtube;

  return rc;
}

extern "C" void tiz_youtube_set_playback_mode (
    tiz_youtube_t *ap_youtube, const tiz_youtube_playback_mode_t mode)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->set_playback_mode (
      static_cast< tizyoutube::playback_mode >(mode));
}

extern "C" int tiz_youtube_play_audio_stream (tiz_youtube_t *ap_youtube,
                                              const char *ap_url_or_id)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->play_audio_stream (ap_url_or_id);
}

extern "C" int tiz_youtube_play_audio_playlist (tiz_youtube_t *ap_youtube,
                                                const char *ap_url_or_id)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->play_audio_playlist (ap_url_or_id);
}

extern "C" void tiz_youtube_clear_queue (tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  ap_youtube->p_proxy_->clear_queue ();
}

extern "C" const char *tiz_youtube_get_next_url (tiz_youtube_t *ap_youtube,
                                                const bool a_remove_current_url)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_next_url (a_remove_current_url);
}

extern "C" const char *tiz_youtube_get_prev_url (tiz_youtube_t *ap_youtube,
                                                const bool a_remove_current_url)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_prev_url (a_remove_current_url);
}

extern "C" void tiz_youtube_destroy (tiz_youtube_t *ap_youtube)
{
  if (ap_youtube)
    {
      tizyoutube *p_gm = ap_youtube->p_proxy_;
      if (p_gm)
        {
          p_gm->stop ();
          p_gm->deinit ();
        }
      youtube_free_data (ap_youtube);
      free (ap_youtube);
    }
}

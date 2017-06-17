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
 * @file   tizdeezer_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Deezer client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "tizdeezer.hpp"
#include "tizdeezer_c.h"

struct tiz_deezer
{
  tizdeezer *p_proxy_;
};

static void deezer_free_data (tiz_deezer_t *ap_deezer)
{
  if (ap_deezer)
    {
      delete ap_deezer->p_proxy_;
      ap_deezer->p_proxy_ = NULL;
    }
}

static int deezer_alloc_data (tiz_deezer_t *ap_deezer, const char *ap_user)
{
  int rc = 0;
  assert (ap_deezer);
  try
    {
      ap_deezer->p_proxy_ = new tizdeezer (ap_user);
    }
  catch (...)
    {
      deezer_free_data (ap_deezer);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_deezer_init (tiz_deezer_ptr_t *app_deezer,
                                const char *ap_user)
{
  tiz_deezer_t *p_deezer = NULL;
  int rc = 1;

  assert (app_deezer);
  assert (ap_user);

  if ((p_deezer = (tiz_deezer_t *)calloc (1, sizeof (tiz_deezer_t))))
    {
      if (!deezer_alloc_data (p_deezer, ap_user))
        {
          tizdeezer *p_gm = p_deezer->p_proxy_;
          assert (p_gm);
          if (!p_gm->init () && !p_gm->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          deezer_free_data (p_deezer);
          free (p_deezer);
          p_deezer = NULL;
        }
    }

  *app_deezer = p_deezer;

  return rc;
}

extern "C" void tiz_deezer_set_playback_mode (
    tiz_deezer_t *ap_deezer, const tiz_deezer_playback_mode_t mode)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  return ap_deezer->p_proxy_->set_playback_mode (
      static_cast< tizdeezer::playback_mode > (mode));
}

extern "C" int tiz_deezer_play_tracks (tiz_deezer_t *ap_deezer,
                                       const char *ap_tracks)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  return ap_deezer->p_proxy_->play_tracks (ap_tracks);
}

extern "C" int tiz_deezer_play_album (tiz_deezer_t *ap_deezer,
                                      const char *ap_album)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  return ap_deezer->p_proxy_->play_album (ap_album);
}

extern "C" int tiz_deezer_play_artist (tiz_deezer_t *ap_deezer,
                                       const char *ap_artist)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  return ap_deezer->p_proxy_->play_artist (ap_artist);
}

extern "C" void tiz_deezer_clear_queue (tiz_deezer_t *ap_deezer)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  ap_deezer->p_proxy_->clear_queue ();
}

extern "C" int tiz_deezer_next_track (tiz_deezer_t *ap_deezer)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  return ap_deezer->p_proxy_->next_track ();
}

extern "C" int tiz_deezer_prev_track (tiz_deezer_t *ap_deezer)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  return ap_deezer->p_proxy_->prev_track ();
}

extern "C" size_t tiz_deezer_get_mp3_data (tiz_deezer_t *ap_deezer,
                                           unsigned char **app_data)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  return ap_deezer->p_proxy_->get_mp3_data (app_data);
}

extern "C" const char *tiz_deezer_get_current_track_title (
    tiz_deezer_t *ap_deezer)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  return ap_deezer->p_proxy_->get_current_track_title ();
}

extern "C" const char *tiz_deezer_get_current_track_artist (
    tiz_deezer_t *ap_deezer)
{
  assert (ap_deezer);
  assert (ap_deezer->p_proxy_);
  return ap_deezer->p_proxy_->get_current_track_artist ();
}

extern "C" void tiz_deezer_destroy (tiz_deezer_t *ap_deezer)
{
  if (ap_deezer)
    {
      tizdeezer *p_gm = ap_deezer->p_proxy_;
      if (p_gm)
        {
          p_gm->stop ();
          p_gm->deinit ();
        }
      deezer_free_data (ap_deezer);
      free (ap_deezer);
    }
}

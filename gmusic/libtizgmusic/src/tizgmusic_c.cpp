/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgmusic_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Google Play Music client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>

#include "tizgmusic.hpp"
#include "tizgmusic_c.h"

struct tiz_gmusic
{
  tizgmusic *p_proxy_;
};

static void gmusic_free_data (tiz_gmusic_t *ap_gmusic)
{
  if (ap_gmusic)
    {
      delete ap_gmusic->p_proxy_;
      ap_gmusic->p_proxy_ = NULL;
    }
}

static int gmusic_alloc_data (tiz_gmusic_t *ap_gmusic, const char *ap_user,
                              const char *ap_pass, const char *ap_device_id)
{
  int rc = 0;
  assert (NULL != ap_gmusic);
  try
    {
      ap_gmusic->p_proxy_ = new tizgmusic (ap_user, ap_pass, ap_device_id);
    }
  catch (...)
    {
      gmusic_free_data (ap_gmusic);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_gmusic_init (tiz_gmusic_ptr_t *app_gmusic,
                                const char *ap_user, const char *ap_pass,
                                const char *ap_device_id)
{
  tiz_gmusic_t *p_gmusic = NULL;
  int rc = 1;

  assert (NULL != app_gmusic);
  assert (NULL != ap_user);
  assert (NULL != ap_pass);
  assert (NULL != ap_device_id);

  if (NULL != (p_gmusic = (tiz_gmusic_t *)calloc (1, sizeof(tiz_gmusic_t))))
    {
      if (!gmusic_alloc_data (p_gmusic, ap_user, ap_pass, ap_device_id))
        {
          tizgmusic * p_gm = p_gmusic->p_proxy_;
          assert (NULL != p_gm);
          if (!p_gm->init () && !p_gm->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          gmusic_free_data (p_gmusic);
          free (p_gmusic);
          p_gmusic = NULL;
        }
    }

  *app_gmusic = p_gmusic;

  return rc;
}

extern "C" int tiz_gmusic_play_album (tiz_gmusic_t *ap_gmusic,
                                      const char *ap_album,
                                      const bool a_all_access_search)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_album (ap_album, a_all_access_search);
}

extern "C" int tiz_gmusic_play_artist (tiz_gmusic_t *ap_gmusic,
                                       const char *ap_artist,
                                       const bool a_all_access_search)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_artist (ap_artist, a_all_access_search);
}

extern "C" int tiz_gmusic_play_playlist (tiz_gmusic_t *ap_gmusic,
                                            const char *ap_playlist)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_playlist (ap_playlist);
}

extern "C" int tiz_gmusic_play_station (tiz_gmusic_t *ap_gmusic,
                                           const char *ap_station)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_station (ap_station);
}

extern "C" int tiz_gmusic_play_promoted_tracks (tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_promoted_tracks ();
}

extern "C" void tiz_gmusic_clear_queue (tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  ap_gmusic->p_proxy_->clear_queue ();
}

extern "C" const char *tiz_gmusic_get_next_url (tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_next_url ();
}

extern "C" const char *tiz_gmusic_get_prev_url (tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_prev_url ();
}

extern "C" const char *tiz_gmusic_get_current_song_artist (
    tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_song_artist ();
}

extern "C" const char *tiz_gmusic_get_current_song_title (
    tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_song_title ();
}

extern "C" const char *tiz_gmusic_get_current_song_album (
    tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_song_album ();
}

extern "C" const char *tiz_gmusic_get_current_song_duration (
    tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_song_duration ();
}

extern "C" const char *tiz_gmusic_get_current_song_track_number (
    tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_song_track_number ();
}

extern "C" const char *tiz_gmusic_get_current_song_tracks_in_album (
    tiz_gmusic_t *ap_gmusic)
{
  assert (NULL != ap_gmusic);
  assert (NULL != ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_song_tracks_in_album ();
}

extern "C" void tiz_gmusic_destroy (tiz_gmusic_t *ap_gmusic)
{
  if (ap_gmusic)
    {
      tizgmusic *p_gm = ap_gmusic->p_proxy_;
      if (p_gm)
        {
          p_gm->stop ();
          p_gm->deinit ();
        }
      gmusic_free_data (ap_gmusic);
      free (ap_gmusic);
    }
}

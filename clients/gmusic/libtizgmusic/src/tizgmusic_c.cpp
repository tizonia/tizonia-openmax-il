/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and
 * contributors
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

#include <assert.h>
#include <stdlib.h>

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
  assert (ap_gmusic);
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

  assert (app_gmusic);
  assert (ap_user);
  assert (ap_pass);
  assert (ap_device_id);

  if ((p_gmusic = (tiz_gmusic_t *)calloc (1, sizeof (tiz_gmusic_t))))
    {
      if (!gmusic_alloc_data (p_gmusic, ap_user, ap_pass, ap_device_id))
        {
          tizgmusic *p_gm = p_gmusic->p_proxy_;
          assert (p_gm);
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

extern "C" const char *tiz_gmusic_get_current_queue_length (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_queue_length ();
}

extern "C" int tiz_gmusic_get_current_queue_length_as_int (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_queue_length_as_int ();
}

extern "C" const char *tiz_gmusic_get_current_queue_progress (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_queue_progress ();
}

extern "C" void tiz_gmusic_set_playback_mode (
    tiz_gmusic_t *ap_gmusic, const tiz_gmusic_playback_mode_t mode)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->set_playback_mode (
      static_cast< tizgmusic::playback_mode > (mode));
}

extern "C" int tiz_gmusic_play_library (tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_library ();
}

extern "C" int tiz_gmusic_play_tracks (tiz_gmusic_t *ap_gmusic,
                                       const char *ap_tracks,
                                       const bool a_unlimited_search)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_tracks (ap_tracks, a_unlimited_search);
}

extern "C" int tiz_gmusic_play_album (tiz_gmusic_t *ap_gmusic,
                                      const char *ap_album,
                                      const bool a_unlimited_search)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_album (ap_album, a_unlimited_search);
}

extern "C" int tiz_gmusic_play_artist (tiz_gmusic_t *ap_gmusic,
                                       const char *ap_artist,
                                       const bool a_unlimited_search)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_artist (ap_artist, a_unlimited_search);
}

extern "C" int tiz_gmusic_play_playlist (tiz_gmusic_t *ap_gmusic,
                                         const char *ap_playlist,
                                         const bool a_unlimited_search)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_playlist (ap_playlist, a_unlimited_search);
}

extern "C" int tiz_gmusic_play_free_station (tiz_gmusic_t *ap_gmusic,
                                             const char *ap_station)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_free_station (ap_station);
}

extern "C" int tiz_gmusic_play_station (tiz_gmusic_t *ap_gmusic,
                                        const char *ap_station)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_station (ap_station);
}

extern "C" int tiz_gmusic_play_genre (tiz_gmusic_t *ap_gmusic,
                                      const char *ap_genre)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_genre (ap_genre);
}

extern "C" int tiz_gmusic_play_situation (tiz_gmusic_t *ap_gmusic,
                                          const char *ap_situation,
                                          const char *ap_additional_keywords)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_situation (ap_situation,
                                              ap_additional_keywords);
}

extern "C" int tiz_gmusic_play_podcast (tiz_gmusic_t *ap_gmusic,
                                        const char *ap_podcast)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_podcast (ap_podcast);
}

extern "C" int tiz_gmusic_play_promoted_tracks (tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->play_promoted_tracks ();
}

extern "C" void tiz_gmusic_clear_queue (tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  ap_gmusic->p_proxy_->clear_queue ();
}

extern "C" void tiz_gmusic_print_queue (tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  ap_gmusic->p_proxy_->print_queue ();
}

extern "C" const char *tiz_gmusic_get_url (tiz_gmusic_t *ap_gmusic,
                                           const int a_position)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_url (a_position);
}

extern "C" const char *tiz_gmusic_get_next_url (tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_next_url ();
}

extern "C" const char *tiz_gmusic_get_prev_url (tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_prev_url ();
}

extern "C" const char *tiz_gmusic_get_current_track_artist (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_track_artist ();
}

extern "C" const char *tiz_gmusic_get_current_track_title (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_track_title ();
}

extern "C" const char *tiz_gmusic_get_current_track_album (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_track_album ();
}

extern "C" const char *tiz_gmusic_get_current_track_duration (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_track_duration ();
}

extern "C" const char *tiz_gmusic_get_current_track_track_number (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_track_track_number ();
}

extern "C" const char *tiz_gmusic_get_current_track_tracks_in_album (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_track_tracks_in_album ();
}

extern "C" const char *tiz_gmusic_get_current_track_year (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_track_year ();
}

extern "C" const char *tiz_gmusic_get_current_track_genre (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_track_genre ();
}

extern "C" const char *tiz_gmusic_get_current_track_album_art (
    tiz_gmusic_t *ap_gmusic)
{
  assert (ap_gmusic);
  assert (ap_gmusic->p_proxy_);
  return ap_gmusic->p_proxy_->get_current_track_album_art ();
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

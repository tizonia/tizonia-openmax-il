/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   tizplex_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Plex audio client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "tizplex.hpp"
#include "tizplex_c.h"

struct tiz_plex
{
  tizplex *p_proxy_;
};

static void plex_free_data (tiz_plex_t *ap_plex)
{
  if (ap_plex)
    {
      delete ap_plex->p_proxy_;
      ap_plex->p_proxy_ = NULL;
    }
}

static int plex_alloc_data (tiz_plex_t *ap_plex)
{
  int rc = 0;
  assert (ap_plex);
  try
    {
      ap_plex->p_proxy_ = new tizplex ();
    }
  catch (...)
    {
      plex_free_data (ap_plex);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_plex_init (tiz_plex_ptr_t *app_plex)
{
  tiz_plex_t *p_plex = NULL;
  int rc = 1;

  assert (app_plex);

  if ((p_plex = (tiz_plex_t *)calloc (1, sizeof (tiz_plex_t))))
    {
      if (!plex_alloc_data (p_plex))
        {
          tizplex *p_px = p_plex->p_proxy_;
          assert (p_px);
          if (!p_px->init () && !p_px->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          plex_free_data (p_plex);
          free (p_plex);
          p_plex = NULL;
        }
    }

  *app_plex = p_plex;

  return rc;
}

extern "C" void tiz_plex_clear_queue (tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  ap_plex->p_proxy_->clear_queue ();
}

extern "C" const char *tiz_plex_get_current_audio_track_index (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_index ();
}

extern "C" const char *tiz_plex_get_current_queue_length (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_queue_length ();
}

extern "C" const char *tiz_plex_get_current_queue_progress (tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_queue_progress ();
}

extern "C" void tiz_plex_set_playback_mode (
    tiz_plex_t *ap_plex, const tiz_plex_playback_mode_t mode)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->set_playback_mode (
      static_cast< tizplex::playback_mode > (mode));
}

extern "C" int tiz_plex_play_audio_tracks (tiz_plex_t *ap_plex,
                                              const char *ap_tracks)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->play_audio_tracks (ap_tracks);
}

extern "C" int tiz_plex_play_audio_artist (tiz_plex_t *ap_plex,
                                              const char *ap_artist)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->play_audio_artist (ap_artist);
}

extern "C" int tiz_plex_play_audio_album (tiz_plex_t *ap_plex,
                                              const char *ap_album)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->play_audio_album (ap_album);
}

extern "C" int tiz_plex_play_audio_playlist (tiz_plex_t *ap_plex,
                                                const char *ap_url_or_id)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->play_audio_playlist (ap_url_or_id);
}

extern "C" const char *tiz_plex_get_next_url (
    tiz_plex_t *ap_plex, const bool a_remove_current_url)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_next_url (a_remove_current_url);
}

extern "C" const char *tiz_plex_get_prev_url (
    tiz_plex_t *ap_plex, const bool a_remove_current_url)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_prev_url (a_remove_current_url);
}

extern "C" const char *tiz_plex_get_current_audio_track_title (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_title ();
}

extern "C" const char *tiz_plex_get_current_audio_track_artist (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_artist ();
}

extern "C" const char *tiz_plex_get_current_audio_track_album (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_album ();
}

extern "C" const char *tiz_plex_get_current_audio_track_year (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_year ();
}

extern "C" const char *tiz_plex_get_current_audio_track_file_size (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_file_size ();
}

extern "C" const char *tiz_plex_get_current_audio_track_duration (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_duration ();
}

extern "C" const char *tiz_plex_get_current_audio_track_bitrate (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_bitrate ();
}

extern "C" const char *tiz_plex_get_current_audio_track_codec (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_codec ();
}

extern "C" const char *tiz_plex_get_current_audio_track_album_art (
    tiz_plex_t *ap_plex)
{
  assert (ap_plex);
  assert (ap_plex->p_proxy_);
  return ap_plex->p_proxy_->get_current_audio_track_album_art ();
}

extern "C" void tiz_plex_destroy (tiz_plex_t *ap_plex)
{
  if (ap_plex)
    {
      tizplex *p_px = ap_plex->p_proxy_;
      if (p_px)
        {
          p_px->stop ();
          p_px->deinit ();
        }
      plex_free_data (ap_plex);
      free (ap_plex);
    }
}

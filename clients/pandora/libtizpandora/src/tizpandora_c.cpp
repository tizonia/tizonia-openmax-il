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
 * @file   tizpandora_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Pandora audio client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "tizpandora.hpp"
#include "tizpandora_c.h"

struct tiz_pandora
{
  tizpandora *p_proxy_;
};

static void pandora_free_data (tiz_pandora_t *ap_pandora)
{
  if (ap_pandora)
    {
      delete ap_pandora->p_proxy_;
      ap_pandora->p_proxy_ = NULL;
    }
}

static int pandora_alloc_data (tiz_pandora_t *ap_pandora, const char *ap_base_url,
                            const char *ap_auth_token)
{
  int rc = 0;
  assert (ap_pandora);
  try
    {
      ap_pandora->p_proxy_ = new tizpandora (ap_base_url, ap_auth_token);
    }
  catch (...)
    {
      pandora_free_data (ap_pandora);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_pandora_init (tiz_pandora_ptr_t *app_pandora, const char *ap_base_url,
                              const char *ap_auth_token)
{
  tiz_pandora_t *p_pandora = NULL;
  int rc = 1;

  assert (app_pandora);

  if ((p_pandora = (tiz_pandora_t *)calloc (1, sizeof (tiz_pandora_t))))
    {
      if (!pandora_alloc_data (p_pandora, ap_base_url, ap_auth_token))
        {
          tizpandora *p_px = p_pandora->p_proxy_;
          assert (p_px);
          if (!p_px->init () && !p_px->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          pandora_free_data (p_pandora);
          free (p_pandora);
          p_pandora = NULL;
        }
    }

  *app_pandora = p_pandora;

  return rc;
}

extern "C" void tiz_pandora_clear_queue (tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  ap_pandora->p_proxy_->clear_queue ();
}

extern "C" const char *tiz_pandora_get_current_audio_track_index (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_index ();
}

extern "C" const char *tiz_pandora_get_current_queue_length (tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_queue_length ();
}

extern "C" const char *tiz_pandora_get_current_queue_progress (tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_queue_progress ();
}

extern "C" void tiz_pandora_set_playback_mode (tiz_pandora_t *ap_pandora,
                                            const tiz_pandora_playback_mode_t mode)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->set_playback_mode (
      static_cast< tizpandora::playback_mode > (mode));
}

extern "C" int tiz_pandora_play_audio_tracks (tiz_pandora_t *ap_pandora,
                                           const char *ap_tracks)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->play_audio_tracks (ap_tracks);
}

extern "C" int tiz_pandora_play_audio_artist (tiz_pandora_t *ap_pandora,
                                           const char *ap_artist)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->play_audio_artist (ap_artist);
}

extern "C" int tiz_pandora_play_audio_album (tiz_pandora_t *ap_pandora,
                                          const char *ap_album)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->play_audio_album (ap_album);
}

extern "C" int tiz_pandora_play_audio_playlist (tiz_pandora_t *ap_pandora,
                                             const char *ap_url_or_id)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->play_audio_playlist (ap_url_or_id);
}

extern "C" const char *tiz_pandora_get_next_url (tiz_pandora_t *ap_pandora,
                                              const bool a_remove_current_url)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_next_url (a_remove_current_url);
}

extern "C" const char *tiz_pandora_get_prev_url (tiz_pandora_t *ap_pandora,
                                              const bool a_remove_current_url)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_prev_url (a_remove_current_url);
}

extern "C" const char *tiz_pandora_get_current_audio_track_title (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_title ();
}

extern "C" const char *tiz_pandora_get_current_audio_track_artist (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_artist ();
}

extern "C" const char *tiz_pandora_get_current_audio_track_album (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_album ();
}

extern "C" const char *tiz_pandora_get_current_audio_track_year (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_year ();
}

extern "C" const char *tiz_pandora_get_current_audio_track_file_size (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_file_size ();
}

extern "C" int tiz_pandora_get_current_audio_track_file_size_as_int (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_file_size_as_int ();
}

extern "C" const char *tiz_pandora_get_current_audio_track_duration (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_duration ();
}

extern "C" const char *tiz_pandora_get_current_audio_track_bitrate (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_bitrate ();
}

extern "C" const char *tiz_pandora_get_current_audio_track_codec (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_codec ();
}

extern "C" const char *tiz_pandora_get_current_audio_track_album_art (
    tiz_pandora_t *ap_pandora)
{
  assert (ap_pandora);
  assert (ap_pandora->p_proxy_);
  return ap_pandora->p_proxy_->get_current_audio_track_album_art ();
}

extern "C" void tiz_pandora_destroy (tiz_pandora_t *ap_pandora)
{
  if (ap_pandora)
    {
      tizpandora *p_px = ap_pandora->p_proxy_;
      if (p_px)
        {
          p_px->stop ();
          p_px->deinit ();
        }
      pandora_free_data (ap_pandora);
      free (ap_pandora);
    }
}

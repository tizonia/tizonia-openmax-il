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
 * @file   tizyoutube_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple YouTube audio client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

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

static int youtube_alloc_data (tiz_youtube_t *ap_youtube)
{
  int rc = 0;
  assert (ap_youtube);
  try
    {
      ap_youtube->p_proxy_ = new tizyoutube ();
    }
  catch (...)
    {
      youtube_free_data (ap_youtube);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_youtube_init (tiz_youtube_ptr_t *app_youtube)
{
  tiz_youtube_t *p_youtube = NULL;
  int rc = 1;

  assert (app_youtube);

  if ((p_youtube = (tiz_youtube_t *)calloc (1, sizeof (tiz_youtube_t))))
    {
      if (!youtube_alloc_data (p_youtube))
        {
          tizyoutube *p_yt = p_youtube->p_proxy_;
          assert (p_yt);
          if (!p_yt->init () && !p_yt->start ())
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

extern "C" void tiz_youtube_clear_queue (tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  ap_youtube->p_proxy_->clear_queue ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_index (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_index ();
}

extern "C" const char *tiz_youtube_get_current_queue_length (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_queue_length ();
}

extern "C" const char *tiz_youtube_get_current_queue_progress (tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_queue_progress ();
}

extern "C" void tiz_youtube_set_playback_mode (
    tiz_youtube_t *ap_youtube, const tiz_youtube_playback_mode_t mode)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->set_playback_mode (
      static_cast< tizyoutube::playback_mode > (mode));
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

extern "C" int tiz_youtube_play_audio_mix (tiz_youtube_t *ap_youtube,
                                           const char *ap_url_or_id)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->play_audio_mix (ap_url_or_id);
}

extern "C" int tiz_youtube_play_audio_search (tiz_youtube_t *ap_youtube,
                                              const char *ap_search)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->play_audio_search (ap_search);
}

extern "C" int tiz_youtube_play_audio_mix_search (tiz_youtube_t *ap_youtube,
                                                  const char *ap_search)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->play_audio_mix_search (ap_search);
}

extern "C" int tiz_youtube_play_audio_channel_uploads (tiz_youtube_t *ap_youtube,
                                                  const char *ap_channel)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->play_audio_channel_uploads (ap_channel);
}

extern "C" const char *tiz_youtube_get_next_url (
    tiz_youtube_t *ap_youtube, const bool a_remove_current_url)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_next_url (a_remove_current_url);
}

extern "C" const char *tiz_youtube_get_prev_url (
    tiz_youtube_t *ap_youtube, const bool a_remove_current_url)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_prev_url (a_remove_current_url);
}

extern "C" const char *tiz_youtube_get_current_audio_stream_title (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_title ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_author (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_author ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_file_size (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_file_size ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_duration (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_duration ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_bitrate (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_bitrate ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_view_count (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_view_count ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_description (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_description ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_file_extension (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_file_extension ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_video_id (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_video_id ();
}

extern "C" const char *tiz_youtube_get_current_audio_stream_published (
    tiz_youtube_t *ap_youtube)
{
  assert (ap_youtube);
  assert (ap_youtube->p_proxy_);
  return ap_youtube->p_proxy_->get_current_audio_stream_published ();
}

extern "C" void tiz_youtube_destroy (tiz_youtube_t *ap_youtube)
{
  if (ap_youtube)
    {
      tizyoutube *p_yt = ap_youtube->p_proxy_;
      if (p_yt)
        {
          p_yt->stop ();
          p_yt->deinit ();
        }
      youtube_free_data (ap_youtube);
      free (ap_youtube);
    }
}

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
 * @file   tizsoundcloud_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple SoundCloud client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>

#include "tizsoundcloud.hpp"
#include "tizsoundcloud_c.h"

struct tiz_scloud
{
  tizsoundcloud *p_proxy_;
};

static void soundcloud_free_data (tiz_scloud_t *ap_scloud)
{
  if (ap_scloud)
    {
      delete ap_scloud->p_proxy_;
      ap_scloud->p_proxy_ = NULL;
    }
}

static int soundcloud_alloc_data (tiz_scloud_t *ap_scloud,
                                  const char *ap_oauth_token)
{
  int rc = 0;
  assert (ap_scloud);
  try
    {
      ap_scloud->p_proxy_ = new tizsoundcloud (ap_oauth_token);
    }
  catch (...)
    {
      soundcloud_free_data (ap_scloud);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_scloud_init (tiz_scloud_ptr_t *app_scloud,
                                const char *ap_oauth_token)
{
  tiz_scloud_t *p_scloud = NULL;
  int rc = 1;

  assert (app_scloud);
  assert (ap_oauth_token);

  if ((p_scloud = (tiz_scloud_t *)calloc (1, sizeof(tiz_scloud_t))))
    {
      if (!soundcloud_alloc_data (p_scloud, ap_oauth_token))
        {
          tizsoundcloud *p_gm = p_scloud->p_proxy_;
          assert (p_gm);
          if (!p_gm->init () && !p_gm->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          soundcloud_free_data (p_scloud);
          free (p_scloud);
          p_scloud = NULL;
        }
    }

  *app_scloud = p_scloud;

  return rc;
}

extern "C" void tiz_scloud_set_playback_mode (
    tiz_scloud_t *ap_scloud, const tiz_scloud_playback_mode_t mode)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->set_playback_mode (
      static_cast< tizsoundcloud::playback_mode >(mode));
}

extern "C" int tiz_scloud_play_user_stream (tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->play_user_stream ();
}

extern "C" int tiz_scloud_play_user_likes (tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->play_user_likes ();
}

extern "C" int tiz_scloud_play_user_playlist (tiz_scloud_t *ap_scloud,
                                              const char *ap_playlist)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->play_user_playlist (ap_playlist);
}

extern "C" int tiz_scloud_play_creator (tiz_scloud_t *ap_scloud,
                                        const char *ap_creator)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->play_creator (ap_creator);
}

extern "C" int tiz_scloud_play_tracks (tiz_scloud_t *ap_scloud,
                                       const char *ap_tracks)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->play_tracks (ap_tracks);
}

extern "C" int tiz_scloud_play_playlists (tiz_scloud_t *ap_scloud,
                                          const char *ap_playlists)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->play_playlists (ap_playlists);
}

extern "C" int tiz_scloud_play_genres (tiz_scloud_t *ap_scloud,
                                        const char *ap_genres)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->play_genres (ap_genres);
}

extern "C" int tiz_scloud_play_tags (tiz_scloud_t *ap_scloud,
                                        const char *ap_tags)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->play_tags (ap_tags);
}

extern "C" void tiz_scloud_clear_queue (tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  ap_scloud->p_proxy_->clear_queue ();
}

extern "C" const char *tiz_scloud_get_next_url (tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_next_url ();
}

extern "C" const char *tiz_scloud_get_prev_url (tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_prev_url ();
}

extern "C" const char *tiz_scloud_get_current_track_user (
    tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_current_track_user ();
}

extern "C" const char *tiz_scloud_get_current_track_title (
    tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_current_track_title ();
}

extern "C" const char *tiz_scloud_get_current_track_duration (
    tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_current_track_duration ();
}

extern "C" const char *tiz_scloud_get_current_track_year (
    tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_current_track_year ();
}

extern "C" const char *tiz_scloud_get_current_track_permalink (
    tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_current_track_permalink ();
}

extern "C" const char *tiz_scloud_get_current_track_license (
    tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_current_track_license ();
}

extern "C" const char *tiz_scloud_get_current_track_likes (
    tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_current_track_likes ();
}

extern "C" const char *tiz_scloud_get_current_track_user_avatar (
    tiz_scloud_t *ap_scloud)
{
  assert (ap_scloud);
  assert (ap_scloud->p_proxy_);
  return ap_scloud->p_proxy_->get_current_track_user_avatar ();
}

extern "C" void tiz_scloud_destroy (tiz_scloud_t *ap_scloud)
{
  if (ap_scloud)
    {
      tizsoundcloud *p_gm = ap_scloud->p_proxy_;
      if (p_gm)
        {
          p_gm->stop ();
          p_gm->deinit ();
        }
      soundcloud_free_data (ap_scloud);
      free (ap_scloud);
    }
}

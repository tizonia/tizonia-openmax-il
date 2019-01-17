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
 * @file   tizspotify_c.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Spotify Web client library (c wrapper)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "tizspotify.hpp"
#include "tizspotify_c.h"

struct tiz_spotify
{
  tizspotify *p_proxy_;
};

static void spotify_free_data (tiz_spotify_t *ap_spotify)
{
  if (ap_spotify)
    {
      delete ap_spotify->p_proxy_;
      ap_spotify->p_proxy_ = NULL;
    }
}

static int spotify_alloc_data (tiz_spotify_t *ap_spotify)
{
  int rc = 0;
  assert (ap_spotify);
  try
    {
      ap_spotify->p_proxy_ = new tizspotify ();
    }
  catch (...)
    {
      spotify_free_data (ap_spotify);
      rc = 1;
    }
  return rc;
}

extern "C" int tiz_spotify_init (tiz_spotify_ptr_t *app_spotify)
{
  tiz_spotify_t *p_spotify = NULL;
  int rc = 1;

  assert (app_spotify);

  if ((p_spotify = (tiz_spotify_t *)calloc (1, sizeof (tiz_spotify_t))))
    {
      if (!spotify_alloc_data (p_spotify))
        {
          tizspotify *p_px = p_spotify->p_proxy_;
          assert (p_px);
          if (!p_px->init () && !p_px->start ())
            {
              // all good
              rc = 0;
            }
        }

      if (0 != rc)
        {
          spotify_free_data (p_spotify);
          free (p_spotify);
          p_spotify = NULL;
        }
    }

  *app_spotify = p_spotify;

  return rc;
}

extern "C" void tiz_spotify_clear_queue (tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  ap_spotify->p_proxy_->clear_queue ();
}

extern "C" const char *tiz_spotify_get_current_track_index (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_index ();
}

extern "C" const char *tiz_spotify_get_current_queue_length (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_queue_length ();
}

extern "C" int tiz_spotify_get_current_queue_length_as_int (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_queue_length_as_int ();
}

extern "C" const char *tiz_spotify_get_current_queue_progress (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_queue_progress ();
}

extern "C" void tiz_spotify_set_playback_mode (
    tiz_spotify_t *ap_spotify, const tiz_spotify_playback_mode_t mode)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->set_playback_mode (
      static_cast< tizspotify::playback_mode > (mode));
}

extern "C" int tiz_spotify_play_tracks (tiz_spotify_t *ap_spotify,
                                        const char *ap_tracks)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_tracks (ap_tracks);
}

extern "C" int tiz_spotify_play_artist (tiz_spotify_t *ap_spotify,
                                        const char *ap_artist)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_artist (ap_artist);
}

extern "C" int tiz_spotify_play_album (tiz_spotify_t *ap_spotify,
                                       const char *ap_album)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_album (ap_album);
}

extern "C" int tiz_spotify_play_playlist (tiz_spotify_t *ap_spotify,
                                          const char *ap_playlist,
                                          const char *ap_owner)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_playlist (ap_playlist, ap_owner);
}

extern "C" int tiz_spotify_play_track_by_id (tiz_spotify_t *ap_spotify,
                                             const char *ap_track_id)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_track_id (ap_track_id);
}

extern "C" int tiz_spotify_play_artist_by_id (tiz_spotify_t *ap_spotify,
                                              const char *ap_artist_id)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_artist_id (ap_artist_id);
}

extern "C" int tiz_spotify_play_album_by_id (tiz_spotify_t *ap_spotify,
                                             const char *ap_album_id)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_album_id (ap_album_id);
}

extern "C" int tiz_spotify_play_playlist_by_id (tiz_spotify_t *ap_spotify,
                                                const char *ap_playlist_id,
                                                const char *ap_owner)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_playlist_id (ap_playlist_id, ap_owner);
}

extern "C" int tiz_spotify_play_related_artists (tiz_spotify_t *ap_spotify,
                                                 const char *ap_artist)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_related_artists (ap_artist);
}

extern "C" int tiz_spotify_play_featured_playlist (tiz_spotify_t *ap_spotify,
                                                   const char *ap_playlist_name)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_featured_playlist (ap_playlist_name);
}

extern "C" int tiz_spotify_play_new_releases (tiz_spotify_t *ap_spotify,
                                              const char *ap_album_name)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_new_releases (ap_album_name);
}

extern "C" int tiz_spotify_play_recommendations_by_track_id (
    tiz_spotify_t *ap_spotify, const char *ap_track_id)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_recommendations_by_track_id (ap_track_id);
}

extern "C" int tiz_spotify_play_recommendations_by_artist_id (
    tiz_spotify_t *ap_spotify, const char *ap_artist_id)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_recommendations_by_artist_id (ap_artist_id);
}

extern "C" int tiz_spotify_play_recommendations_by_genre (
    tiz_spotify_t *ap_spotify, const char *ap_genre)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->play_recommendations_by_genre (ap_genre);
}

extern "C" const char *tiz_spotify_get_next_uri (
    tiz_spotify_t *ap_spotify, const bool a_remove_current_uri)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_next_uri (a_remove_current_uri);
}

extern "C" const char *tiz_spotify_get_prev_uri (
    tiz_spotify_t *ap_spotify, const bool a_remove_current_uri)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_prev_uri (a_remove_current_uri);
}

extern "C" const char *tiz_spotify_get_current_track_title (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_title ();
}

extern "C" const char *tiz_spotify_get_current_track_artist (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_artist ();
}

extern "C" const char *tiz_spotify_get_current_track_album (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_album ();
}

extern "C" const char *tiz_spotify_get_current_track_release_date (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_release_date ();
}

extern "C" const char *tiz_spotify_get_current_track_duration (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_duration ();
}

extern "C" const char *tiz_spotify_get_current_track_album_art (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_album_art ();
}

extern "C" const char *tiz_spotify_get_current_track_uri (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_uri ();
}

extern "C" const char *tiz_spotify_get_current_track_artist_uri (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_artist_uri ();
}

extern "C" const char *tiz_spotify_get_current_track_album_uri (
    tiz_spotify_t *ap_spotify)
{
  assert (ap_spotify);
  assert (ap_spotify->p_proxy_);
  return ap_spotify->p_proxy_->get_current_track_album_uri ();
}

extern "C" void tiz_spotify_destroy (tiz_spotify_t *ap_spotify)
{
  if (ap_spotify)
    {
      tizspotify *p_px = ap_spotify->p_proxy_;
      if (p_px)
        {
          p_px->stop ();
          p_px->deinit ();
        }
      spotify_free_data (ap_spotify);
      free (ap_spotify);
    }
}

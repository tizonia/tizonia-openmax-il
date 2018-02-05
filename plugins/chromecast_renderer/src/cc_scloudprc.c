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
 * along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.
 */

/**
 * @file   cc_scloudprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  SoundCloud chromecast renderer - processor class
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "chromecastrnd.h"
#include "cc_scloudprc.h"
#include "cc_scloudprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc.scloud"
#endif

/* forward declarations */

#define on_scloud_error_ret_omx_oom(expr)                                    \
  do                                                                         \
    {                                                                        \
      int scloud_error = 0;                                                  \
      if (0 != (scloud_error = (expr)))                                      \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtizsoundcloud");                                    \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

static OMX_ERRORTYPE
retrieve_sc_session (cc_scloud_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioSoundCloudSession, &(ap_prc->sc_session_));
}

static OMX_ERRORTYPE
retrieve_sc_playlist (cc_scloud_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioSoundCloudPlaylist, &(ap_prc->sc_playlist_));
}

static OMX_ERRORTYPE
enqueue_gm_playlist_items (cc_scloud_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_sc_);

  {
    const char * p_playlist = (const char *) ap_prc->sc_playlist_.cPlaylistName;
    const OMX_BOOL shuffle = ap_prc->sc_playlist_.bShuffle;

    tiz_scloud_set_playback_mode (
      ap_prc->p_sc_, (shuffle == OMX_TRUE ? ETIZScloudPlaybackModeShuffle
                                              : ETIZScloudPlaybackModeNormal));

    switch (ap_prc->sc_playlist_.ePlaylistType)
      {
        case OMX_AUDIO_SoundCloudPlaylistTypeUnknown:
          {
            /* TODO */
            assert (0);
          }
          break;
        case OMX_AUDIO_SoundCloudPlaylistTypeUserStream:
          {
            rc = tiz_scloud_play_user_stream (ap_prc->p_sc_);
          }
          break;
        case OMX_AUDIO_SoundCloudPlaylistTypeUserLikes:
          {
            rc = tiz_scloud_play_user_likes (ap_prc->p_sc_);
          }
          break;
        case OMX_AUDIO_SoundCloudPlaylistTypeUserPlaylist:
          {
            rc = tiz_scloud_play_user_playlist (ap_prc->p_sc_, p_playlist);
          }
          break;
        case OMX_AUDIO_SoundCloudPlaylistTypeCreator:
          {
            rc = tiz_scloud_play_creator (ap_prc->p_sc_, p_playlist);
          }
          break;
        case OMX_AUDIO_SoundCloudPlaylistTypeTracks:
          {
            rc = tiz_scloud_play_tracks (ap_prc->p_sc_, p_playlist);
          }
          break;
        case OMX_AUDIO_SoundCloudPlaylistTypePlaylists:
          {
            rc = tiz_scloud_play_playlists (ap_prc->p_sc_, p_playlist);
          }
          break;
        case OMX_AUDIO_SoundCloudPlaylistTypeGenres:
          {
            rc = tiz_scloud_play_genres (ap_prc->p_sc_, p_playlist);
          }
          break;
        case OMX_AUDIO_SoundCloudPlaylistTypeTags:
          {
            rc = tiz_scloud_play_tags (ap_prc->p_sc_, p_playlist);
          }
          break;
        default:
          {
            assert (0);
          }
          break;
      };
  }
  return (rc == 0 ? OMX_ErrorNone : OMX_ErrorInsufficientResources);
}

/*
 * cc_scloudprc
 */

static void *
cc_scloud_prc_ctor (void * ap_obj, va_list * app)
{
  cc_scloud_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "cc_scloudprc"), ap_obj, app);
  TIZ_INIT_OMX_STRUCT (p_prc->sc_session_);
  TIZ_INIT_OMX_STRUCT (p_prc->sc_playlist_);
  p_prc->p_sc_ = NULL;
  return p_prc;
}

static void *
cc_scloud_prc_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "cc_scloudprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
cc_scloud_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_scloud_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (tiz_srv_super_allocate_resources (
    typeOf (p_prc, "cc_scloudprc"), p_prc, a_pid));

  tiz_check_omx (retrieve_sc_session (p_prc));
  tiz_check_omx (retrieve_sc_playlist (p_prc));

  on_scloud_error_ret_omx_oom (tiz_scloud_init (
    &(p_prc->p_sc_), (const char *) p_prc->sc_session_.cUserOauthToken));

  tiz_check_omx (enqueue_gm_playlist_items (p_prc));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_scloud_prc_deallocate_resources (void * ap_prc)
{
  cc_scloud_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_scloud_destroy (p_prc->p_sc_);
  p_prc->p_sc_ = NULL;
  return tiz_srv_super_deallocate_resources (typeOf (ap_prc, "cc_scloudprc"),
                                             ap_prc);
}

static const char *
cc_scloud_prc_get_next_url (const void * p_obj)
{
  cc_scloud_prc_t * p_prc = (cc_scloud_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_sc_);
  return tiz_scloud_get_next_url (p_prc->p_sc_);
}

static const char *
cc_scloud_prc_get_prev_url (const void * p_obj)
{
  cc_scloud_prc_t * p_prc = (cc_scloud_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_sc_);
  return tiz_scloud_get_prev_url (p_prc->p_sc_);
}

static const char *
cc_scloud_prc_get_current_song_album_art_url (const void * p_obj)
{
  cc_scloud_prc_t * p_prc = (cc_scloud_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_sc_);
  /*   return tiz_scloud_get_current_song_album_art (p_prc->p_sc_); */
  return NULL;
}

static OMX_ERRORTYPE
cc_scloud_prc_store_song_metadata (cc_scloud_prc_t * ap_obj)
{
  cc_scloud_prc_t * p_prc = (cc_scloud_prc_t *) ap_obj;
  cc_prc_t * p_cc_prc = (cc_prc_t *) p_prc;
  assert (p_prc);

  /* User and track title */
  tiz_check_omx (cc_prc_store_song_metadata_item (
    p_cc_prc, tiz_scloud_get_current_track_user (p_prc->p_sc_),
    tiz_scloud_get_current_track_title (p_prc->p_sc_)));

  /* Store the year if not 0 */
  {
    const char * p_year = tiz_scloud_get_current_track_year (p_prc->p_sc_);
    if (p_year && strncmp (p_year, "0", 4) != 0)
      {
        tiz_check_omx (
          cc_prc_store_song_metadata_item (p_cc_prc, "Year", p_year));
      }
  }

  /* Duration */
  tiz_check_omx (cc_prc_store_song_metadata_item (
    p_cc_prc, "Duration",
    tiz_scloud_get_current_track_duration (p_prc->p_sc_)));

  /* Likes */
  tiz_check_omx (cc_prc_store_song_metadata_item (
    p_cc_prc, "Likes count",
    tiz_scloud_get_current_track_likes (p_prc->p_sc_)));

  /* Permalink */
  tiz_check_omx (cc_prc_store_song_metadata_item (
    p_cc_prc, "Permalink",
    tiz_scloud_get_current_track_permalink (p_prc->p_sc_)));

  /* License */
  tiz_check_omx (cc_prc_store_song_metadata_item (
    p_cc_prc, "License", tiz_scloud_get_current_track_license (p_prc->p_sc_)));

  return OMX_ErrorNone;
}

/*
 * cc_scloud_prc_class
 */

static void *
cc_scloud_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "cc_scloudprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
cc_scloud_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_scloudprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (cc_prc), "cc_scloudprc_class", classOf (cc_prc),
     sizeof (cc_scloud_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_scloud_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_scloudprc_class;
}

void *
cc_scloud_prc_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_scloudprc_class = tiz_get_type (ap_hdl, "cc_scloudprc_class");
  TIZ_LOG_CLASS (cc_scloudprc_class);
  void * cc_scloudprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_scloudprc_class, "cc_scloudprc", cc_prc, sizeof (cc_scloud_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_scloud_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_scloud_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, cc_scloud_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, cc_scloud_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_next_url, cc_scloud_prc_get_next_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_prev_url, cc_scloud_prc_get_prev_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_current_song_album_art_url,
     cc_scloud_prc_get_current_song_album_art_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_song_metadata, cc_scloud_prc_store_song_metadata,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_scloudprc;
}

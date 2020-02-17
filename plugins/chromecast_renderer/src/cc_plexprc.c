/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   cc_plexprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Plex chromecast renderer - processor class
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
#include "cc_plexprc.h"
#include "cc_plexprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc.plex"
#endif

/* forward declarations */

#define on_plex_error_ret_omx_oom(expr)                                      \
  do                                                                         \
    {                                                                        \
      int plex_error = 0;                                                    \
      if (0 != (plex_error = (expr)))                                        \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtizplex");                                          \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

static OMX_ERRORTYPE
retrieve_plex_session (cc_plex_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioPlexSession, &(ap_prc->sc_session_));
}

static OMX_ERRORTYPE
retrieve_plex_playlist (cc_plex_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioPlexPlaylist, &(ap_prc->sc_playlist_));
}

static OMX_ERRORTYPE
enqueue_plex_playlist_items (cc_plex_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_plex_);

  {
    const char * p_playlist = (const char *) ap_prc->sc_playlist_.cPlaylistName;
    const OMX_BOOL shuffle = ap_prc->sc_playlist_.bShuffle;

    tiz_plex_set_playback_mode (
      ap_prc->p_plex_, (shuffle == OMX_TRUE ? ETIZPlexPlaybackModeShuffle
                                            : ETIZPlexPlaybackModeNormal));

    switch (ap_prc->sc_playlist_.ePlaylistType)
      {
        case OMX_AUDIO_PlexPlaylistTypeUnknown:
          {
            /* TODO */
            assert (0);
          }
          break;
        case OMX_AUDIO_PlexPlaylistTypeAudioTracks:
          {
            rc = tiz_plex_play_audio_tracks (ap_prc->p_plex_, p_playlist);
          }
          break;
        case OMX_AUDIO_PlexPlaylistTypeAudioArtist:
          {
            rc = tiz_plex_play_audio_artist (ap_prc->p_plex_, p_playlist);
          }
          break;
        case OMX_AUDIO_PlexPlaylistTypeAudioAlbum:
          {
            rc = tiz_plex_play_audio_album (ap_prc->p_plex_, p_playlist);
          }
          break;
        case OMX_AUDIO_PlexPlaylistTypeAudioPlaylist:
          {
            rc = tiz_plex_play_audio_playlist (ap_prc->p_plex_, p_playlist);
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
 * cc_plexprc
 */

static void *
cc_plex_prc_ctor (void * ap_obj, va_list * app)
{
  cc_plex_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "cc_plexprc"), ap_obj, app);
  TIZ_INIT_OMX_STRUCT (p_prc->sc_session_);
  TIZ_INIT_OMX_STRUCT (p_prc->sc_playlist_);
  p_prc->p_plex_ = NULL;
  p_prc->remove_current_url_ = false;
  return p_prc;
}

static void *
cc_plex_prc_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "cc_plexprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
cc_plex_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_plex_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (tiz_srv_super_allocate_resources (typeOf (p_prc, "cc_plexprc"),
                                                   p_prc, a_pid));

  tiz_check_omx (retrieve_plex_session (p_prc));
  tiz_check_omx (retrieve_plex_playlist (p_prc));

  on_plex_error_ret_omx_oom (tiz_plex_init (
    &(p_prc->p_plex_), (const char *) p_prc->sc_session_.cBaseUrl,
    (const char *) p_prc->sc_session_.cAuthToken,
    (const char *) p_prc->sc_session_.cMusicSectionName));

  tiz_check_omx (enqueue_plex_playlist_items (p_prc));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_plex_prc_deallocate_resources (void * ap_prc)
{
  cc_plex_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_plex_destroy (p_prc->p_plex_);
  p_prc->p_plex_ = NULL;
  return tiz_srv_super_deallocate_resources (typeOf (ap_prc, "cc_plexprc"),
                                             ap_prc);
}

static const char *
cc_plex_prc_get_next_url (const void * p_obj)
{
  cc_plex_prc_t * p_prc = (cc_plex_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_plex_);
  return tiz_plex_get_next_url (p_prc->p_plex_, p_prc->remove_current_url_);
}

static const char *
cc_plex_prc_get_prev_url (const void * p_obj)
{
  cc_plex_prc_t * p_prc = (cc_plex_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_plex_);
  return tiz_plex_get_prev_url (p_prc->p_plex_, p_prc->remove_current_url_);
}

static const char *
cc_plex_prc_get_current_stream_album_art_url (const void * p_obj)
{
  cc_plex_prc_t * p_prc = (cc_plex_prc_t *) p_obj;
  const char * p_art_url = NULL;
  assert (p_prc);
  assert (p_prc->p_plex_);
#define PLEX_LOGO "https://tizonia.org/img/plex-logo.png"
  p_art_url = tiz_plex_get_current_audio_track_album_art (p_prc->p_plex_);
  return (p_art_url ? p_art_url : PLEX_LOGO);
}

static OMX_ERRORTYPE
cc_plex_prc_store_stream_metadata (cc_plex_prc_t * ap_obj)
{
  cc_plex_prc_t * p_prc = (cc_plex_prc_t *) ap_obj;
  cc_prc_t * p_cc_prc = (cc_prc_t *) p_prc;
  assert (p_prc);

  /* User and track title */
  {
    const char * p_user
      = tiz_plex_get_current_audio_track_artist (p_prc->p_plex_);
    const char * p_title
      = tiz_plex_get_current_audio_track_title (p_prc->p_plex_);
    tiz_check_omx (cc_prc_store_display_title (p_cc_prc, p_user, p_title));
    tiz_check_omx (
      cc_prc_store_stream_metadata_item (p_cc_prc, p_user, p_title));
  }

  /* Playback queue progress */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_prc, "Stream #", tiz_plex_get_current_queue_progress (p_prc->p_plex_)));

  /* Album */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_prc, "Album", tiz_plex_get_current_audio_track_album (p_prc->p_plex_)));

  /* Store the year if not 0 */
  {
    const char * p_year
      = tiz_plex_get_current_audio_track_year (p_prc->p_plex_);
    if (p_year && strncmp (p_year, "0", 4) != 0)
      {
        tiz_check_omx (
          cc_prc_store_stream_metadata_item (p_cc_prc, "Published", p_year));
      }
  }

  /* File size */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_prc, "Size",
    tiz_plex_get_current_audio_track_file_size (p_prc->p_plex_)));

  /* Duration */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_prc, "Duration",
    tiz_plex_get_current_audio_track_duration (p_prc->p_plex_)));

  /* File Format */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_prc, "Codec", tiz_plex_get_current_audio_track_codec (p_prc->p_plex_)));

  return OMX_ErrorNone;
}

/*
 * cc_plex_prc_class
 */

static void *
cc_plex_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "cc_plexprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
cc_plex_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_plexprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (cc_prc), "cc_plexprc_class", classOf (cc_prc),
     sizeof (cc_plex_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_plex_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_plexprc_class;
}

void *
cc_plex_prc_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_plexprc_class = tiz_get_type (ap_hdl, "cc_plexprc_class");
  TIZ_LOG_CLASS (cc_plexprc_class);
  void * cc_plexprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_plexprc_class, "cc_plexprc", cc_prc, sizeof (cc_plex_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_plex_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_plex_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, cc_plex_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, cc_plex_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_next_url, cc_plex_prc_get_next_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_prev_url, cc_plex_prc_get_prev_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_current_stream_album_art_url,
     cc_plex_prc_get_current_stream_album_art_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_stream_metadata, cc_plex_prc_store_stream_metadata,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_plexprc;
}

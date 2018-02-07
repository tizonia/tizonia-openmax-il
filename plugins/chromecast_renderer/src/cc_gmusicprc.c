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
 * @file   cc_gmusicprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Google Music Chromecast renderer - processor class
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
#include <strings.h>

#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "chromecastrnd.h"
#include "cc_gmusicprc.h"
#include "cc_gmusicprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc.gmusic"
#endif

/* forward declarations */

#define on_gmusic_error_ret_omx_oom(expr)                                    \
  do                                                                         \
    {                                                                        \
      int gmusic_error = 0;                                                  \
      if (0 != (gmusic_error = (expr)))                                      \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtizgmusic");                                        \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

static OMX_ERRORTYPE
retrieve_gm_session (cc_gmusic_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioGmusicSession, &(ap_prc->gm_session_));
}

static OMX_ERRORTYPE
retrieve_gm_playlist (cc_gmusic_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioGmusicPlaylist, &(ap_prc->gm_playlist_));
}

static OMX_ERRORTYPE
enqueue_gm_playlist_items (cc_gmusic_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_gm_);

  {
    const char * p_playlist = (const char *) ap_prc->gm_playlist_.cPlaylistName;
    const OMX_BOOL is_unlimited_search = ap_prc->gm_playlist_.bUnlimitedSearch;
    const OMX_BOOL shuffle = ap_prc->gm_playlist_.bShuffle;

    tiz_gmusic_set_playback_mode (
      ap_prc->p_gm_, (shuffle == OMX_TRUE ? ETIZGmusicPlaybackModeShuffle
                                          : ETIZGmusicPlaybackModeNormal));

    switch (ap_prc->gm_playlist_.ePlaylistType)
      {
        case OMX_AUDIO_GmusicPlaylistTypeUnknown:
          {
            /* TODO */
            assert (0);
          }
          break;
        case OMX_AUDIO_GmusicPlaylistTypeUser:
          {
            rc = tiz_gmusic_play_playlist (ap_prc->p_gm_, p_playlist,
                                           is_unlimited_search);
          }
          break;
        case OMX_AUDIO_GmusicPlaylistTypeArtist:
          {
            rc = tiz_gmusic_play_artist (ap_prc->p_gm_, p_playlist,
                                         is_unlimited_search);
          }
          break;
        case OMX_AUDIO_GmusicPlaylistTypeAlbum:
          {
            rc = tiz_gmusic_play_album (ap_prc->p_gm_, p_playlist,
                                        is_unlimited_search);
          }
          break;
        case OMX_AUDIO_GmusicPlaylistTypeStation:
          {
            rc = tiz_gmusic_play_station (ap_prc->p_gm_, p_playlist);
          }
          break;
        case OMX_AUDIO_GmusicPlaylistTypeGenre:
          {
            rc = tiz_gmusic_play_genre (ap_prc->p_gm_, p_playlist);
          }
          break;
        case OMX_AUDIO_GmusicPlaylistTypeSituation:
          {
            rc = tiz_gmusic_play_situation (ap_prc->p_gm_, p_playlist);
          }
          break;
        case OMX_AUDIO_GmusicPlaylistTypePromotedTracks:
          {
            rc = tiz_gmusic_play_promoted_tracks (ap_prc->p_gm_);
          }
          break;
        case OMX_AUDIO_GmusicPlaylistTypeTracks:
          {
            rc = tiz_gmusic_play_tracks (ap_prc->p_gm_, p_playlist,
                                         is_unlimited_search);
          }
          break;
        case OMX_AUDIO_GmusicPlaylistTypePodcast:
          {
            rc = tiz_gmusic_play_podcast (ap_prc->p_gm_, p_playlist);
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
 * cc_gmusicprc
 */

static void *
cc_gmusic_prc_ctor (void * ap_obj, va_list * app)
{
  cc_gmusic_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "cc_gmusicprc"), ap_obj, app);
  TIZ_INIT_OMX_STRUCT (p_prc->gm_session_);
  TIZ_INIT_OMX_STRUCT (p_prc->gm_playlist_);
  p_prc->p_gm_ = NULL;
  return p_prc;
}

static void *
cc_gmusic_prc_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "cc_gmusicprc"), ap_obj);
}

static OMX_ERRORTYPE
cc_gmusic_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_gmusic_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (tiz_srv_super_allocate_resources (
    typeOf (p_prc, "cc_gmusicprc"), p_prc, a_pid));

  tiz_check_omx (retrieve_gm_session (p_prc));
  tiz_check_omx (retrieve_gm_playlist (p_prc));

  on_gmusic_error_ret_omx_oom (tiz_gmusic_init (
    &(p_prc->p_gm_), (const char *) p_prc->gm_session_.cUserName,
    (const char *) p_prc->gm_session_.cUserPassword,
    (const char *) p_prc->gm_session_.cDeviceId));

  tiz_check_omx (enqueue_gm_playlist_items (p_prc));

  TIZ_TRACE (handleOf (p_prc), "cUserName  : [%s]",
             p_prc->gm_session_.cUserName);
  TIZ_TRACE (handleOf (p_prc), "cUserPassword  : [%s]",
             p_prc->gm_session_.cUserPassword);
  TIZ_TRACE (handleOf (p_prc), "cDeviceId  : [%s]",
             p_prc->gm_session_.cDeviceId);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_gmusic_prc_deallocate_resources (void * ap_prc)
{
  cc_gmusic_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_gmusic_destroy (p_prc->p_gm_);
  p_prc->p_gm_ = NULL;
  return tiz_srv_super_deallocate_resources (typeOf (ap_prc, "cc_gmusicprc"),
                                             ap_prc);
}

static const char *
cc_gmusic_prc_get_next_url (const void * p_obj)
{
  cc_gmusic_prc_t * p_prc = (cc_gmusic_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_gm_);
  return tiz_gmusic_get_next_url (p_prc->p_gm_);
}

static const char *
cc_gmusic_prc_get_prev_url (const void * p_obj)
{
  cc_gmusic_prc_t * p_prc = (cc_gmusic_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_gm_);
  return tiz_gmusic_get_prev_url (p_prc->p_gm_);
}

static const char *
cc_gmusic_prc_get_current_stream_album_art_url (const void * p_obj)
{
  cc_gmusic_prc_t * p_prc = (cc_gmusic_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_gm_);
  return tiz_gmusic_get_current_song_album_art (p_prc->p_gm_);
}

static OMX_ERRORTYPE
cc_gmusic_prc_store_stream_metadata (const void * p_obj)
{
  cc_gmusic_prc_t * p_prc = (cc_gmusic_prc_t *) p_obj;
  cc_prc_t * p_cc_prc = (cc_prc_t *) p_obj;
  assert (p_prc);

  TIZ_DEBUG (handleOf (p_prc), "store_stream_metadata");

  /* Artist and song title */
  {
    const char * p_artist = tiz_gmusic_get_current_song_artist (p_prc->p_gm_);
    const char * p_title = tiz_gmusic_get_current_song_title (p_prc->p_gm_);
    tiz_check_omx (cc_prc_store_display_title (p_cc_prc, p_artist, p_title));
    tiz_check_omx (
      cc_prc_store_stream_metadata_item (p_cc_prc, p_artist, p_title));
  }

  /* Album */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Album", tiz_gmusic_get_current_song_album (p_prc->p_gm_)));

  /* Store the year if not 0 */
  {
    const char * p_year = tiz_gmusic_get_current_song_year (p_prc->p_gm_);
    if (p_year && strncmp (p_year, "0", 4) != 0)
      {
        tiz_check_omx (
          cc_prc_store_stream_metadata_item (p_cc_prc, "Year", p_year));
      }
  }

  /* Store genre if not empty */
  {
    const char * p_genre = tiz_gmusic_get_current_song_genre (p_prc->p_gm_);
    if (p_genre && strnlen (p_genre, OMX_MAX_STRINGNAME_SIZE) > 0)
      {
        tiz_check_omx (
          cc_prc_store_stream_metadata_item (p_cc_prc, "Genre", p_genre));
      }
  }

  /* Store album art if not empty */
  {
    const char * p_album_art
      = tiz_gmusic_get_current_song_album_art (p_prc->p_gm_);
    if (p_album_art && strnlen (p_album_art, OMX_MAX_STRINGNAME_SIZE) > 0)
      {
        tiz_check_omx (
          cc_prc_store_stream_metadata_item (p_cc_prc, "Album Art", p_album_art));
      }
  }

  /* Song duration */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Duration", tiz_gmusic_get_current_song_duration (p_prc->p_gm_)));

  /* Track number */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Track",
    tiz_gmusic_get_current_song_track_number (p_prc->p_gm_)));

  /* Store total tracks if not 0 */
  {
    const char * p_total_tracks
      = tiz_gmusic_get_current_song_tracks_in_album (p_prc->p_gm_);
    if (p_total_tracks && strncmp (p_total_tracks, "0", 2) != 0)
      {
        tiz_check_omx (cc_prc_store_stream_metadata_item (
          p_cc_prc, "Total tracks", p_total_tracks));
      }
  }

  return OMX_ErrorNone;
}

/*
 * cc_gmusic_prc_class
 */

static void *
cc_gmusic_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "cc_gmusicprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
cc_gmusic_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_gmusicprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (cc_prc), "cc_gmusicprc_class", classOf (cc_prc),
     sizeof (cc_gmusic_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_gmusic_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_gmusicprc_class;
}

void *
cc_gmusic_prc_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_gmusicprc_class = tiz_get_type (ap_hdl, "cc_gmusicprc_class");
  TIZ_LOG_CLASS (cc_gmusicprc_class);
  void * cc_gmusicprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_gmusicprc_class, "cc_gmusicprc", cc_prc, sizeof (cc_gmusic_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_gmusic_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_gmusic_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, cc_gmusic_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, cc_gmusic_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_next_url, cc_gmusic_prc_get_next_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_prev_url, cc_gmusic_prc_get_prev_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_current_stream_album_art_url,
     cc_gmusic_prc_get_current_stream_album_art_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_stream_metadata, cc_gmusic_prc_store_stream_metadata,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_gmusicprc;
}

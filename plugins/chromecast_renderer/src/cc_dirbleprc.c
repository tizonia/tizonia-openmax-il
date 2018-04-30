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
 * along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.
 */

/**
 * @file   cc_dirbleprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Dirble Chromecast renderer - processor class
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
#include "cc_dirbleprc.h"
#include "cc_dirbleprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc.dirble"
#endif

/* forward declarations */

#define on_dirble_error_ret_omx_oom(expr)                                    \
  do                                                                         \
    {                                                                        \
      int dirble_error = 0;                                                  \
      if (0 != (dirble_error = (expr)))                                      \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtizdirble [error %d]",                              \
                     dirble_error);                                          \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

static OMX_ERRORTYPE
retrieve_db_session (cc_dirble_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioDirbleSession, &(ap_prc->db_session_));
}

static OMX_ERRORTYPE
retrieve_db_playlist (cc_dirble_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioDirblePlaylist, &(ap_prc->db_playlist_));
}

static OMX_ERRORTYPE
enqueue_db_playlist_items (cc_dirble_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_db_);

  {
    const char * p_playlist = (const char *) ap_prc->db_playlist_.cPlaylistName;
    const OMX_BOOL shuffle = ap_prc->db_playlist_.bShuffle;

    tiz_dirble_set_playback_mode (
      ap_prc->p_db_, (shuffle == OMX_TRUE ? ETIZDirblePlaybackModeShuffle
                                          : ETIZDirblePlaybackModeNormal));

    switch (ap_prc->db_playlist_.ePlaylistType)
      {
        case OMX_AUDIO_DirblePlaylistTypeUnknown:
          {
            /* TODO */
            assert (0);
          }
          break;
        case OMX_AUDIO_DirblePlaylistTypePopularStations:
          {
            rc = tiz_dirble_play_popular_stations (ap_prc->p_db_);
          }
          break;
        case OMX_AUDIO_DirblePlaylistTypeStations:
          {
            rc = tiz_dirble_play_stations (ap_prc->p_db_, p_playlist);
          }
          break;
        case OMX_AUDIO_DirblePlaylistTypeCategory:
          {
            rc = tiz_dirble_play_category (ap_prc->p_db_, p_playlist);
          }
          break;
        case OMX_AUDIO_DirblePlaylistTypeCountry:
          {
            rc = tiz_dirble_play_country (ap_prc->p_db_, p_playlist);
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
 * cc_dirbleprc
 */

static void *
cc_dirble_prc_ctor (void * ap_obj, va_list * app)
{
  cc_dirble_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "cc_dirbleprc"), ap_obj, app);
  TIZ_INIT_OMX_STRUCT (p_prc->db_session_);
  TIZ_INIT_OMX_STRUCT (p_prc->db_playlist_);
  p_prc->p_db_ = NULL;
  p_prc->remove_current_url_ = false;
  return p_prc;
}

static void *
cc_dirble_prc_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "cc_dirbleprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
cc_dirble_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_dirble_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (tiz_srv_super_allocate_resources (
    typeOf (p_prc, "cc_dirbleprc"), p_prc, a_pid));

  tiz_check_omx (retrieve_db_session (p_prc));
  tiz_check_omx (retrieve_db_playlist (p_prc));

  on_dirble_error_ret_omx_oom (tiz_dirble_init (
    &(p_prc->p_db_), (const char *) p_prc->db_session_.cApiKey));

  tiz_check_omx (enqueue_db_playlist_items (p_prc));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_dirble_prc_deallocate_resources (void * ap_prc)
{
  cc_dirble_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_dirble_destroy (p_prc->p_db_);
  p_prc->p_db_ = NULL;
  return tiz_srv_super_deallocate_resources (typeOf (ap_prc, "cc_dirbleprc"),
                                             ap_prc);
}

static const char *
cc_dirble_prc_get_next_url (const void * p_obj)
{
  cc_dirble_prc_t * p_prc = (cc_dirble_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_db_);
  return tiz_dirble_get_next_url (p_prc->p_db_, p_prc->remove_current_url_);
}

static const char *
cc_dirble_prc_get_prev_url (const void * p_obj)
{
  cc_dirble_prc_t * p_prc = (cc_dirble_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_db_);
  return tiz_dirble_get_prev_url (p_prc->p_db_, p_prc->remove_current_url_);
}

static const char *
cc_dirble_prc_get_current_stream_album_art_url (const void * p_obj)
{
  cc_dirble_prc_t * p_prc = (cc_dirble_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_db_);
#define DB_LOGO "http://tizonia.org/img/dirble-logo.png"
  return DB_LOGO;
}

static OMX_ERRORTYPE
cc_dirble_prc_store_stream_metadata (cc_dirble_prc_t * ap_obj)
{
  cc_dirble_prc_t * p_prc = (cc_dirble_prc_t *) ap_obj;
  cc_prc_t * p_cc_prc = (cc_prc_t *) p_prc;
  assert (p_prc);

  /* Station Name */
  {
    const char * p_station = tiz_dirble_get_current_station_name (p_prc->p_db_);
    tiz_check_omx (
      cc_prc_store_stream_metadata_item (p_prc, "Station", p_station));
    tiz_check_omx (cc_prc_store_display_title (p_cc_prc, "Station", p_station));
  }

  /* Stream URL */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Stream URL",
    tiz_dirble_get_current_station_stream_url (p_prc->p_db_)));

  /* Bitrate */
  {
    const char * p_bitrate
      = tiz_dirble_get_current_station_bitrate (p_prc->p_db_);
    if (p_bitrate && strncmp (p_bitrate, "0", 3) != 0)
      {
        tiz_check_omx (
          cc_prc_store_stream_metadata_item (p_cc_prc, "Bitrate", p_bitrate));
      }
  }

  /* Country */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Country",
    tiz_dirble_get_current_station_country (p_prc->p_db_)));

  /* Category */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Categories",
    tiz_dirble_get_current_station_category (p_prc->p_db_)));

  /* Website */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Website",
    tiz_dirble_get_current_station_website (p_prc->p_db_)));

  return OMX_ErrorNone;
}

/*
 * cc_dirble_prc_class
 */

static void *
cc_dirble_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "cc_dirbleprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
cc_dirble_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_dirbleprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (cc_prc), "cc_dirbleprc_class", classOf (cc_prc),
     sizeof (cc_dirble_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_dirble_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_dirbleprc_class;
}

void *
cc_dirble_prc_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_dirbleprc_class = tiz_get_type (ap_hdl, "cc_dirbleprc_class");
  TIZ_LOG_CLASS (cc_dirbleprc_class);
  void * cc_dirbleprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_dirbleprc_class, "cc_dirbleprc", cc_prc, sizeof (cc_dirble_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_dirble_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_dirble_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, cc_dirble_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, cc_dirble_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_next_url, cc_dirble_prc_get_next_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_prev_url, cc_dirble_prc_get_prev_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_current_stream_album_art_url,
     cc_dirble_prc_get_current_stream_album_art_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_stream_metadata, cc_dirble_prc_store_stream_metadata,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_dirbleprc;
}

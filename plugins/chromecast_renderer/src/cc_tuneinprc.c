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
 * along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.
 */

/**
 * @file   cc_tuneinprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tunein Chromecast renderer - processor class
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
#include "cc_tuneinprc.h"
#include "cc_tuneinprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc.tunein"
#endif

/* forward declarations */

#define on_tunein_error_ret_omx_oom(expr)                                    \
  do                                                                         \
    {                                                                        \
      int tunein_error = 0;                                                  \
      if (0 != (tunein_error = (expr)))                                      \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtiztunein [error %d]",                              \
                     tunein_error);                                          \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

static OMX_ERRORTYPE
retrieve_tunein_session (cc_tunein_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioTuneinSession, &(ap_prc->tunein_session_));
}

static OMX_ERRORTYPE
retrieve_tunein_playlist (cc_tunein_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioTuneinPlaylist, &(ap_prc->tunein_playlist_));
}

static OMX_ERRORTYPE
enqueue_tunein_playlist_items (cc_tunein_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_tunein_);

  {
    const char * p_playlist = (const char *) ap_prc->tunein_playlist_.cPlaylistName;
    const char * p_keywords1 = (const char *) ap_prc->tunein_playlist_.cAdditionalKeywords1;
    const char * p_keywords2 = (const char *) ap_prc->tunein_playlist_.cAdditionalKeywords2;
    const char * p_keywords3 = (const char *) ap_prc->tunein_playlist_.cAdditionalKeywords3;
    const OMX_BOOL shuffle = ap_prc->tunein_playlist_.bShuffle;

    tiz_tunein_set_playback_mode (
      ap_prc->p_tunein_, (shuffle == OMX_TRUE ? ETIZTuneinPlaybackModeShuffle
                                          : ETIZTuneinPlaybackModeNormal));

    switch (ap_prc->tunein_playlist_.eSearchType)
      {
        case OMX_AUDIO_TuneinSearchTypeAll:
          {
            tiz_tunein_set_search_mode (ap_prc->p_tunein_,
                                        ETIZTuneinSearchModeAll);
          }
          break;
        case OMX_AUDIO_TuneinSearchTypeStations:
          {
            tiz_tunein_set_search_mode (ap_prc->p_tunein_,
                                        ETIZTuneinSearchModeStations);
          }
          break;
        case OMX_AUDIO_TuneinSearchTypeShows:
          {
            tiz_tunein_set_search_mode (ap_prc->p_tunein_,
                                        ETIZTuneinSearchModeShows);
          }
          break;
        default:
          {
            assert (0);
          }
          break;
      };

    switch (ap_prc->tunein_playlist_.ePlaylistType)
      {
        case OMX_AUDIO_TuneinPlaylistTypeUnknown:
          {
            /* TODO */
            assert (0);
          }
          break;
        case OMX_AUDIO_TuneinPlaylistTypeRadios:
          {
            rc = tiz_tunein_play_radios (ap_prc->p_tunein_, p_playlist,
                                         p_keywords1, p_keywords2, p_keywords3);
          }
          break;
        case OMX_AUDIO_TuneinPlaylistTypeCategory:
          {
            rc = tiz_tunein_play_category (ap_prc->p_tunein_, p_playlist,
                                           p_keywords1, p_keywords2,
                                           p_keywords3);
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
 * cc_tuneinprc
 */

static void *
cc_tunein_prc_ctor (void * ap_obj, va_list * app)
{
  cc_tunein_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "cc_tuneinprc"), ap_obj, app);
  TIZ_INIT_OMX_STRUCT (p_prc->tunein_session_);
  TIZ_INIT_OMX_STRUCT (p_prc->tunein_playlist_);
  p_prc->p_tunein_ = NULL;
  p_prc->remove_current_url_ = false;
  return p_prc;
}

static void *
cc_tunein_prc_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "cc_tuneinprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
cc_tunein_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_tunein_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (tiz_srv_super_allocate_resources (
    typeOf (p_prc, "cc_tuneinprc"), p_prc, a_pid));

  tiz_check_omx (retrieve_tunein_session (p_prc));
  tiz_check_omx (retrieve_tunein_playlist (p_prc));

  on_tunein_error_ret_omx_oom (tiz_tunein_init (
    &(p_prc->p_tunein_)));

  tiz_check_omx (enqueue_tunein_playlist_items (p_prc));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_tunein_prc_deallocate_resources (void * ap_prc)
{
  cc_tunein_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_tunein_destroy (p_prc->p_tunein_);
  p_prc->p_tunein_ = NULL;
  return tiz_srv_super_deallocate_resources (typeOf (ap_prc, "cc_tuneinprc"),
                                             ap_prc);
}

static const char *
cc_tunein_prc_get_next_url (const void * p_obj)
{
  cc_tunein_prc_t * p_prc = (cc_tunein_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_tunein_);
  return tiz_tunein_get_next_url (p_prc->p_tunein_, p_prc->remove_current_url_);
}

static const char *
cc_tunein_prc_get_prev_url (const void * p_obj)
{
  cc_tunein_prc_t * p_prc = (cc_tunein_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_tunein_);
  return tiz_tunein_get_prev_url (p_prc->p_tunein_, p_prc->remove_current_url_);
}

static const char *
cc_tunein_prc_get_current_stream_album_art_url (const void * p_obj)
{
  cc_tunein_prc_t * p_prc = (cc_tunein_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_tunein_);
#define TUNEIN_LOGO "http://tizonia.org/img/tunein-logo.png"
  return TUNEIN_LOGO;
}

static OMX_ERRORTYPE
cc_tunein_prc_store_stream_metadata (cc_tunein_prc_t * ap_obj)
{
  cc_tunein_prc_t * p_prc = (cc_tunein_prc_t *) ap_obj;
  cc_prc_t * p_cc_prc = (cc_prc_t *) p_prc;
  assert (p_prc);

  /* Station Name */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Station",
                    tiz_tunein_get_current_radio_name (p_prc->p_tunein_)));

  /* Playback queue progress */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Item #",
                    tiz_tunein_get_current_queue_progress (p_prc->p_tunein_)));

  /* Station Description */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Description",
                    tiz_tunein_get_current_radio_description (p_prc->p_tunein_)));

  /* Type */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Type",
                    tiz_tunein_get_current_radio_type (p_prc->p_tunein_)));

  /* Station formats */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Format",
                    tiz_tunein_get_current_radio_format (p_prc->p_tunein_)));

  /* Station Bitrate */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Bitrate",
                    tiz_tunein_get_current_radio_bitrate (p_prc->p_tunein_)));

  /* Reliability */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Reliability",
    tiz_tunein_get_current_radio_reliability (p_prc->p_tunein_)));

  /* Streaming URL */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Streaming URL",
    tiz_tunein_get_current_radio_stream_url (p_prc->p_tunein_)));

  /* Thumbnail */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Thumbnail URL",
    tiz_tunein_get_current_radio_thumbnail_url (p_prc->p_tunein_)));

  return OMX_ErrorNone;
}

/*
 * cc_tunein_prc_class
 */

static void *
cc_tunein_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "cc_tuneinprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
cc_tunein_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_tuneinprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (cc_prc), "cc_tuneinprc_class", classOf (cc_prc),
     sizeof (cc_tunein_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_tunein_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_tuneinprc_class;
}

void *
cc_tunein_prc_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_tuneinprc_class = tiz_get_type (ap_hdl, "cc_tuneinprc_class");
  TIZ_LOG_CLASS (cc_tuneinprc_class);
  void * cc_tuneinprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_tuneinprc_class, "cc_tuneinprc", cc_prc, sizeof (cc_tunein_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_tunein_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_tunein_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, cc_tunein_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, cc_tunein_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_next_url, cc_tunein_prc_get_next_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_prev_url, cc_tunein_prc_get_prev_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_current_stream_album_art_url,
     cc_tunein_prc_get_current_stream_album_art_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_stream_metadata, cc_tunein_prc_store_stream_metadata,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_tuneinprc;
}

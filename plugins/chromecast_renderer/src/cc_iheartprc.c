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
 * @file   cc_iheartprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Iheart Chromecast renderer - processor class
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
#include "cc_iheartprc.h"
#include "cc_iheartprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc.iheart"
#endif

/* forward declarations */

#define on_iheart_error_ret_omx_oom(expr)                                    \
  do                                                                         \
    {                                                                        \
      int iheart_error = 0;                                                  \
      if (0 != (iheart_error = (expr)))                                      \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtiziheart [error %d]",                              \
                     iheart_error);                                          \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

static OMX_ERRORTYPE
retrieve_iheart_session (cc_iheart_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioIheartSession, &(ap_prc->iheart_session_));
}

static OMX_ERRORTYPE
retrieve_iheart_playlist (cc_iheart_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioIheartPlaylist, &(ap_prc->iheart_playlist_));
}

static OMX_ERRORTYPE
enqueue_iheart_playlist_items (cc_iheart_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_iheart_);

  {
    const char * p_playlist = (const char *) ap_prc->iheart_playlist_.cPlaylistName;
    const char * p_keywords1 = (const char *) ap_prc->iheart_playlist_.cAdditionalKeywords1;
    const char * p_keywords2 = (const char *) ap_prc->iheart_playlist_.cAdditionalKeywords2;
    const char * p_keywords3 = (const char *) ap_prc->iheart_playlist_.cAdditionalKeywords3;
    const OMX_BOOL shuffle = ap_prc->iheart_playlist_.bShuffle;

    tiz_iheart_set_playback_mode (
      ap_prc->p_iheart_, (shuffle == OMX_TRUE ? ETIZIheartPlaybackModeShuffle
                                          : ETIZIheartPlaybackModeNormal));

    switch (ap_prc->iheart_playlist_.ePlaylistType)
      {
        case OMX_AUDIO_IheartPlaylistTypeUnknown:
          {
            /* TODO */
            assert (0);
          }
          break;
        case OMX_AUDIO_IheartPlaylistTypeRadios:
          {
            rc = tiz_iheart_play_radios (ap_prc->p_iheart_, p_playlist,
                                         p_keywords1, p_keywords2, p_keywords3);
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
 * cc_iheartprc
 */

static void *
cc_iheart_prc_ctor (void * ap_obj, va_list * app)
{
  cc_iheart_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "cc_iheartprc"), ap_obj, app);
  TIZ_INIT_OMX_STRUCT (p_prc->iheart_session_);
  TIZ_INIT_OMX_STRUCT (p_prc->iheart_playlist_);
  p_prc->p_iheart_ = NULL;
  p_prc->remove_current_url_ = false;
  return p_prc;
}

static void *
cc_iheart_prc_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "cc_iheartprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
cc_iheart_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_iheart_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (tiz_srv_super_allocate_resources (
    typeOf (p_prc, "cc_iheartprc"), p_prc, a_pid));

  tiz_check_omx (retrieve_iheart_session (p_prc));
  tiz_check_omx (retrieve_iheart_playlist (p_prc));

  on_iheart_error_ret_omx_oom (tiz_iheart_init (
    &(p_prc->p_iheart_)));

  tiz_check_omx (enqueue_iheart_playlist_items (p_prc));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_iheart_prc_deallocate_resources (void * ap_prc)
{
  cc_iheart_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_iheart_destroy (p_prc->p_iheart_);
  p_prc->p_iheart_ = NULL;
  return tiz_srv_super_deallocate_resources (typeOf (ap_prc, "cc_iheartprc"),
                                             ap_prc);
}

static const char *
cc_iheart_prc_get_next_url (const void * p_obj)
{
  cc_iheart_prc_t * p_prc = (cc_iheart_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_iheart_);
  return tiz_iheart_get_next_url (p_prc->p_iheart_, p_prc->remove_current_url_);
}

static const char *
cc_iheart_prc_get_prev_url (const void * p_obj)
{
  cc_iheart_prc_t * p_prc = (cc_iheart_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_iheart_);
  return tiz_iheart_get_prev_url (p_prc->p_iheart_, p_prc->remove_current_url_);
}

static const char *
cc_iheart_prc_get_current_stream_album_art_url (const void * p_obj)
{
  cc_iheart_prc_t * p_prc = (cc_iheart_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_iheart_);
#define IHEART_LOGO "https://tizonia.org/img/iheart-logo.png"
  return IHEART_LOGO;
}

static OMX_ERRORTYPE
cc_iheart_prc_store_stream_metadata (cc_iheart_prc_t * ap_obj)
{
  cc_iheart_prc_t * p_prc = (cc_iheart_prc_t *) ap_obj;
  cc_prc_t * p_cc_prc = (cc_prc_t *) p_prc;
  assert (p_prc);

  /* Station Name */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Station",
                    tiz_iheart_get_current_radio_name (p_prc->p_iheart_)));

  /* Playback queue progress */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Item #",
                    tiz_iheart_get_current_queue_progress (p_prc->p_iheart_)));

  /* Station Description */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Description",
                    tiz_iheart_get_current_radio_description (p_prc->p_iheart_)));

  /* City */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "City",
                    tiz_iheart_get_current_radio_city (p_prc->p_iheart_)));

  /* State */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "State",
                    tiz_iheart_get_current_radio_state (p_prc->p_iheart_)));

  /* Website */
  tiz_check_omx (
    cc_prc_store_stream_metadata_item (p_cc_prc, "Website",
                    tiz_iheart_get_current_radio_website_url (p_prc->p_iheart_)));

  /* Streaming URL */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Streaming URL",
    tiz_iheart_get_current_radio_stream_url (p_prc->p_iheart_)));

  /* Thumbnail */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Thumbnail URL",
    tiz_iheart_get_current_radio_thumbnail_url (p_prc->p_iheart_)));

  return OMX_ErrorNone;
}

/*
 * cc_iheart_prc_class
 */

static void *
cc_iheart_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "cc_iheartprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
cc_iheart_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_iheartprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (cc_prc), "cc_iheartprc_class", classOf (cc_prc),
     sizeof (cc_iheart_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_iheart_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_iheartprc_class;
}

void *
cc_iheart_prc_init (void * ap_tos, void * ap_hdl)
{
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_iheartprc_class = tiz_get_type (ap_hdl, "cc_iheartprc_class");
  TIZ_LOG_CLASS (cc_iheartprc_class);
  void * cc_iheartprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_iheartprc_class, "cc_iheartprc", cc_prc, sizeof (cc_iheart_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_iheart_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_iheart_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, cc_iheart_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, cc_iheart_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_next_url, cc_iheart_prc_get_next_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_prev_url, cc_iheart_prc_get_prev_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_current_stream_album_art_url,
     cc_iheart_prc_get_current_stream_album_art_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_stream_metadata, cc_iheart_prc_store_stream_metadata,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_iheartprc;
}

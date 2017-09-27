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
 * @file   cc_dirbleprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Dirble chromecast renderer - processor class
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
static OMX_ERRORTYPE
cc_dirble_prc_deallocate_resources (void *);
static OMX_ERRORTYPE
release_buffer (cc_dirble_prc_t *);
static OMX_ERRORTYPE
cc_dirble_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid);
static OMX_ERRORTYPE
cc_dirble_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid);

#define on_lib_error_ret_omx_oom(expr)                                    \
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
store_metadata (cc_dirble_prc_t * ap_prc, const char * ap_header_name,
                const char * ap_header_info)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_CONFIG_METADATAITEMTYPE * p_meta = NULL;
  size_t metadata_len = 0;
  size_t info_len = 0;

  assert (ap_prc);
  if (ap_header_name && ap_header_info)
    {
      info_len = strnlen (ap_header_info, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
      metadata_len = sizeof (OMX_CONFIG_METADATAITEMTYPE) + info_len;

      if (!(p_meta
            = (OMX_CONFIG_METADATAITEMTYPE *) tiz_mem_calloc (1, metadata_len)))
        {
          rc = OMX_ErrorInsufficientResources;
        }
      else
        {
          const size_t name_len
            = strnlen (ap_header_name, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
          strncpy ((char *) p_meta->nKey, ap_header_name, name_len - 1);
          p_meta->nKey[name_len - 1] = '\0';
          p_meta->nKeySizeUsed = name_len;

          strncpy ((char *) p_meta->nValue, ap_header_info, info_len - 1);
          p_meta->nValue[info_len - 1] = '\0';
          p_meta->nValueMaxSize = info_len;
          p_meta->nValueSizeUsed = info_len;

          p_meta->nSize = metadata_len;
          p_meta->nVersion.nVersion = OMX_VERSION;
          p_meta->eScopeMode = OMX_MetadataScopeAllLevels;
          p_meta->nScopeSpecifier = 0;
          p_meta->nMetadataItemIndex = 0;
          p_meta->eSearchMode = OMX_MetadataSearchValueSizeByIndex;
          p_meta->eKeyCharset = OMX_MetadataCharsetASCII;
          p_meta->eValueCharset = OMX_MetadataCharsetASCII;

          rc = tiz_krn_store_metadata (tiz_get_krn (handleOf (ap_prc)), p_meta);
        }
    }
  return rc;
}

static inline void
delete_uri (cc_dirble_prc_t * ap_prc)
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static OMX_ERRORTYPE
update_metadata (cc_dirble_prc_t * ap_prc)
{
  assert (ap_prc);

  /* Clear previous metatada items */
  tiz_krn_clear_metadata (tiz_get_krn (handleOf (ap_prc)));

  /* Station Name */
  tiz_check_omx (
    store_metadata (ap_prc, "Station",
                    tiz_dirble_get_current_station_name (ap_prc->p_db_)));

  /* Country */
  tiz_check_omx (store_metadata (
    ap_prc, "URL", (const char *) ap_prc->p_uri_param_->contentURI));

  /* Country */
  tiz_check_omx (store_metadata (
    ap_prc, "Country",
    tiz_dirble_get_current_station_country (ap_prc->p_db_)));

  /* Category */
  tiz_check_omx (store_metadata (
    ap_prc, "Categories",
    tiz_dirble_get_current_station_category (ap_prc->p_db_)));

  /* Website */
  tiz_check_omx (store_metadata (
    ap_prc, "Website",
    tiz_dirble_get_current_station_website (ap_prc->p_db_)));

  /* Signal that a new set of metatadata items is available */
  (void) tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventIndexSettingChanged,
                              OMX_ALL, /* no particular port associated */
                              OMX_IndexConfigMetadataItem, /* index of the
                                                             struct that has
                                                             been modififed */
                              NULL);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
obtain_next_url (cc_dirble_prc_t * ap_prc, int a_skip_value)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);
  assert (ap_prc->p_db_);

  if (!ap_prc->p_uri_param_)
    {
      ap_prc->p_uri_param_ = tiz_mem_calloc (
        1, sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1);
    }

  tiz_check_null_ret_oom (ap_prc->p_uri_param_);

  ap_prc->p_uri_param_->nSize
    = sizeof (OMX_PARAM_CONTENTURITYPE) + pathname_max + 1;
  ap_prc->p_uri_param_->nVersion.nVersion = OMX_VERSION;

  {
    const char * p_next_url
      = a_skip_value > 0 ? tiz_dirble_get_next_url (ap_prc->p_db_,
                                                    ap_prc->remove_current_url_)
                         : tiz_dirble_get_prev_url (
                             ap_prc->p_db_, ap_prc->remove_current_url_);
    ap_prc->remove_current_url_ = false;
    tiz_check_null_ret_oom (p_next_url);

    {
      const OMX_U32 url_len = strnlen (p_next_url, pathname_max);
      TIZ_TRACE (handleOf (ap_prc), "URL [%s]", p_next_url);

      /* Verify we are getting an chromecast scheme */
      if (!p_next_url || !url_len
          || (memcmp (p_next_url, "chromecast://", 7) != 0
              && memcmp (p_next_url, "chromecasts://", 8) != 0))
        {
          rc = OMX_ErrorContentURIError;
        }
      else
        {
          strncpy ((char *) ap_prc->p_uri_param_->contentURI, p_next_url,
                   url_len);
          ap_prc->p_uri_param_->contentURI[url_len] = '\000';

          /* Song metadata is now available, update the IL client */
          rc = update_metadata (ap_prc);
        }
    }
  }

  return rc;
}

static OMX_ERRORTYPE
release_buffer (cc_dirble_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_inhdr_)
    {
      if (ap_prc->eos_)
        {
          ap_prc->eos_ = false;
          ap_prc->p_inhdr_->nFlags |= OMX_BUFFERFLAG_EOS;
        }
      tiz_check_omx (tiz_krn_release_buffer (
        tiz_get_krn (handleOf (ap_prc)),
        ARATELIA_CHROMECAST_RENDERER_PORT_INDEX, ap_prc->p_inhdr_));
      ap_prc->p_inhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
retrieve_session_configuration (cc_dirble_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioDirbleSession, &(ap_prc->db_session_));
}

static OMX_ERRORTYPE
retrieve_playlist (cc_dirble_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioDirblePlaylist, &(ap_prc->playlist_));
}

static OMX_ERRORTYPE
enqueue_playlist_items (cc_dirble_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_db_);

  {
    const char * p_playlist = (const char *) ap_prc->playlist_.cPlaylistName;
    const OMX_BOOL shuffle = ap_prc->playlist_.bShuffle;

    tiz_dirble_set_playback_mode (
      ap_prc->p_db_, (shuffle == OMX_TRUE ? ETIZDirblePlaybackModeShuffle
                                              : ETIZDirblePlaybackModeNormal));

    switch (ap_prc->playlist_.ePlaylistType)
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
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_skip_);
  TIZ_INIT_OMX_STRUCT (p_prc->cc_session_);
  p_prc->p_uri_param_ = NULL;
  p_prc->p_inhdr_ = NULL;
  p_prc->p_db_ = NULL;
  p_prc->p_cc_ = NULL;
  p_prc->eos_ = false;
  p_prc->port_disabled_ = false;
  p_prc->uri_changed_ = false;
  p_prc->remove_current_url_ = false;
  return p_prc;
}

static void *
cc_dirble_prc_dtor (void * ap_obj)
{
  (void) cc_dirble_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "cc_dirbleprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
cc_dirble_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_dirble_prc_t * p_prc = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  assert (p_prc);
  tiz_check_omx (retrieve_session_configuration (p_prc));
  tiz_check_omx (retrieve_playlist (p_prc));

  on_lib_error_ret_omx_oom (tiz_dirble_init (
    &(p_prc->p_db_), (const char *) p_prc->db_session_.cApiKey));

  tiz_check_omx (enqueue_playlist_items (p_prc));
  tiz_check_omx (obtain_next_url (p_prc, 1));

  return rc;
}

static OMX_ERRORTYPE
cc_dirble_prc_deallocate_resources (void * ap_prc)
{
  cc_dirble_prc_t * p_prc = ap_prc;
  assert (p_prc);
  delete_uri (p_prc);
  tiz_dirble_destroy (p_prc->p_db_);
  p_prc->p_db_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_dirble_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  cc_dirble_prc_t * p_prc = ap_prc;
  assert (ap_prc);
  p_prc->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_dirble_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
{
  cc_dirble_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  return rc;
}

static OMX_ERRORTYPE
cc_dirble_prc_stop_and_return (void * ap_prc)
{
  cc_dirble_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return release_buffer (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
cc_dirble_prc_buffers_ready (const void * ap_prc)
{
  cc_dirble_prc_t * p_prc = (cc_dirble_prc_t *) ap_prc;
  assert (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_dirble_prc_timer_ready (void * ap_prc, tiz_event_timer_t * ap_ev_timer,
                           void * ap_arg, const uint32_t a_id)
{
  cc_dirble_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_dirble_prc_pause (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_dirble_prc_resume (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_dirble_prc_port_flush (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  cc_dirble_prc_t * p_prc = (cc_dirble_prc_t *) ap_obj;
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
cc_dirble_prc_port_disable (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  cc_dirble_prc_t * p_prc = (cc_dirble_prc_t *) ap_obj;
  assert (p_prc);
  p_prc->port_disabled_ = true;
  /* Release any buffers held  */
  return release_buffer ((cc_dirble_prc_t *) ap_obj);
}

static OMX_ERRORTYPE
cc_dirble_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  cc_dirble_prc_t * p_prc = (cc_dirble_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->port_disabled_)
    {
      p_prc->port_disabled_ = false;
      if (p_prc->uri_changed_)
        {
          p_prc->uri_changed_ = false;
        }
    }
  return rc;
}

static OMX_ERRORTYPE
cc_dirble_prc_config_change (void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid),
                             OMX_INDEXTYPE a_config_idx)
{
  cc_dirble_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_prc);

  if (OMX_TizoniaIndexConfigPlaylistSkip == a_config_idx && p_prc->p_cc_)
    {
      TIZ_INIT_OMX_STRUCT (p_prc->playlist_skip_);
      tiz_check_omx (tiz_api_GetConfig (
        tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
        OMX_TizoniaIndexConfigPlaylistSkip, &p_prc->playlist_skip_));
      p_prc->playlist_skip_.nValue > 0 ? obtain_next_url (p_prc, 1)
                                       : obtain_next_url (p_prc, -1);
      /* Changing the URL has the side effect of halting the current
         download */
      if (p_prc->port_disabled_)
        {
          /* Record that the URI has changed, so that when the port is
             re-enabled, we restart the transfer */
          p_prc->uri_changed_ = true;
        }

    }
  return rc;
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
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_dirbleprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "cc_dirbleprc_class", classOf (tizprc),
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
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_dirbleprc_class = tiz_get_type (ap_hdl, "cc_dirbleprc_class");
  TIZ_LOG_CLASS (cc_dirbleprc_class);
  void * cc_dirbleprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_dirbleprc_class, "cc_dirbleprc", tizprc, sizeof (cc_dirble_prc_t),
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
     tiz_srv_prepare_to_transfer, cc_dirble_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, cc_dirble_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, cc_dirble_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_timer_ready, cc_dirble_prc_timer_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, cc_dirble_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, cc_dirble_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, cc_dirble_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, cc_dirble_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, cc_dirble_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, cc_dirble_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_config_change, cc_dirble_prc_config_change,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_dirbleprc;
}

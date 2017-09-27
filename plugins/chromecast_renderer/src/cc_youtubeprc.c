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
 * @file   cc_youtubeprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Youtube chromecast renderer - processor class
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
#include "cc_youtubeprc.h"
#include "cc_youtubeprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.chromecast_renderer.prc.youtube"
#endif

/* forward declarations */
static OMX_ERRORTYPE
cc_youtube_prc_deallocate_resources (void *);
static OMX_ERRORTYPE
release_buffer (cc_youtube_prc_t *);
static OMX_ERRORTYPE
cc_youtube_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid);
static OMX_ERRORTYPE
cc_youtube_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid);

#define on_lib_error_ret_omx_oom(expr)                                   \
  do                                                                         \
    {                                                                        \
      int youtube_error = 0;                                                 \
      if (0 != (youtube_error = (expr)))                                     \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtizyoutube [error %d]",                             \
                     youtube_error);                                         \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)


static OMX_ERRORTYPE
store_metadata (cc_youtube_prc_t * ap_prc, const char * ap_header_name,
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

      if (NULL == (p_meta = (OMX_CONFIG_METADATAITEMTYPE *) tiz_mem_calloc (
                     1, metadata_len)))
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
delete_uri (cc_youtube_prc_t * ap_prc)
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static OMX_ERRORTYPE
update_metadata (cc_youtube_prc_t * ap_prc)
{
  assert (ap_prc);

  /* Clear previous metadata items */
  tiz_krn_clear_metadata (tiz_get_krn (handleOf (ap_prc)));

  /* Audio stream title */
  tiz_check_omx (store_metadata (
    ap_prc, tiz_youtube_get_current_audio_stream_author (ap_prc->p_yt_),
    tiz_youtube_get_current_audio_stream_title (ap_prc->p_yt_)));

  /* ID */
  tiz_check_omx (store_metadata (
    ap_prc, "YouTube Id",
    tiz_youtube_get_current_audio_stream_video_id (ap_prc->p_yt_)));

  /* Duration */
  tiz_check_omx (store_metadata (
    ap_prc, "Duration",
    tiz_youtube_get_current_audio_stream_duration (ap_prc->p_yt_)));

  /* File Format */
  tiz_check_omx (store_metadata (
    ap_prc, "File Format",
    tiz_youtube_get_current_audio_stream_file_extension (ap_prc->p_yt_)));

  /* Bitrate */
  tiz_check_omx (store_metadata (
    ap_prc, "Bitrate",
    tiz_youtube_get_current_audio_stream_bitrate (ap_prc->p_yt_)));

  /* File Size */
  tiz_check_omx (store_metadata (
    ap_prc, "Size",
    tiz_youtube_get_current_audio_stream_file_size (ap_prc->p_yt_)));

  /* View count */
  tiz_check_omx (store_metadata (
    ap_prc, "View Count",
    tiz_youtube_get_current_audio_stream_view_count (ap_prc->p_yt_)));

  /* Description */
  tiz_check_omx (store_metadata (
    ap_prc, "Description",
    tiz_youtube_get_current_audio_stream_description (ap_prc->p_yt_)));

  /* Publication date/time */
  tiz_check_omx (store_metadata (
    ap_prc, "Published",
    tiz_youtube_get_current_audio_stream_published (ap_prc->p_yt_)));

  /* Signal that a new set of metadata items is available */
  (void) tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventIndexSettingChanged,
                              OMX_ALL, /* no particular port associated */
                              OMX_IndexConfigMetadataItem, /* index of the
                                                             struct that has
                                                             been modififed */
                              NULL);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
obtain_next_url (cc_youtube_prc_t * ap_prc, int a_skip_value)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);
  assert (ap_prc->p_yt_);

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
      = a_skip_value > 0 ? tiz_youtube_get_next_url (
                             ap_prc->p_yt_, ap_prc->remove_current_url_)
                         : tiz_youtube_get_prev_url (
                             ap_prc->p_yt_, ap_prc->remove_current_url_);
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
release_buffer (cc_youtube_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_inhdr_)
    {
      if (ap_prc->bytes_before_eos_ > ap_prc->p_inhdr_->nFilledLen)
        {
          ap_prc->bytes_before_eos_ -= ap_prc->p_inhdr_->nFilledLen;
        }
      else
        {
          ap_prc->bytes_before_eos_ = 0;
          ap_prc->eos_ = true;
        }

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
retrieve_session_configuration (cc_youtube_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioYoutubeSession, &(ap_prc->yt_session_));
}

static OMX_ERRORTYPE
retrieve_playlist (cc_youtube_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioYoutubePlaylist, &(ap_prc->playlist_));
}

static OMX_ERRORTYPE
enqueue_playlist_items (cc_youtube_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_yt_);

  {
    const char * p_playlist = (const char *) ap_prc->playlist_.cPlaylistName;
    const OMX_BOOL shuffle = ap_prc->playlist_.bShuffle;

    tiz_youtube_set_playback_mode (
      ap_prc->p_yt_,
      (shuffle == OMX_TRUE ? ETIZYoutubePlaybackModeShuffle
                           : ETIZYoutubePlaybackModeNormal));

    switch (ap_prc->playlist_.ePlaylistType)
      {
        case OMX_AUDIO_YoutubePlaylistTypeUnknown:
          {
            /* TODO */
            assert (0);
          }
          break;
        case OMX_AUDIO_YoutubePlaylistTypeAudioStream:
          {
            rc = tiz_youtube_play_audio_stream (ap_prc->p_yt_, p_playlist);
          }
          break;
        case OMX_AUDIO_YoutubePlaylistTypeAudioPlaylist:
          {
            rc = tiz_youtube_play_audio_playlist (ap_prc->p_yt_,
                                                  p_playlist);
          }
          break;
        case OMX_AUDIO_YoutubePlaylistTypeAudioMix:
          {
            rc = tiz_youtube_play_audio_mix (ap_prc->p_yt_, p_playlist);
          }
          break;
        case OMX_AUDIO_YoutubePlaylistTypeAudioSearch:
          {
            rc = tiz_youtube_play_audio_search (ap_prc->p_yt_, p_playlist);
          }
          break;
        case OMX_AUDIO_YoutubePlaylistTypeAudioMixSearch:
          {
            rc = tiz_youtube_play_audio_mix_search (ap_prc->p_yt_,
                                                    p_playlist);
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
 * cc_youtubeprc
 */

static void *
cc_youtube_prc_ctor (void * ap_obj, va_list * app)
{
  cc_youtube_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "cc_youtubeprc"), ap_obj, app);
  TIZ_INIT_OMX_STRUCT (p_prc->yt_session_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_skip_);
  p_prc->p_uri_param_ = NULL;
  p_prc->p_inhdr_ = NULL;
  p_prc->p_yt_ = NULL;
  p_prc->p_cc_ = NULL;
  p_prc->eos_ = false;
  p_prc->port_disabled_ = false;
  p_prc->uri_changed_ = false;
  p_prc->bytes_before_eos_ = 0;
  p_prc->remove_current_url_ = false;
  return p_prc;
}

static void *
cc_youtube_prc_dtor (void * ap_obj)
{
  (void) cc_youtube_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "cc_youtubeprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
cc_youtube_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_youtube_prc_t * p_prc = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  assert (p_prc);
  tiz_check_omx (retrieve_session_configuration (p_prc));
  tiz_check_omx (retrieve_playlist (p_prc));

  on_lib_error_ret_omx_oom (tiz_youtube_init (&(p_prc->p_yt_)));

  tiz_check_omx (enqueue_playlist_items (p_prc));
  tiz_check_omx (obtain_next_url (p_prc, 1));

  return rc;
}

static OMX_ERRORTYPE
cc_youtube_prc_deallocate_resources (void * ap_prc)
{
  cc_youtube_prc_t * p_prc = ap_prc;
  assert (p_prc);
  delete_uri (p_prc);
  tiz_youtube_destroy (p_prc->p_yt_);
  p_prc->p_yt_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_youtube_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  cc_youtube_prc_t * p_prc = ap_prc;
  assert (ap_prc);
  p_prc->eos_ = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_youtube_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
{
  cc_youtube_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  return rc;
}

static OMX_ERRORTYPE
cc_youtube_prc_stop_and_return (void * ap_prc)
{
  cc_youtube_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return release_buffer (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
cc_youtube_prc_buffers_ready (const void * ap_prc)
{
  cc_youtube_prc_t * p_prc = (cc_youtube_prc_t *) ap_prc;
  assert (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_youtube_prc_io_ready (void * ap_prc, tiz_event_io_t * ap_ev_io, int a_fd,
                         int a_events)
{
  cc_youtube_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_youtube_prc_timer_ready (void * ap_prc, tiz_event_timer_t * ap_ev_timer,
                            void * ap_arg, const uint32_t a_id)
{
  cc_youtube_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_youtube_prc_pause (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_youtube_prc_resume (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_youtube_prc_port_flush (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  cc_youtube_prc_t * p_prc = (cc_youtube_prc_t *) ap_obj;
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
cc_youtube_prc_port_disable (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  cc_youtube_prc_t * p_prc = (cc_youtube_prc_t *) ap_obj;
  assert (p_prc);
  p_prc->port_disabled_ = true;
  /* Release any buffers held  */
  return release_buffer ((cc_youtube_prc_t *) ap_obj);
}

static OMX_ERRORTYPE
cc_youtube_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  cc_youtube_prc_t * p_prc = (cc_youtube_prc_t *) ap_prc;
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
cc_youtube_prc_config_change (void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid),
                              OMX_INDEXTYPE a_config_idx)
{
  cc_youtube_prc_t * p_prc = ap_prc;
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
 * cc_youtube_prc_class
 */

static void *
cc_youtube_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "cc_youtubeprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
cc_youtube_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_youtubeprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "cc_youtubeprc_class", classOf (tizprc),
     sizeof (cc_youtube_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_youtube_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return cc_youtubeprc_class;
}

void *
cc_youtube_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * cc_youtubeprc_class = tiz_get_type (ap_hdl, "cc_youtubeprc_class");
  TIZ_LOG_CLASS (cc_youtubeprc_class);
  void * cc_youtubeprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_youtubeprc_class, "cc_youtubeprc", tizprc, sizeof (cc_youtube_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, cc_youtube_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, cc_youtube_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, cc_youtube_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, cc_youtube_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, cc_youtube_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, cc_youtube_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, cc_youtube_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_io_ready, cc_youtube_prc_io_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_timer_ready, cc_youtube_prc_timer_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, cc_youtube_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, cc_youtube_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, cc_youtube_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, cc_youtube_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, cc_youtube_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, cc_youtube_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_config_change, cc_youtube_prc_config_change,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_youtubeprc;
}

/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @brief  YouTube Chromecast renderer - processor class
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

#define on_yt_error_ret_omx_oom(expr)                                        \
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
retrieve_yt_session (cc_youtube_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioYoutubeSession, &(ap_prc->yt_session_));
}

static OMX_ERRORTYPE
retrieve_yt_playlist (cc_youtube_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioYoutubePlaylist, &(ap_prc->yt_playlist_));
}

static OMX_ERRORTYPE
enqueue_yt_playlist_items (cc_youtube_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_yt_);

  {
    const char * p_playlist = (const char *) ap_prc->yt_playlist_.cPlaylistName;
    const OMX_BOOL shuffle = ap_prc->yt_playlist_.bShuffle;

    tiz_youtube_set_playback_mode (
      ap_prc->p_yt_, (shuffle == OMX_TRUE ? ETIZYoutubePlaybackModeShuffle
                                          : ETIZYoutubePlaybackModeNormal));

    switch (ap_prc->yt_playlist_.ePlaylistType)
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
            rc = tiz_youtube_play_audio_playlist (ap_prc->p_yt_, p_playlist);
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
            rc = tiz_youtube_play_audio_mix_search (ap_prc->p_yt_, p_playlist);
          }
          break;
        case OMX_AUDIO_YoutubePlaylistTypeAudioChannelUploads:
          {
            rc = tiz_youtube_play_audio_channel_uploads (ap_prc->p_yt_, p_playlist);
          }
          break;
        case OMX_AUDIO_YoutubePlaylistTypeAudioChannelPlaylist:
          {
            rc = tiz_youtube_play_audio_channel_playlist (ap_prc->p_yt_, p_playlist);
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
  TIZ_INIT_OMX_STRUCT (p_prc->yt_playlist_);
  p_prc->p_yt_ = NULL;
  p_prc->remove_current_url_ = false;
  return p_prc;
}

static void *
cc_youtube_prc_dtor (void * ap_obj)
{
  return super_dtor (typeOf (ap_obj, "cc_youtubeprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
cc_youtube_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  cc_youtube_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (tiz_srv_super_allocate_resources (
    typeOf (p_prc, "cc_youtubeprc"), p_prc, a_pid));

  tiz_check_omx (retrieve_yt_session (p_prc));
  tiz_check_omx (retrieve_yt_playlist (p_prc));

  on_yt_error_ret_omx_oom (tiz_youtube_init (&(p_prc->p_yt_)));

  tiz_check_omx (enqueue_yt_playlist_items (p_prc));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cc_youtube_prc_deallocate_resources (void * ap_prc)
{
  cc_youtube_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_youtube_destroy (p_prc->p_yt_);
  p_prc->p_yt_ = NULL;
  return tiz_srv_super_deallocate_resources (typeOf (ap_prc, "cc_youtubeprc"),
                                             ap_prc);
}

static const char *
cc_youtube_prc_get_next_url (const void * p_obj)
{
  cc_youtube_prc_t * p_prc = (cc_youtube_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_yt_);
  return tiz_youtube_get_next_url (p_prc->p_yt_, p_prc->remove_current_url_);
}

static const char *
cc_youtube_prc_get_prev_url (const void * p_obj)
{
  cc_youtube_prc_t * p_prc = (cc_youtube_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_yt_);
  return tiz_youtube_get_prev_url (p_prc->p_yt_, p_prc->remove_current_url_);
}

static const char *
cc_youtube_prc_get_current_stream_album_art_url (const void * p_obj)
{
  cc_youtube_prc_t * p_prc = (cc_youtube_prc_t *) p_obj;
  assert (p_prc);
  assert (p_prc->p_yt_);
#define YT_LOGO "http://tizonia.org/img/youtube-logo.png"
  return YT_LOGO;
}

static OMX_ERRORTYPE
cc_youtube_prc_store_stream_metadata (const void * p_obj)
{
  cc_youtube_prc_t * p_prc = (cc_youtube_prc_t *) p_obj;
  cc_prc_t * p_cc_prc = (cc_prc_t *) p_obj;
  assert (p_prc);

  /* Audio stream title */
  {
    const char * p_author
      = tiz_youtube_get_current_audio_stream_author (p_prc->p_yt_);
    const char * p_title
      = tiz_youtube_get_current_audio_stream_title (p_prc->p_yt_);
    tiz_check_omx (cc_prc_store_display_title (p_cc_prc, p_author, p_title));
    tiz_check_omx (
      cc_prc_store_stream_metadata_item (p_cc_prc, p_author, p_title));
  }

  /* ID */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "YouTube Id",
    tiz_youtube_get_current_audio_stream_video_id (p_prc->p_yt_)));

  /* Duration */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Duration",
    tiz_youtube_get_current_audio_stream_duration (p_prc->p_yt_)));

  /* File Format */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "File Format",
    tiz_youtube_get_current_audio_stream_file_extension (p_prc->p_yt_)));

  /* Bitrate */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Bitrate",
    tiz_youtube_get_current_audio_stream_bitrate (p_prc->p_yt_)));

  /* File Size */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Size",
    tiz_youtube_get_current_audio_stream_file_size (p_prc->p_yt_)));

  /* View count */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "View Count",
    tiz_youtube_get_current_audio_stream_view_count (p_prc->p_yt_)));

  /* Description */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Description",
    tiz_youtube_get_current_audio_stream_description (p_prc->p_yt_)));

  /* Publication date/time */
  tiz_check_omx (cc_prc_store_stream_metadata_item (
    p_cc_prc, "Published",
    tiz_youtube_get_current_audio_stream_published (p_prc->p_yt_)));

  return OMX_ErrorNone;
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
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_youtubeprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (cc_prc), "cc_youtubeprc_class", classOf (cc_prc),
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
  void * cc_prc = tiz_get_type (ap_hdl, "cc_prc");
  void * cc_youtubeprc_class = tiz_get_type (ap_hdl, "cc_youtubeprc_class");
  TIZ_LOG_CLASS (cc_youtubeprc_class);
  void * cc_youtubeprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (cc_youtubeprc_class, "cc_youtubeprc", cc_prc, sizeof (cc_youtube_prc_t),
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
     cc_prc_get_next_url, cc_youtube_prc_get_next_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_prev_url, cc_youtube_prc_get_prev_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_get_current_stream_album_art_url,
     cc_youtube_prc_get_current_stream_album_art_url,
     /* TIZ_CLASS_COMMENT: */
     cc_prc_store_stream_metadata, cc_youtube_prc_store_stream_metadata,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return cc_youtubeprc;
}

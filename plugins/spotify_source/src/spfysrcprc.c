/**
  * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   spfysrcprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Spotify client component
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "spfysrc.h"
#include "spfysrcprc.h"
#include "spfysrcprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.spotify_source.prc"
#endif

/* This macro assumes the existence of an "ap_prc" local variable */
#define goto_end_on_sp_error(expr)                         \
  do                                                       \
    {                                                      \
      sp_error sperr = SP_ERROR_OK;                        \
      if ((sperr = (expr)) != SP_ERROR_OK)                 \
        {                                                  \
          TIZ_ERROR (handleOf (ap_prc),                    \
                     "[OMX_ErrorInsufficientResources] : " \
                     "%s",                                 \
                     sp_error_message (sperr));            \
          goto end;                                        \
        }                                                  \
    }                                                      \
  while (0)

/* Forward declarations */
static OMX_ERRORTYPE spfysrc_prc_deallocate_resources (void *ap_obj);

/* The application key, specific to each project. */
extern const uint8_t g_appkey[];
/* The size of the application key. */
extern const size_t g_appkey_size;

typedef struct spfy_logged_in_data spfy_logged_in_data_t;
struct spfy_logged_in_data
{
  sp_session *p_sess;
  sp_error error;
};

typedef struct spfy_music_delivery_data spfy_music_delivery_data_t;
struct spfy_music_delivery_data
{
  sp_session *p_sess;
  sp_audioformat format;
  const void *p_frames;
  int num_frames;
};

typedef struct spfy_playlist_added_removed_data
    spfy_playlist_added_removed_data_t;
struct spfy_playlist_added_removed_data
{
  sp_playlistcontainer *p_pc;
  sp_playlist *p_pl;
  int position;
};

typedef struct spfy_tracks_operation_data spfy_tracks_operation_data_t;
struct spfy_tracks_operation_data
{
  sp_playlist *p_pl;
  sp_track *const *p_track_handles; /* An array of track handles */
  const int *p_track_indices;       /* An array of track indices */
  int num_tracks;
  int position;
};

static void reset_stream_parameters (spfysrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_buffer_clear (ap_prc->p_store_);
  ap_prc->initial_cache_bytes_
      = ((ARATELIA_SPOTIFY_SOURCE_DEFAULT_BIT_RATE_KBITS * 1000) / 8)
        * ARATELIA_SPOTIFY_SOURCE_DEFAULT_CACHE_SECONDS;
}

static void skip_tracks (spfysrc_prc_t *ap_prc, const OMX_S32 a_skip_value)
{
  assert (NULL != ap_prc);

  if (ap_prc->p_sp_playlist_ && a_skip_value != 0)
    {
      const int list_size = sp_playlist_num_tracks (ap_prc->p_sp_playlist_);
      int new_track_index = ap_prc->track_index_ + a_skip_value;
      if (new_track_index >= list_size)
        {
          new_track_index %= list_size;
        }
      else if (new_track_index < 0)
        {
          new_track_index = list_size - abs (new_track_index);
        }

      assert (new_track_index >= 0 && new_track_index < list_size);
      TIZ_TRACE (handleOf (ap_prc),
                 "ap_prc->track_index_ [%d] new index [%d]",
                 ap_prc->track_index_, new_track_index);
      ap_prc->track_index_ = new_track_index;
    }
}

static void update_track_index (spfysrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (0 != ap_prc->playlist_skip_.nValue)
    {
      skip_tracks (ap_prc, ap_prc->playlist_skip_.nValue);
      ap_prc->playlist_skip_.nValue = 0;
    }
  else
    {
      ap_prc->track_index_++;
      if (ap_prc->track_index_
          >= sp_playlist_num_tracks (ap_prc->p_sp_playlist_))
        {
          ap_prc->track_index_ = 0;
        }
    }
}

static void process_spotify_event (spfysrc_prc_t *ap_prc,
                                   tiz_event_pluggable_hdlr_f apf_hdlr,
                                   void *ap_data)
{
  tiz_event_pluggable_t *p_event = NULL;
  assert (NULL != ap_prc);
  assert (NULL != apf_hdlr);
  assert (NULL != ap_data);

  p_event = tiz_mem_calloc (1, sizeof(tiz_event_pluggable_t));
  if (p_event)
    {
      p_event->p_servant = ap_prc;
      p_event->pf_hdlr = apf_hdlr;
      p_event->p_data = ap_data;
      apf_hdlr (ap_prc, p_event);
    }
}

static void post_spotify_event (spfysrc_prc_t *ap_prc,
                                tiz_event_pluggable_hdlr_f apf_hdlr,
                                void *ap_data)
{
  tiz_event_pluggable_t *p_event = NULL;
  assert (NULL != ap_prc);
  assert (NULL != apf_hdlr);
  assert (NULL != ap_data);

  p_event = tiz_mem_calloc (1, sizeof(tiz_event_pluggable_t));
  if (p_event)
    {
      p_event->p_servant = ap_prc;
      p_event->pf_hdlr = apf_hdlr;
      p_event->p_data = ap_data;
      tiz_comp_event_pluggable (handleOf (ap_prc), p_event);
    }
}

static OMX_ERRORTYPE prepare_for_port_auto_detection (spfysrc_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_SPOTIFY_SOURCE_PORT_INDEX);
  tiz_check_omx_err (
      tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_IndexParamPortDefinition, &port_def));
  ap_prc->audio_coding_type_ = port_def.format.audio.eEncoding;
  ap_prc->auto_detect_on_
      = (OMX_AUDIO_CodingAutoDetect == ap_prc->audio_coding_type_) ? true
                                                                   : false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE set_audio_coding_on_port (spfysrc_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_SPOTIFY_SOURCE_PORT_INDEX);
  tiz_check_omx_err (
      tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_IndexParamPortDefinition, &port_def));

  /* Set the new value */
  port_def.format.audio.eEncoding = ap_prc->audio_coding_type_;

  tiz_check_omx_err (tiz_krn_SetParameter_internal (
      tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
      OMX_IndexParamPortDefinition, &port_def));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE set_pcm_audio_info_on_port (spfysrc_prc_t *ap_prc)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (pcmmode, ARATELIA_SPOTIFY_SOURCE_PORT_INDEX);
  tiz_check_omx_err (tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)),
                                           handleOf (ap_prc),
                                           OMX_IndexParamAudioPcm, &pcmmode));

  /* Set the new values */
  pcmmode.nChannels = ap_prc->num_channels_;
  pcmmode.nSamplingRate = ap_prc->samplerate_;
  pcmmode.bInterleaved = OMX_TRUE;
  pcmmode.nBitPerSample = 16;

  tiz_check_omx_err (tiz_krn_SetParameter_internal (
      tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
      OMX_IndexParamAudioPcm, &pcmmode));
  return OMX_ErrorNone;
}

static void send_port_settings_change_event (spfysrc_prc_t *ap_prc)
{
  assert (ap_prc);
  TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_EventPortSettingsChanged");
  tiz_srv_issue_event ((OMX_PTR)ap_prc, OMX_EventPortSettingsChanged,
                       ARATELIA_SPOTIFY_SOURCE_PORT_INDEX, /* port 0 */
                       OMX_IndexParamPortDefinition,       /* the index of the
                                            struct that has
                                            been modififed */
                       NULL);
}

static void send_port_auto_detect_events (spfysrc_prc_t *ap_prc)
{
  assert (ap_prc);

  TIZ_PRINTF_DBG_MAG ("send_port_auto_detect_events");

  if (ap_prc->audio_coding_type_ != OMX_AUDIO_CodingUnused
      || ap_prc->audio_coding_type_ != OMX_AUDIO_CodingAutoDetect)
    {
      TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_EventPortFormatDetected");
      tiz_srv_issue_event ((OMX_PTR)ap_prc, OMX_EventPortFormatDetected, 0, 0,
                           NULL);
      send_port_settings_change_event (ap_prc);
    }
  else
    {
      /* Oops... could not detect the stream format */
      tiz_srv_issue_err_event ((OMX_PTR)ap_prc, OMX_ErrorFormatNotDetected);
    }
}

static OMX_ERRORTYPE store_metadata (spfysrc_prc_t *ap_prc, const char *ap_key,
                                     const char *ap_value)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_CONFIG_METADATAITEMTYPE *p_meta = NULL;
  size_t metadata_len = 0;
  size_t info_len = 0;

  assert (NULL != ap_prc);
  assert (NULL != ap_key);
  assert (NULL != ap_value);

  info_len = strnlen (ap_value, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
  metadata_len = sizeof(OMX_CONFIG_METADATAITEMTYPE) + info_len;

  if (NULL == (p_meta = (OMX_CONFIG_METADATAITEMTYPE *)tiz_mem_calloc (
                   1, metadata_len)))
    {
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      const size_t name_len = strnlen (ap_key, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
      strncpy ((char *)p_meta->nKey, ap_key, name_len - 1);
      p_meta->nKey[name_len - 1] = '\0';
      p_meta->nKeySizeUsed = name_len;

      strncpy ((char *)p_meta->nValue, ap_value, info_len - 1);
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

  return rc;
}

static void store_metadata_playlist (spfysrc_prc_t *ap_prc,
                                     const char *a_playlist_name,
                                     const int a_num_tracks)
{
  char playlist_info[OMX_MAX_STRINGNAME_SIZE];
  assert (NULL != a_playlist_name);
  snprintf (playlist_info, OMX_MAX_STRINGNAME_SIZE - 1, "%d tracks",
            a_num_tracks);
  (void)store_metadata (ap_prc, a_playlist_name, playlist_info);
}

static void store_metadata_track_name (spfysrc_prc_t *ap_prc,
                                       const char *a_track_name,
                                       const int a_track_index,
                                       const int a_num_tracks)
{
  char name_str[OMX_MAX_STRINGNAME_SIZE];
  assert (NULL != a_track_name);
  snprintf (name_str, OMX_MAX_STRINGNAME_SIZE - 1, "%s (%d of %d)",
            a_track_name, a_track_index + 1, a_num_tracks);
  (void)store_metadata (ap_prc, "Track", name_str);
}

static void store_metadata_track_duration (spfysrc_prc_t *ap_prc,
                                           const int a_duration_ms)
{
  if (a_duration_ms > 0)
    {
      int duration = a_duration_ms / 1000;
      char duration_str[OMX_MAX_STRINGNAME_SIZE];
      int seconds = duration % 60;
      int minutes = (duration - seconds) / 60;
      int hours = 0;

      assert (ap_prc);

      if (minutes >= 60)
        {
          int total_minutes = minutes;
          minutes = total_minutes % 60;
          hours = (total_minutes - minutes) / 60;
        }

      if (hours > 0)
        {
          snprintf (duration_str, OMX_MAX_STRINGNAME_SIZE - 1, "%2ih:%2im:%2is",
                    hours, minutes, seconds);
        }
      else if (minutes > 0)
        {
          snprintf (duration_str, OMX_MAX_STRINGNAME_SIZE - 1, "%2im:%2is",
                    minutes, seconds);
        }
      else
        {
          snprintf (duration_str, OMX_MAX_STRINGNAME_SIZE - 1, "%2is", seconds);
        }
      (void)store_metadata (ap_prc, "Duration", duration_str);
    }
}

static void store_relevant_track_metadata (spfysrc_prc_t *ap_prc, const int a_num_tracks)
{
  assert (NULL != ap_prc);
  (void)tiz_krn_clear_metadata (tiz_get_krn (handleOf (ap_prc)));
  (void)store_metadata_playlist (
      ap_prc, sp_playlist_name (ap_prc->p_sp_playlist_), a_num_tracks);
  (void)store_metadata (ap_prc, "Artist", sp_artist_name (sp_track_artist (
                                              ap_prc->p_sp_track_, 0)));
  (void)store_metadata (ap_prc, "Album",
                        sp_album_name (sp_track_album (ap_prc->p_sp_track_)));
  store_metadata_track_name (ap_prc, sp_track_name (ap_prc->p_sp_track_),
                             ap_prc->track_index_, a_num_tracks);
  store_metadata_track_duration (ap_prc,
                                 sp_track_duration (ap_prc->p_sp_track_));
}

static OMX_ERRORTYPE retrieve_session_configuration (spfysrc_prc_t *ap_prc)
{
  return tiz_api_GetParameter (
      tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
      OMX_TizoniaIndexParamAudioSpotifySession, &(ap_prc->session_));
}

static OMX_ERRORTYPE retrieve_playlist (spfysrc_prc_t *ap_prc)
{
  return tiz_api_GetParameter (
      tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
      OMX_TizoniaIndexParamAudioSpotifyPlaylist, &(ap_prc->playlist_));
}

static sp_connection_type omx_connection_type_to_spfy (
    const OMX_TIZONIA_AUDIO_SPOTIFYCONNECTIONTYPE a_conn_type)
{
  sp_connection_type spfy_conn = OMX_AUDIO_SpotifyConnectionUnknown;
  switch (a_conn_type)
    {
      case OMX_AUDIO_SpotifyConnectionUnknown:
        {
          spfy_conn = SP_CONNECTION_TYPE_UNKNOWN;
        }
        break;
      case OMX_AUDIO_SpotifyConnectionNone:
        {
          spfy_conn = SP_CONNECTION_TYPE_NONE;
        }
        break;
      case OMX_AUDIO_SpotifyConnectionMobile:
        {
          spfy_conn = SP_CONNECTION_TYPE_MOBILE;
        }
        break;
      case OMX_AUDIO_SpotifyConnectionMobileRoaming:
        {
          spfy_conn = SP_CONNECTION_TYPE_MOBILE_ROAMING;
        }
        break;
      case OMX_AUDIO_SpotifyConnectionMobileWifi:
        {
          spfy_conn = SP_CONNECTION_TYPE_WIFI;
        }
        break;
      case OMX_AUDIO_SpotifyConnectionMobileWired:
        {
          spfy_conn = SP_CONNECTION_TYPE_WIRED;
        }
        break;
      default:
        {
          assert (0);
        }
        break;
    }
  return spfy_conn;
}

static sp_bitrate omx_preferred_bitrate_to_spfy (
    const OMX_TIZONIA_AUDIO_SPOTIFYBITRATETYPE a_bitrate_type, int *ap_bitrate)
{
  sp_bitrate spfy_bitrate = SP_BITRATE_320k;
  assert (ap_bitrate);
  switch (a_bitrate_type)
    {
      case OMX_AUDIO_SpotifyBitrate160Kbps:
        {
          spfy_bitrate = SP_BITRATE_160k;
          *ap_bitrate = 160;
        }
        break;
      case OMX_AUDIO_SpotifyBitrate320Kbps:
        {
          spfy_bitrate = SP_BITRATE_320k;
          *ap_bitrate = 320;
        }
        break;
      case OMX_AUDIO_SpotifyBitrate96Kbps:
        {
          spfy_bitrate = SP_BITRATE_96k;
          *ap_bitrate = 96;
        }
        break;
      default:
        {
          assert (0);
        }
        break;
    }
  return spfy_bitrate;
}

static OMX_ERRORTYPE set_spotify_session_options (spfysrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  int bitrate = 0;

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_sp_session_);

  goto_end_on_sp_error (sp_session_set_connection_type (
      ap_prc->p_sp_session_,
      omx_connection_type_to_spfy (ap_prc->session_.eConnectionType)));

  goto_end_on_sp_error (sp_session_preferred_bitrate (
      ap_prc->p_sp_session_,
      omx_preferred_bitrate_to_spfy (ap_prc->session_.ePreferredBitRate,
                                     &bitrate)));

  ap_prc->min_cache_bytes_ = ((bitrate * 1000) / 8)
                             * ARATELIA_SPOTIFY_SOURCE_MIN_CACHE_SECONDS;

  ap_prc->max_cache_bytes_ = ((bitrate * 1000) / 8)
                             * ARATELIA_SPOTIFY_SOURCE_MAX_CACHE_SECONDS;

  /* All OK */
  rc = OMX_ErrorNone;

end:

  return rc;
}

static OMX_ERRORTYPE allocate_temp_data_store (spfysrc_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (NULL != ap_prc);
  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_SPOTIFY_SOURCE_PORT_INDEX);
  tiz_check_omx_err (
      tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_IndexParamPortDefinition, &port_def));
  assert (ap_prc->p_store_ == NULL);
  return tiz_buffer_init (&(ap_prc->p_store_), port_def.nBufferSize);
}

static inline void deallocate_temp_data_store (
    /*@special@ */ spfysrc_prc_t *ap_prc)
/*@releases ap_prc->p_store_@ */
/*@ensures isnull ap_prc->p_store_@ */
{
  assert (NULL != ap_prc);
  tiz_buffer_destroy (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
}

static inline int copy_to_omx_buffer (OMX_BUFFERHEADERTYPE *ap_hdr,
                                      const void *ap_src, const int nbytes)
{
  int n = MIN (nbytes, ap_hdr->nAllocLen - ap_hdr->nFilledLen);
  (void)memcpy (ap_hdr->pBuffer + ap_hdr->nOffset, ap_src, n);
  ap_hdr->nFilledLen += n;
  ap_hdr->nOffset += n;
  return n;
}

static OMX_ERRORTYPE release_buffer (spfysrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);

  if (ap_prc->p_outhdr_)
    {
      OMX_BUFFERHEADERTYPE *p_hdr = ap_prc->p_outhdr_;
/*       TIZ_NOTICE (handleOf (ap_prc), */
/*                   "releasing HEADER [%p] nFilledLen [%d] store [%d] [%d - %d] " */
/*                   "spotify paused [%s]", */
/*                   p_hdr, p_hdr->nFilledLen, */
/*                   tiz_buffer_bytes_available (ap_prc->p_store_), */
/*                   ap_prc->min_cache_bytes_, ap_prc->max_cache_bytes_, */
/*                   ap_prc->spotify_paused_ ? "YES" : "NO"); */
      if (ap_prc->eos_)
        {
          ap_prc->bytes_till_eos_ -= p_hdr->nFilledLen;
          if (ap_prc->bytes_till_eos_ <= 0)
            {
              ap_prc->bytes_till_eos_ = 0;
              ap_prc->eos_ = false;
              p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
            }
        }
      p_hdr->nOffset = 0;
      tiz_check_omx_err (
          tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)), 0, p_hdr));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_BUFFERHEADERTYPE *buffer_needed (spfysrc_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  assert (NULL != ap_prc);

  if (!ap_prc->port_disabled_)
    {
      if (NULL != ap_prc->p_outhdr_)
        {
          p_hdr = ap_prc->p_outhdr_;
        }
      else
        {
          if (OMX_ErrorNone
              == (tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)),
                                        ARATELIA_SPOTIFY_SOURCE_PORT_INDEX, 0,
                                        &ap_prc->p_outhdr_)))
            {
              if (NULL != ap_prc->p_outhdr_)
                {
/*                   TIZ_TRACE (handleOf (ap_prc), */
/*                              "Claimed HEADER [%p]...nFilledLen [%d]", */
/*                              ap_prc->p_outhdr_, ap_prc->p_outhdr_->nFilledLen); */
                  p_hdr = ap_prc->p_outhdr_;
                }
            }
        }
    }
  return p_hdr;
}

static void control_cache_size (spfysrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);

  if (ap_prc->p_sp_session_ && !ap_prc->initial_cache_bytes_)
    {
      const int current_cache_bytes
          = tiz_buffer_bytes_available (ap_prc->p_store_);
      if (current_cache_bytes > ap_prc->max_cache_bytes_
          && !ap_prc->spotify_paused_)
        {
          /* Temporarily pause spotify music delivery */
          sp_session_player_play (ap_prc->p_sp_session_, false);
          ap_prc->spotify_paused_ = true;
        }
      else if (current_cache_bytes <= ap_prc->min_cache_bytes_
               && ap_prc->spotify_paused_)
        {
          /* Re-start spotify music delivery */
          sp_session_player_play (ap_prc->p_sp_session_, true);
          ap_prc->spotify_paused_ = false;
        }
    }
}

static OMX_ERRORTYPE consume_cache (spfysrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);

  /* Also, control here the delivery of the next eos flag */
  if (ap_prc->eos_ && ap_prc->bytes_till_eos_ <= 0)
    {
      ap_prc->bytes_till_eos_ = tiz_buffer_bytes_available (ap_prc->p_store_);
    }

  if (tiz_buffer_bytes_available (ap_prc->p_store_)
      > ap_prc->initial_cache_bytes_)
    {
      int nbytes_stored = 0;
      OMX_BUFFERHEADERTYPE *p_out = NULL;

      /* Reset the initial size */
      ap_prc->initial_cache_bytes_ = 0;

      while ((nbytes_stored = tiz_buffer_bytes_available (ap_prc->p_store_)) > 0
             && (p_out = buffer_needed (ap_prc)) != NULL)
        {
          int nbytes_copied = copy_to_omx_buffer (
              p_out, tiz_buffer_get_data (ap_prc->p_store_), nbytes_stored);
          tiz_check_omx_err (release_buffer (ap_prc));
          (void)tiz_buffer_advance (ap_prc->p_store_, nbytes_copied);
          p_out = NULL;
        }
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE process_spotify_session_events (spfysrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);

  if (ap_prc->p_sp_session_ && ap_prc->keep_processing_sp_events_)
    {
      do
        {
          sp_session_process_events (ap_prc->p_sp_session_,
                                     &(ap_prc->next_timeout_));
        }
      while (0 == ap_prc->next_timeout_);

      rc = tiz_srv_timer_watcher_start (ap_prc, ap_prc->p_ev_timer_,
                                        ap_prc->next_timeout_ / 1000, 0);
    }
  return rc;
}

static void stop_spotify_session_timer (spfysrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (ap_prc->next_timeout_ && ap_prc->p_ev_timer_)
    {
      (void)tiz_srv_timer_watcher_stop (ap_prc, ap_prc->p_ev_timer_);
      ap_prc->next_timeout_ = 0;
    }
}

static void stop_playback (spfysrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  if (ap_prc->p_sp_track_ != NULL)
    {
      sp_session_player_unload (ap_prc->p_sp_session_);
      ap_prc->p_sp_track_ = NULL;
      stop_spotify_session_timer (ap_prc);
    }
}

/**
 * Called on various events to start playback if it hasn't been started already.
 *
 * The function simply starts playing the first track of the playlist.
 */
static void start_playback (spfysrc_prc_t *ap_prc)
{
#define verify_or_return(outcome, msg)                \
  do                                                  \
    {                                                 \
      if (!outcome)                                   \
        {                                             \
          TIZ_DEBUG (handleOf (ap_prc), "[%s]", msg); \
          return;                                     \
        }                                             \
    }                                                 \
  while (0)

  sp_track *p_track = NULL;

  assert (NULL != ap_prc);

  if (ap_prc->transfering_ && ap_prc->p_sp_playlist_)
    {
      const int num_tracks = sp_playlist_num_tracks (ap_prc->p_sp_playlist_);
      verify_or_return ((num_tracks > 0), "No tracks in playlist. Waiting.");
      assert (ap_prc->track_index_ < num_tracks);

      p_track
          = sp_playlist_track (ap_prc->p_sp_playlist_, ap_prc->track_index_);

      if (ap_prc->p_sp_track_ && p_track != ap_prc->p_sp_track_)
        {
          /* Someone changed the current track */
          stop_playback (ap_prc);
        }

      verify_or_return ((p_track != NULL), "Track is NULL. Waiting.");
      verify_or_return ((sp_track_error (p_track) == SP_ERROR_OK),
                        "Track error. Waiting.");
      if (ap_prc->p_sp_track_ != p_track)
        {
          ap_prc->p_sp_track_ = p_track;
          store_relevant_track_metadata (ap_prc, num_tracks);
          sp_session_player_load (ap_prc->p_sp_session_, p_track);
          sp_session_player_play (ap_prc->p_sp_session_, 1);
          ap_prc->spotify_inited_ = true;
        }
    }
}

static void logged_in_cback_handler (OMX_PTR ap_prc,
                                     tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data)
    {
      spfy_logged_in_data_t *p_lg_data = ap_event->p_data;

      if (SP_ERROR_OK == p_lg_data->error)
        {
          int i = 0;
          int nplaylists = 0;
          sp_error sp_rc = SP_ERROR_OK;
          sp_playlistcontainer *pc
              = sp_session_playlistcontainer (p_lg_data->p_sess);
          assert (NULL != pc);

          set_spotify_session_options (p_prc);

          sp_rc = sp_playlistcontainer_add_callbacks (
              pc, &(p_prc->sp_plct_cbacks_), p_prc);
          assert (SP_ERROR_OK == sp_rc);

          nplaylists = sp_playlistcontainer_num_playlists (pc);

          for (i = 0; i < nplaylists; ++i)
            {
              sp_playlist *pl = sp_playlistcontainer_playlist (pc, i);
              assert (NULL != pl);

              sp_rc = sp_playlist_add_callbacks (pl, &(p_prc->sp_pl_cbacks_),
                                                 p_prc);
              assert (SP_ERROR_OK == sp_rc);

              if (!strcasecmp (sp_playlist_name (pl),
                               (const char *)p_prc->playlist_.cPlayListName))
                {
                  p_prc->p_sp_playlist_ = pl;
                  start_playback (p_prc);
                }
            }

          if (!p_prc->p_sp_playlist_)
            {
              TIZ_PRINTF_DBG_MAG (
                  "No such playlist. Waiting for one to pop up...");
            }
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * This callback is called when an attempt to login has succeeded or failed.
 *
 */
static void logged_in (sp_session *sess, sp_error error)
{
  spfy_logged_in_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_logged_in_data_t));
  if (p_data)
    {
      p_data->p_sess = sess;
      p_data->error = error;
      process_spotify_event (sp_session_userdata (sess),
                             logged_in_cback_handler, p_data);
    }
}

static void notify_main_thread_cback_handler (OMX_PTR ap_prc,
                                              tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);
  tiz_mem_free (ap_event);
  p_prc->keep_processing_sp_events_ = true;
  if (!p_prc->stopping_)
    {
      (void)process_spotify_session_events (p_prc);
    }
}

/**
 * This callback is called from an internal libspotify thread to ask us to
 * reiterate the main loop.
 *
 * @note This function is called from an internal session thread!
 */
static void notify_main_thread (sp_session *sess)
{
  post_spotify_event (sp_session_userdata (sess),
                      notify_main_thread_cback_handler, sess);
}

static void metadata_updated_cback_handler (OMX_PTR ap_prc,
                                            tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (ap_event->p_data && p_prc->p_sp_track_)
    {
      if (sp_track_error (p_prc->p_sp_track_) == SP_ERROR_OK)
        {
          TIZ_TRACE (handleOf (ap_prc), "Track = [%s]",
                     sp_track_name (p_prc->p_sp_track_));
        }
    }
  tiz_mem_free (ap_event);
  start_playback (p_prc);
}

/**
 * Callback called when libspotify has new metadata available
 *
 */
static void metadata_updated (sp_session *sess)
{
  process_spotify_event (sp_session_userdata (sess),
                         metadata_updated_cback_handler, sess);
}

static void music_delivery_cback_handler (OMX_PTR ap_prc,
                                          tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;

  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (!p_prc->stopping_ && ap_event->p_data)
    {
      spfy_music_delivery_data_t *p_data = ap_event->p_data;
      int nbytes = p_data->num_frames * sizeof(int16_t)
                   * p_data->format.channels;
      OMX_U8 *p_in = (OMX_U8 *)p_data->p_frames;
      OMX_BUFFERHEADERTYPE *p_out = NULL;

      if (!p_prc->spotify_paused_)
        {
          consume_cache (p_prc);
          while (nbytes > 0 && (p_out = buffer_needed (p_prc)) != NULL)
            {
              int nbytes_copied = copy_to_omx_buffer (p_out, p_in, nbytes);
              (void)release_buffer (p_prc);
              nbytes -= nbytes_copied;
              p_in += nbytes_copied;
            }
        }

      if (nbytes > 0)
        {
          int nbytes_stored = 0;
          if ((nbytes_stored = tiz_buffer_store_data (p_prc->p_store_, p_in,
                                                      nbytes)) < nbytes)
            {
              TIZ_ERROR (handleOf (p_prc),
                         "Unable to store all the data (wanted %d, "
                         "stored %d).",
                         nbytes, nbytes_stored);
              assert (0);
            }
          nbytes -= nbytes_stored;
        }
      if (p_prc->auto_detect_on_
          || p_prc->num_channels_ != p_data->format.channels
          || p_prc->samplerate_ != p_data->format.sample_rate)
        {
          p_prc->auto_detect_on_ = false;
          p_prc->num_channels_ = p_data->format.channels;
          p_prc->samplerate_ = p_data->format.sample_rate;
          p_prc->audio_coding_type_ = OMX_AUDIO_CodingPCM;
          set_audio_coding_on_port (p_prc);
          set_pcm_audio_info_on_port (p_prc);
          /* And now trigger the OMX_EventPortFormatDetected and
             OMX_EventPortSettingsChanged events or a
             OMX_ErrorFormatNotDetected event */
          send_port_auto_detect_events (p_prc);
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * This callback is used from libspotify whenever there is PCM data available.
 *
 * @note This function is called from an internal session thread!
 */
static int music_delivery (sp_session *sess, const sp_audioformat *format,
                           const void *frames, int num_frames)
{
  if (num_frames > 0)
    {
      spfy_music_delivery_data_t *p_data
          = tiz_mem_calloc (1, sizeof(spfy_music_delivery_data_t));
      if (p_data)
        {
          assert (NULL != sess);
          assert (NULL != format);
          assert (NULL != frames);
          p_data->p_sess = sess;
          p_data->format.sample_type = format->sample_type;
          p_data->format.sample_rate = format->sample_rate;
          p_data->format.channels = format->channels;
          p_data->p_frames = frames;
          p_data->num_frames = num_frames;
          post_spotify_event (sp_session_userdata (sess),
                              music_delivery_cback_handler, p_data);
        }
    }
  control_cache_size (sp_session_userdata (sess));
  return num_frames;
}

static void end_of_track_cback_handler (OMX_PTR ap_prc,
                                        tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  if (!p_prc->stopping_ && ap_event->p_data)
    {
      int tracks = 0;
      p_prc->eos_ = true;

      if (p_prc->p_sp_track_)
        {
          TIZ_PRINTF_DBG_MAG ("end of track = [%s]",
                              sp_track_name (p_prc->p_sp_track_));
          TIZ_TRACE (handleOf (ap_prc), "end of track = [%s]",
                     sp_track_name (p_prc->p_sp_track_));
          sp_session_player_unload (p_prc->p_sp_session_);
          p_prc->p_sp_track_ = NULL;
          if (p_prc->remove_tracks_)
            {
              sp_playlist_remove_tracks (p_prc->p_sp_playlist_, &tracks, 1);
            }
          else
            {
              update_track_index (p_prc);
            }

          assert (p_prc->track_index_
                  < sp_playlist_num_tracks (p_prc->p_sp_playlist_));
          assert (p_prc->track_index_ >= 0);
        }

      start_playback (p_prc);
      (void)process_spotify_session_events (p_prc);
    }
  tiz_mem_free (ap_event);
}

/**
 * This callback is used from libspotify when the current track has ended
 *
 * @note This function is called from an internal session thread!
 */
static void end_of_track (sp_session *sess)
{
  post_spotify_event (sp_session_userdata (sess), end_of_track_cback_handler,
                      sess);
}

static void play_token_lost_cback_handler (OMX_PTR ap_prc,
                                           tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);
  TIZ_PRINTF_DBG_MAG ("play_token_lost");
  stop_playback (p_prc);
  tiz_mem_free (ap_event);
}

/**
 * Notification that some other connection has started playing on this account.
 * Playback has been stopped.
 *
 */
static void play_token_lost (sp_session *sess)
{
  process_spotify_event (sp_session_userdata (sess),
                         play_token_lost_cback_handler, sess);
}

static void log_message (sp_session *sess, const char *msg)
{
  TIZ_PRINTF_DBG_MAG ("%s", msg);
}

/* --------------------  PLAYLIST CONTAINER CALLBACKS  --------------------- */

static void playlist_added_cback_handler (OMX_PTR ap_prc,
                                          tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  TIZ_PRINTF_DBG_MAG ("playlist_added");

  if (ap_event->p_data)
    {
      spfy_playlist_added_removed_data_t *p_data = ap_event->p_data;
      sp_playlist_add_callbacks (p_data->p_pl, &(p_prc->sp_pl_cbacks_), p_prc);

      TIZ_TRACE (handleOf (ap_prc), "playlist added = [%s]",
                 sp_playlist_name (p_data->p_pl));

      if (!strcasecmp (sp_playlist_name (p_data->p_pl),
                       (const char *)p_prc->playlist_.cPlayListName))
        {
          p_prc->p_sp_playlist_ = p_data->p_pl;
          start_playback (p_prc);
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, telling us a playlist was added to the playlist
 *container.
 *
 * We add our playlist callbacks to the newly added playlist.
 *
 * @param  pc            The playlist container handle
 * @param  pl            The playlist handle
 * @param  position      Index of the added playlist
 * @param  userdata      The opaque pointer
 */
static void playlist_added (sp_playlistcontainer *pc, sp_playlist *pl,
                            int position, void *userdata)
{
  spfy_playlist_added_removed_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_playlist_added_removed_data_t));
  if (p_data)
    {
      p_data->p_pc = pc;
      p_data->p_pl = pl;
      p_data->position = position;
      process_spotify_event (userdata, playlist_added_cback_handler, p_data);
    }
}

static void playlist_removed_cback_handler (OMX_PTR ap_prc,
                                            tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  TIZ_PRINTF_DBG_MAG ("playlist_removed");

  if (ap_event->p_data)
    {
      spfy_playlist_added_removed_data_t *p_data = ap_event->p_data;
      TIZ_TRACE (handleOf (ap_prc), "playlist removed = [%s]",
                 sp_playlist_name (p_data->p_pl));
      sp_playlist_remove_callbacks (p_data->p_pl, &(p_prc->sp_pl_cbacks_),
                                    NULL);
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, telling us a playlist was removed from the playlist
 *container.
 *
 * This is the place to remove our playlist callbacks.
 *
 * @param  pc            The playlist container handle
 * @param  pl            The playlist handle
 * @param  position      Index of the removed playlist
 * @param  userdata      The opaque pointer
 */
static void playlist_removed (sp_playlistcontainer *pc, sp_playlist *pl,
                              int position, void *userdata)
{
  spfy_playlist_added_removed_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_playlist_added_removed_data_t));
  if (p_data)
    {
      p_data->p_pc = pc;
      p_data->p_pl = pl;
      p_data->position = position;
      process_spotify_event (userdata, playlist_removed_cback_handler, p_data);
    }
}

static void container_loaded_cback_handler (OMX_PTR ap_prc,
                                            tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  TIZ_PRINTF_DBG_MAG ("tracks_added");

  if (ap_event->p_data)
    {
      int i = 0;
      int nplaylists = 0;
      sp_error sp_rc = SP_ERROR_OK;
      sp_playlistcontainer *pc = ap_event->p_data;
      bool playlist_found = false;
      TIZ_PRINTF_DBG_MAG (
          "container_loaded; Rootlist synchronized (%d playlists)",
          sp_playlistcontainer_num_playlists (pc));
      TIZ_DEBUG (handleOf (ap_prc), "Rootlist synchronized (%d playlists)",
                 sp_playlistcontainer_num_playlists (pc));

      sp_rc = sp_playlistcontainer_add_callbacks (pc, &(p_prc->sp_plct_cbacks_),
                                                  p_prc);
      assert (SP_ERROR_OK == sp_rc);

      nplaylists = sp_playlistcontainer_num_playlists (pc);
      for (i = 0; i < nplaylists; ++i)
        {
          sp_playlist *pl = sp_playlistcontainer_playlist (pc, i);
          assert (NULL != pl);

          sp_rc
              = sp_playlist_add_callbacks (pl, &(p_prc->sp_pl_cbacks_), p_prc);
          assert (SP_ERROR_OK == sp_rc);

          TIZ_DEBUG (handleOf (ap_prc), "playlist : %s",
                     sp_playlist_name (pl));

          if (!strcasecmp (sp_playlist_name (pl),
                           (const char *)p_prc->playlist_.cPlayListName))
            {
              p_prc->p_sp_playlist_ = pl;
              start_playback (p_prc);
              playlist_found = true;
              break;
            }
        }
      if (!playlist_found)
        {
          tiz_srv_issue_err_event ((OMX_PTR)ap_prc, OMX_ErrorContentURIError);
        }
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, telling us the rootlist is fully synchronized
 * We just print an informational message
 *
 * @param  pc            The playlist container handle
 * @param  userdata      The opaque pointer
 */
static void container_loaded (sp_playlistcontainer *pc, void *userdata)
{
  process_spotify_event (userdata, container_loaded_cback_handler, pc);
}

static void tracks_added_cback_handler (OMX_PTR ap_prc,
                                        tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  TIZ_PRINTF_DBG_MAG ("tracks_added");

  if (ap_event->p_data)
    {
      spfy_tracks_operation_data_t *p_data = ap_event->p_data;
      if (p_data->p_pl == p_prc->p_sp_playlist_)
        {
          TIZ_DEBUG (handleOf (p_prc), "%d tracks were added",
                     p_data->num_tracks);
          start_playback (p_prc);
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, saying that a track has been added to a playlist.
 *
 * @param  pl          The playlist handle
 * @param  tracks      An array of track handles
 * @param  num_tracks  The number of tracks in the \c tracks array
 * @param  position    Where the tracks were inserted
 * @param  userdata    The opaque pointer
 */
static void tracks_added (sp_playlist *pl, sp_track *const *tracks,
                          int num_tracks, int position, void *userdata)
{
  spfy_tracks_operation_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_tracks_operation_data_t));
  if (p_data)
    {
      p_data->p_pl = pl;
      p_data->p_track_handles = tracks;
      p_data->num_tracks = num_tracks;
      p_data->position = position;
      process_spotify_event (userdata, tracks_added_cback_handler, p_data);
    }
}

static void tracks_removed_cback_handler (OMX_PTR ap_prc,
                                          tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  TIZ_PRINTF_DBG_MAG ("tracks_removed");

  if (ap_event->p_data)
    {
      spfy_tracks_operation_data_t *p_data = ap_event->p_data;
      int i = 0;
      int k = 0;
      if (p_data->p_pl == p_prc->p_sp_playlist_)
        {
          for (i = 0; i < p_data->num_tracks; ++i)
            {
              if (p_data->p_track_indices[i] < p_prc->track_index_)
                {
                  ++k;
                }
            }
          p_prc->track_index_ -= k;
          TIZ_DEBUG (handleOf (p_prc), "%d tracks have been removed",
                     p_data->num_tracks);
          start_playback (p_prc);
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, saying that a track has been added to a playlist.
 *
 * @param  pl          The playlist handle
 * @param  tracks      An array of track indices
 * @param  num_tracks  The number of tracks in the \c tracks array
 * @param  userdata    The opaque pointer
 */
static void tracks_removed (sp_playlist *pl, const int *tracks, int num_tracks,
                            void *userdata)
{
  spfy_tracks_operation_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_tracks_operation_data_t));
  if (p_data)
    {
      p_data->p_pl = pl;
      p_data->p_track_indices = tracks;
      p_data->num_tracks = num_tracks;
      process_spotify_event (userdata, tracks_removed_cback_handler, p_data);
    }
}

static void tracks_moved_cback_handler (OMX_PTR ap_prc,
                                        tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  TIZ_PRINTF_DBG_MAG ("tracks_moved");

  if (ap_event->p_data)
    {
      spfy_tracks_operation_data_t *p_data = ap_event->p_data;
      if (p_data->p_pl == p_prc->p_sp_playlist_)
        {
          TIZ_DEBUG (handleOf (p_prc), "%d tracks were moved around",
                     p_data->num_tracks);
          start_playback (p_prc);
        }
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify, telling when tracks have been moved around in a
 *playlist.
 *
 * @param  pl            The playlist handle
 * @param  tracks        An array of track indices
 * @param  num_tracks    The number of tracks in the \c tracks array
 * @param  new_position  To where the tracks were moved
 * @param  userdata      The opaque pointer
 */
static void tracks_moved (sp_playlist *pl, const int *tracks, int num_tracks,
                          int new_position, void *userdata)
{
  spfy_tracks_operation_data_t *p_data
      = tiz_mem_calloc (1, sizeof(spfy_tracks_operation_data_t));
  if (p_data)
    {
      p_data->p_pl = pl;
      p_data->p_track_indices = tracks;
      p_data->num_tracks = num_tracks;
      p_data->position = new_position;
      process_spotify_event (userdata, tracks_moved_cback_handler, p_data);
    }
}

static void playlist_renamed_cback_handler (OMX_PTR ap_prc,
                                            tiz_event_pluggable_t *ap_event)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  assert (NULL != ap_event);

  TIZ_PRINTF_DBG_MAG ("playlist_renamed");

  if (ap_event->p_data)
    {
      sp_playlist *pl = ap_event->p_data;
      const char *name = sp_playlist_name (pl);

      if (!strcasecmp (name, (const char *)p_prc->playlist_.cPlayListName))
        {
          p_prc->p_sp_playlist_ = pl;
          p_prc->track_index_ = 0;
          start_playback (p_prc);
        }
      else if (p_prc->p_sp_playlist_ == pl)
        {
          TIZ_DEBUG (handleOf (p_prc), "current playlist renamed to \"%s\".",
                     name);
          p_prc->p_sp_playlist_ = NULL;
          stop_playback (p_prc);
        }
    }
  tiz_mem_free (ap_event);
}

/**
 * Callback from libspotify. Something renamed the playlist.
 *
 * @param  pl            The playlist handle
 * @param  userdata      The opaque pointer
 */
static void playlist_renamed (sp_playlist *pl, void *userdata)
{
  process_spotify_event (userdata, playlist_renamed_cback_handler, pl);
}

/*
 * spfysrcprc
 */

static void *spfysrc_prc_ctor (void *ap_obj, va_list *app)
{
  spfysrc_prc_t *p_prc
      = super_ctor (typeOf (ap_obj, "spfysrcprc"), ap_obj, app);

  p_prc->p_outhdr_ = NULL;
  p_prc->p_uri_param_ = NULL;
  p_prc->eos_ = false;
  p_prc->bytes_till_eos_ = 0;
  p_prc->transfering_ = false;
  p_prc->stopping_ = false;
  p_prc->port_disabled_ = false;
  p_prc->spotify_inited_ = false;
  p_prc->spotify_paused_ = false;
  p_prc->initial_cache_bytes_ = 0;
  p_prc->min_cache_bytes_ = 0;
  p_prc->max_cache_bytes_ = 0;
  p_prc->p_store_ = NULL;
  p_prc->p_ev_timer_ = NULL;
  TIZ_INIT_OMX_STRUCT (p_prc->session_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_);
  p_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
  p_prc->num_channels_ = 2;
  p_prc->samplerate_ = 44100;
  p_prc->auto_detect_on_ = false;

  p_prc->track_index_ = 0;
  p_prc->p_sp_session_ = NULL;

  /* Init the spotify config struct */
  tiz_mem_set ((OMX_PTR) & p_prc->sp_config_, 0, sizeof(p_prc->sp_config_));
  p_prc->sp_config_.api_version = SPOTIFY_API_VERSION;
  p_prc->sp_config_.cache_location = "tmp";
  p_prc->sp_config_.settings_location = "tmp";
  p_prc->sp_config_.application_key = g_appkey;
  p_prc->sp_config_.application_key_size = g_appkey_size;
  p_prc->sp_config_.user_agent = "tizonia-source-component";
  p_prc->sp_config_.callbacks = &(p_prc->sp_cbacks_);
  p_prc->sp_config_.userdata = p_prc;
  p_prc->sp_config_.compress_playlists = false;
  p_prc->sp_config_.dont_save_metadata_for_playlists = true;
  p_prc->sp_config_.initially_unload_playlists = false;

  /* Init the spotify callbacks struct */
  tiz_mem_set ((OMX_PTR) & p_prc->sp_cbacks_, 0, sizeof(p_prc->sp_cbacks_));
  p_prc->sp_cbacks_.logged_in = &logged_in;
  p_prc->sp_cbacks_.notify_main_thread = &notify_main_thread;
  p_prc->sp_cbacks_.music_delivery = &music_delivery;
  p_prc->sp_cbacks_.metadata_updated = &metadata_updated;
  p_prc->sp_cbacks_.play_token_lost = &play_token_lost;
  p_prc->sp_cbacks_.log_message = &log_message;
  p_prc->sp_cbacks_.end_of_track = &end_of_track;

  /* Init the spotify playlist container callbacks struct */
  tiz_mem_set ((OMX_PTR) & p_prc->sp_plct_cbacks_, 0,
               sizeof(p_prc->sp_plct_cbacks_));
  p_prc->sp_plct_cbacks_.playlist_added = &playlist_added;
  p_prc->sp_plct_cbacks_.playlist_removed = &playlist_removed;
  p_prc->sp_plct_cbacks_.container_loaded = &container_loaded;

  /* Init the spotify playlist callbacks struct */
  tiz_mem_set ((OMX_PTR) & p_prc->sp_pl_cbacks_, 0,
               sizeof(p_prc->sp_pl_cbacks_));
  p_prc->sp_pl_cbacks_.tracks_added = &tracks_added;
  p_prc->sp_pl_cbacks_.tracks_removed = &tracks_removed;
  p_prc->sp_pl_cbacks_.tracks_moved = &tracks_moved;
  p_prc->sp_pl_cbacks_.playlist_renamed = &playlist_renamed;

  p_prc->p_sp_playlist_ = NULL;
  p_prc->p_sp_track_ = NULL;
  p_prc->remove_tracks_ = false;
  p_prc->keep_processing_sp_events_ = false;
  p_prc->next_timeout_ = 0;

  return p_prc;
}

static void *spfysrc_prc_dtor (void *ap_obj)
{
  (void)spfysrc_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "spfysrcprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE spfysrc_prc_allocate_resources (void *ap_prc,
                                                     OMX_U32 a_pid)
{
  spfysrc_prc_t *p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;

  assert (NULL != p_prc);
  assert (NULL == p_prc->p_ev_timer_);
  assert (NULL == p_prc->p_uri_param_);

  tiz_check_omx_err (allocate_temp_data_store (p_prc));
  tiz_check_omx_err (retrieve_session_configuration (p_prc));
  tiz_check_omx_err (retrieve_playlist (p_prc));
  tiz_check_omx_err (tiz_srv_timer_watcher_init (p_prc, &(p_prc->p_ev_timer_)));

  /* Create a spotify session */
  goto_end_on_sp_error (
      sp_session_create (&(p_prc->sp_config_), &(p_prc->p_sp_session_)));

  /* Initiate the login */
  goto_end_on_sp_error (sp_session_login (
      p_prc->p_sp_session_, (const char *)p_prc->session_.cUserName,
      (const char *)p_prc->session_.cUserPassword,
      0, /* If true, the username /
               password will be remembered
               by libspotify */
      NULL));

  /* All OK */
  rc = OMX_ErrorNone;

end:

  return rc;
}

static OMX_ERRORTYPE spfysrc_prc_deallocate_resources (void *ap_prc)
{
  spfysrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);
  if (p_prc->p_ev_timer_)
    {
      stop_spotify_session_timer (p_prc);
      tiz_srv_timer_watcher_destroy (p_prc, p_prc->p_ev_timer_);
      p_prc->p_ev_timer_ = NULL;
    }
  (void)sp_session_release (p_prc->p_sp_session_);
  p_prc->p_sp_session_ = NULL;
  p_prc->spotify_inited_ = false;
  deallocate_temp_data_store (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE spfysrc_prc_prepare_to_transfer (void *ap_obj,
                                                      OMX_U32 a_pid)
{
  reset_stream_parameters (ap_obj);
  return prepare_for_port_auto_detection (ap_obj);
}

static OMX_ERRORTYPE spfysrc_prc_transfer_and_process (void *ap_obj,
                                                       OMX_U32 a_pid)
{
  spfysrc_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  p_prc->transfering_ = true;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE spfysrc_prc_stop_and_return (void *ap_obj)
{
  spfysrc_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  p_prc->transfering_ = false;
  p_prc->stopping_ = true;
  stop_playback (p_prc);
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE spfysrc_prc_buffers_ready (const void *ap_obj)
{
  spfysrc_prc_t *p_prc = (spfysrc_prc_t *)ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != p_prc);
  if (!p_prc->spotify_inited_)
    {
      start_playback (p_prc);
    }
  if (p_prc->transfering_ && p_prc->spotify_paused_)
    {
      rc = consume_cache (p_prc);
      control_cache_size (p_prc);
    }
  else
    {
      rc = process_spotify_session_events (p_prc);
    }
  return rc;
}

static OMX_ERRORTYPE spfysrc_prc_timer_ready (void *ap_prc,
                                              tiz_event_timer_t *ap_ev_timer,
                                              void *ap_arg, const uint32_t a_id)
{
  spfysrc_prc_t *p_prc = (spfysrc_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != p_prc);
  p_prc->next_timeout_ = 0;
  if (!p_prc->stopping_)
    {
      rc = process_spotify_session_events (p_prc);
    }
  return rc;
}

static OMX_ERRORTYPE spfysrc_prc_pause (const void *ap_obj)
{
  /* TODO */
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE spfysrc_prc_resume (const void *ap_obj)
{
  /* TODO */
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE spfysrc_prc_port_flush (const void *ap_obj,
                                             OMX_U32 TIZ_UNUSED (a_pid))
{
  spfysrc_prc_t *p_prc = (spfysrc_prc_t *)ap_obj;
  assert (NULL != p_prc);
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE spfysrc_prc_port_disable (const void *ap_obj,
                                               OMX_U32 TIZ_UNUSED (a_pid))
{
  spfysrc_prc_t *p_prc = (spfysrc_prc_t *)ap_obj;
  assert (NULL != p_prc);
  TIZ_NOTICE (handleOf (p_prc), "Disabling port ; was disabled? [%s]",
              p_prc->port_disabled_ ? "YES" : "NO");
  p_prc->port_disabled_ = true;
  stop_spotify_session_timer (p_prc);
  tiz_buffer_clear (p_prc->p_store_);
  /* Release any buffers held  */
  return release_buffer ((spfysrc_prc_t *)ap_obj);
}

static OMX_ERRORTYPE spfysrc_prc_port_enable (const void *ap_prc, OMX_U32 a_pid)
{
  spfysrc_prc_t *p_prc = (spfysrc_prc_t *)ap_prc;
  assert (NULL != p_prc);
  TIZ_NOTICE (handleOf (p_prc), "Enabling port [%d]; was disabled? [%s]", a_pid,
              p_prc->port_disabled_ ? "YES" : "NO");
  if (p_prc->port_disabled_)
    {
      p_prc->port_disabled_ = false;
      tiz_buffer_clear (p_prc->p_store_);
      start_playback (p_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE spfysrc_prc_config_change (void *ap_obj,
                                                OMX_U32 TIZ_UNUSED (a_pid),
                                                OMX_INDEXTYPE a_config_idx)
{
  spfysrc_prc_t *p_prc = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != p_prc);

  if (OMX_TizoniaIndexConfigPlaylistSkip == a_config_idx
      && p_prc->p_sp_session_)
    {
      TIZ_INIT_OMX_STRUCT (p_prc->playlist_skip_);
      tiz_check_omx_err (
          tiz_api_GetConfig (tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
                             OMX_TizoniaIndexConfigPlaylistSkip, &p_prc->playlist_skip_));
      TIZ_TRACE (handleOf (p_prc),
                 "[OMX_TizoniaIndexConfigPlaylistSkip] : skip.nValue = [%d]",
                 p_prc->playlist_skip_.nValue);

      /* Let's processed a faked end of track event */
      process_spotify_event (p_prc, end_of_track_cback_handler,
                             p_prc->p_sp_session_);
    }
  return rc;
}

/*
 * spfysrc_prc_class
 */

static void *spfysrc_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "spfysrcprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *spfysrc_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *spfysrcprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizprc), "spfysrcprc_class", classOf (tizprc),
       sizeof(spfysrc_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, spfysrc_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return spfysrcprc_class;
}

void *spfysrc_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *spfysrcprc_class = tiz_get_type (ap_hdl, "spfysrcprc_class");
  TIZ_LOG_CLASS (spfysrcprc_class);
  void *spfysrcprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (spfysrcprc_class, "spfysrcprc", tizprc, sizeof(spfysrc_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, spfysrc_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, spfysrc_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, spfysrc_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, spfysrc_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, spfysrc_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, spfysrc_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, spfysrc_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_timer_ready, spfysrc_prc_timer_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, spfysrc_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_pause, spfysrc_prc_pause,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_resume, spfysrc_prc_resume,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_flush, spfysrc_prc_port_flush,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_disable, spfysrc_prc_port_disable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_enable, spfysrc_prc_port_enable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_config_change, spfysrc_prc_config_change,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return spfysrcprc;
}

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
  * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
  */

/**
 * @file   spfysrcprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Spotify client component
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
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

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

#define SPFYSRC_MIN_QUEUE_UNUSED_SPACES 5
#define SPFYSRC_MAX_STRING_SIZE 2 * OMX_MAX_STRINGNAME_SIZE
#define SPFYSRC_MAX_WAIT_TIME_SECONDS 25

#define SPFYSRC_TRACK "spotify:track:7tAanIARL1guE35AUQzFnj"

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
static OMX_ERRORTYPE
spfysrc_prc_deallocate_resources (void * ap_obj);
static sp_bitrate
omx_preferred_bitrate_to_spfy (
  const OMX_TIZONIA_AUDIO_SPOTIFYBITRATETYPE a_bitrate_type, int * ap_bitrate);
static void
end_of_track_handler (OMX_PTR ap_prc, tiz_event_pluggable_t * ap_event);
static OMX_ERRORTYPE
obtain_next_url (spfysrc_prc_t * ap_prc, int a_skip_value);

#define on_spotifyweb_error_ret_omx_oom(expr)                                \
  do                                                                         \
    {                                                                        \
      int spotify_error = 0;                                                 \
      if (0 != (spotify_error = (expr)))                                     \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtizspotify");                                       \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

/* The application key, specific to each project. */
extern const uint8_t g_appkey[];
/* The size of the application key. */
extern const size_t g_appkey_size;

typedef struct spfy_music_delivery_data spfy_music_delivery_data_t;
struct spfy_music_delivery_data
{
  sp_session * p_sess;
  sp_audioformat format;
  void * p_frames;
  int num_frames;
};

typedef struct spfy_login_failure_data spfy_login_failure_data_t;
struct spfy_login_failure_data
{
  sp_session * p_sess;
  sp_error login_error;
};

static char *
concat (const char * s1, const char * s2)
{
  const size_t len1 = strnlen (s1, SPFYSRC_MAX_STRING_SIZE);
  const size_t len2 = strnlen (s2, SPFYSRC_MAX_STRING_SIZE);
  char * result
    = tiz_mem_calloc (1, len1 + len2 + 1);  // +1 for the null-terminator
  assert (result);
  memcpy (result, s1, len1);
  memcpy (result + len1, s2, len2 + 1);  // +1 for the null-terminator
  return result;
}

static char *
get_cache_prefix ()
{
#define TMPDIR "/var/tmp"
  char * p_prefix = NULL;
  char * p_env_str = NULL;

  if ((p_env_str = getenv ("SNAP_USER_COMMON")))
    {
      p_prefix = concat (p_env_str, "/tizonia-");
    }
  else
    {
      p_prefix = concat (TMPDIR, "/tizonia-");
    }
  return p_prefix;
}

static char *
get_cache_location (char * user)
{
  uid_t uid = geteuid ();
  struct passwd * pw = getpwuid (uid);
  if (pw)
    {
      char * p_pw_name_dash = concat (pw->pw_name, "-spotify-");
      char * p_pw_name_dash_user = concat (p_pw_name_dash, user);
      char * p_cache_prefix = get_cache_prefix ();
      char * p_cache_location = concat (p_cache_prefix, p_pw_name_dash_user);
      tiz_mem_free ((void *) p_pw_name_dash);
      tiz_mem_free ((void *) p_pw_name_dash_user);
      tiz_mem_free ((void *) p_cache_prefix);
      return p_cache_location;
    }
  return user;
}

static void
reset_stream_parameters (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);
  tiz_buffer_clear (ap_prc->p_store_);
  ap_prc->initial_cache_bytes_
    = ((ARATELIA_SPOTIFY_SOURCE_DEFAULT_BIT_RATE_KBITS * 1000) / 8)
      * ARATELIA_SPOTIFY_SOURCE_DEFAULT_CACHE_SECONDS;
}

static void
process_spotify_event (spfysrc_prc_t * ap_prc,
                       tiz_event_pluggable_hdlr_f apf_hdlr, void * ap_data)
{
  tiz_event_pluggable_t * p_event = NULL;
  assert (ap_prc);
  assert (apf_hdlr);
  assert (ap_data);

  p_event = tiz_mem_calloc (1, sizeof (tiz_event_pluggable_t));
  if (p_event)
    {
      p_event->p_servant = ap_prc;
      p_event->pf_hdlr = apf_hdlr;
      p_event->p_data = ap_data;
      apf_hdlr (ap_prc, p_event);
    }
}

static void
post_spotify_event (spfysrc_prc_t * ap_prc, tiz_event_pluggable_hdlr_f apf_hdlr,
                    void * ap_data)
{
  tiz_event_pluggable_t * p_event = NULL;
  assert (ap_prc);
  assert (apf_hdlr);
  assert (ap_data);

  p_event = tiz_mem_calloc (1, sizeof (tiz_event_pluggable_t));
  if (p_event)
    {
      p_event->p_servant = ap_prc;
      p_event->pf_hdlr = apf_hdlr;
      p_event->p_data = ap_data;
      tiz_comp_event_pluggable (handleOf (ap_prc), p_event);
    }
}

static OMX_ERRORTYPE
prepare_for_port_auto_detection (spfysrc_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_SPOTIFY_SOURCE_PORT_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));
  ap_prc->audio_coding_type_ = port_def.format.audio.eEncoding;
  ap_prc->auto_detect_on_
    = (OMX_AUDIO_CodingAutoDetect == ap_prc->audio_coding_type_) ? true : false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
set_audio_coding_on_port (spfysrc_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_SPOTIFY_SOURCE_PORT_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));

  /* Set the new value */
  port_def.format.audio.eEncoding = ap_prc->audio_coding_type_;

  tiz_check_omx (tiz_krn_SetParameter_internal (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_IndexParamPortDefinition, &port_def));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
set_pcm_audio_info_on_port (spfysrc_prc_t * ap_prc)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (pcmmode, ARATELIA_SPOTIFY_SOURCE_PORT_INDEX);
  tiz_check_omx (tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)),
                                       handleOf (ap_prc),
                                       OMX_IndexParamAudioPcm, &pcmmode));

  /* Set the new values */
  pcmmode.nChannels = ap_prc->num_channels_;
  pcmmode.nSamplingRate = ap_prc->samplerate_;
  pcmmode.bInterleaved = OMX_TRUE;
  pcmmode.nBitPerSample = 16;

  tiz_check_omx (tiz_krn_SetParameter_internal (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc), OMX_IndexParamAudioPcm,
    &pcmmode));
  return OMX_ErrorNone;
}

static void
send_port_settings_change_event (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);
  TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_EventPortSettingsChanged");
  tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortSettingsChanged,
                       ARATELIA_SPOTIFY_SOURCE_PORT_INDEX, /* port 0 */
                       OMX_IndexParamPortDefinition, /* the index of the */
                                                     /* struct that has */
                                                     /* been modififed  */
                       NULL);
}

static void
send_port_auto_detect_events (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->audio_coding_type_ != OMX_AUDIO_CodingUnused
      && ap_prc->audio_coding_type_ != OMX_AUDIO_CodingAutoDetect)
    {
      TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_EventPortFormatDetected");
      tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortFormatDetected, 0, 0,
                           NULL);
      send_port_settings_change_event (ap_prc);
    }
  else
    {
      /* Oops... could not detect the stream format */
      tiz_srv_issue_err_event ((OMX_PTR) ap_prc, OMX_ErrorFormatNotDetected);
    }
}

static OMX_ERRORTYPE
store_metadata (spfysrc_prc_t * ap_prc, const char * ap_key,
                const char * ap_value)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_CONFIG_METADATAITEMTYPE * p_meta = NULL;
  size_t metadata_len = 0;
  size_t info_len = 0;

  assert (ap_prc);
  assert (ap_key);
  assert (ap_value);

  info_len = strnlen (ap_value, SPFYSRC_MAX_STRING_SIZE - 1) + 1;
  metadata_len = sizeof (OMX_CONFIG_METADATAITEMTYPE) + info_len;

  if (NULL
      == (p_meta
          = (OMX_CONFIG_METADATAITEMTYPE *) tiz_mem_calloc (1, metadata_len)))
    {
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      const size_t name_len = strnlen (ap_key, SPFYSRC_MAX_STRING_SIZE - 1) + 1;
      strncpy ((char *) p_meta->nKey, ap_key, name_len - 1);
      p_meta->nKey[name_len - 1] = '\0';
      p_meta->nKeySizeUsed = name_len;

      strncpy ((char *) p_meta->nValue, ap_value, info_len - 1);
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

/* static void */
/* store_metadata_playlist (spfysrc_prc_t * ap_prc, const char * a_playlist_name, */
/*                          const int a_num_tracks, const int a_num_subscribers) */
/* { */
/*   char playlist_info[SPFYSRC_MAX_STRING_SIZE]; */
/*   assert (a_playlist_name); */
/*   snprintf (playlist_info, SPFYSRC_MAX_STRING_SIZE - 1, */
/*             "%d tracks, %d subscribers", a_num_tracks, a_num_subscribers); */
/*   (void) store_metadata (ap_prc, a_playlist_name, playlist_info); */
/* } */

static void
store_metadata_track_name (spfysrc_prc_t * ap_prc, const char * a_track_name)
{
  char name_str[SPFYSRC_MAX_STRING_SIZE];
  assert (a_track_name);
  snprintf (name_str, SPFYSRC_MAX_STRING_SIZE - 1, "%s  [%s]  (%s)", a_track_name,
            tiz_spotify_get_current_track_uri (ap_prc->p_spfy_web_),
            tiz_spotify_get_current_queue_progress (ap_prc->p_spfy_web_));
  (void) store_metadata (ap_prc, "Track", name_str);
}

static void
store_metadata_artist (spfysrc_prc_t * ap_prc, const char * a_artist_name)
{
  char artist_str[SPFYSRC_MAX_STRING_SIZE];
  assert (a_artist_name);
  snprintf (artist_str, SPFYSRC_MAX_STRING_SIZE - 1, "%s  [%s]", a_artist_name,
            tiz_spotify_get_current_track_artist_uri (ap_prc->p_spfy_web_));
  (void) store_metadata (ap_prc, "Artist", artist_str);
}

static void
store_metadata_album (spfysrc_prc_t * ap_prc, const char * a_album_name)
{
  char album_str[SPFYSRC_MAX_STRING_SIZE];
  assert (a_album_name);
  if (tiz_spotify_get_current_track_album_uri (ap_prc->p_spfy_web_))
    {
      snprintf (album_str, SPFYSRC_MAX_STRING_SIZE - 1, "%s  [%s]", a_album_name,
                tiz_spotify_get_current_track_album_uri (ap_prc->p_spfy_web_));
    }
  else
    {
      snprintf (album_str, SPFYSRC_MAX_STRING_SIZE - 1, "%s", a_album_name);
    }
  (void) store_metadata (ap_prc, "Album", album_str);
}

static void
store_metadata_track_duration (spfysrc_prc_t * ap_prc, const int a_duration_ms)
{
  if (a_duration_ms > 0)
    {
      int duration = a_duration_ms / 1000;
      char duration_str[SPFYSRC_MAX_STRING_SIZE];
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
          snprintf (duration_str, SPFYSRC_MAX_STRING_SIZE - 1, "%2ih:%2im:%2is",
                    hours, minutes, seconds);
        }
      else if (minutes > 0)
        {
          snprintf (duration_str, SPFYSRC_MAX_STRING_SIZE - 1, "%2im:%2is",
                    minutes, seconds);
        }
      else
        {
          snprintf (duration_str, SPFYSRC_MAX_STRING_SIZE - 1, "%2is", seconds);
        }
      (void) store_metadata (ap_prc, "Duration", duration_str);
    }
}

static void
store_metadata_preferred_bitrate (spfysrc_prc_t * ap_prc)
{
  char bitrate_str[SPFYSRC_MAX_STRING_SIZE];
  int bitrate = 0;
  omx_preferred_bitrate_to_spfy (ap_prc->session_.ePreferredBitRate, &bitrate);
  snprintf (bitrate_str, SPFYSRC_MAX_STRING_SIZE - 1, "%d kbps", bitrate);
  (void) store_metadata (ap_prc, "Bit rate", bitrate_str);
}

static void
store_relevant_track_metadata (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);
  (void) tiz_krn_clear_metadata (tiz_get_krn (handleOf (ap_prc)));
  store_metadata_track_name (ap_prc, sp_track_name (ap_prc->p_sp_track_));
  store_metadata_artist (ap_prc, sp_artist_name (sp_track_artist (ap_prc->p_sp_track_, 0)));
  store_metadata_album (ap_prc, sp_album_name (sp_track_album (ap_prc->p_sp_track_)));
  (void) store_metadata (
    ap_prc, "Release Date",
    tiz_spotify_get_current_track_release_date (ap_prc->p_spfy_web_));
  store_metadata_track_duration (ap_prc,
                                 sp_track_duration (ap_prc->p_sp_track_));
  store_metadata_preferred_bitrate (ap_prc);
}

static OMX_ERRORTYPE
retrieve_session_configuration (spfysrc_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioSpotifySession, &(ap_prc->session_));
}

static OMX_ERRORTYPE
retrieve_playlist (spfysrc_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioSpotifyPlaylist, &(ap_prc->playlist_));
}

static sp_connection_type
omx_connection_type_to_spfy (
  const OMX_TIZONIA_AUDIO_SPOTIFYCONNECTIONTYPE a_conn_type)
{
  sp_connection_type spfy_conn
    = (sp_connection_type) OMX_AUDIO_SpotifyConnectionUnknown;
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

static sp_bitrate
omx_preferred_bitrate_to_spfy (
  const OMX_TIZONIA_AUDIO_SPOTIFYBITRATETYPE a_bitrate_type, int * ap_bitrate)
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
    };
  return spfy_bitrate;
}

static OMX_ERRORTYPE
set_spotify_session_options (spfysrc_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  int bitrate = 0;
  bool private_session = true;

  assert (ap_prc);
  assert (ap_prc->p_sp_session_);

  goto_end_on_sp_error (sp_session_set_connection_type (
    ap_prc->p_sp_session_,
    omx_connection_type_to_spfy (ap_prc->session_.eConnectionType)));

  goto_end_on_sp_error (sp_session_preferred_bitrate (
    ap_prc->p_sp_session_, omx_preferred_bitrate_to_spfy (
                             ap_prc->session_.ePreferredBitRate, &bitrate)));

  /* TODO: Expose this setting via the OMX extension */
  goto_end_on_sp_error (
    sp_session_set_private_session (ap_prc->p_sp_session_, private_session));

  ap_prc->min_cache_bytes_
    = ((bitrate * 1000) / 8) * ARATELIA_SPOTIFY_SOURCE_MIN_CACHE_SECONDS;

  ap_prc->max_cache_bytes_
    = ((bitrate * 1000) / 8) * ARATELIA_SPOTIFY_SOURCE_MAX_CACHE_SECONDS;

  /* All OK */
  rc = OMX_ErrorNone;

end:

  return rc;
}

static OMX_ERRORTYPE
allocate_temp_data_store (spfysrc_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);
  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_SPOTIFY_SOURCE_PORT_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));
  assert (ap_prc->p_store_ == NULL);
  return tiz_buffer_init (&(ap_prc->p_store_), port_def.nBufferSize);
}

static inline void
deallocate_temp_data_store (
  /*@special@ */ spfysrc_prc_t * ap_prc)
/*@releases ap_prc->p_store_@ */
/*@ensures isnull ap_prc->p_store_@ */
{
  assert (ap_prc);
  tiz_buffer_destroy (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
}

static inline int
copy_to_omx_buffer (OMX_BUFFERHEADERTYPE * ap_hdr, const void * ap_src,
                    const int nbytes)
{
  int n = MIN (nbytes, ap_hdr->nAllocLen - ap_hdr->nFilledLen);
  assert (n > 0);
  (void) memcpy (ap_hdr->pBuffer + ap_hdr->nOffset, ap_src, n);
  ap_hdr->nFilledLen += n;
  ap_hdr->nOffset += n;
  TIZ_PRINTF_DBG_YEL (
    "HEADER [%p] nbytes [%d] alloc [%d] fill [%d] offset [%d]\n", ap_hdr, n,
    ap_hdr->nAllocLen, ap_hdr->nFilledLen, ap_hdr->nOffset);
  return n;
}

static OMX_ERRORTYPE
release_buffer (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_outhdr_)
    {
      OMX_BUFFERHEADERTYPE * p_hdr = ap_prc->p_outhdr_;
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
      tiz_check_omx (
        tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)), 0, p_hdr));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_BUFFERHEADERTYPE *
buffer_needed (spfysrc_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  assert (ap_prc);

  if (!ap_prc->port_disabled_)
    {
      if (ap_prc->p_outhdr_)
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
              if (ap_prc->p_outhdr_)
                {
                  p_hdr = ap_prc->p_outhdr_;
                }
            }
        }
    }
  return p_hdr;
}

static void
start_spotify (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);
  TIZ_TRACE (handleOf (ap_prc), "start_spotify [%p]", ap_prc->p_sp_session_);
  if (ap_prc->p_sp_session_)
    {
      sp_error error = sp_session_player_play (ap_prc->p_sp_session_, true);
      TIZ_DEBUG (handleOf (ap_prc), "Track error code [%s]",
                 sp_error_message (error));
      ap_prc->spotify_paused_ = false;
    }
}

static void
pause_spotify (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->p_sp_session_)
    {
      sp_session_player_play (ap_prc->p_sp_session_, false);
      ap_prc->spotify_paused_ = true;
    }
}

static void
stop_spotify_session_timer (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->next_timeout_ && ap_prc->p_session_timer_)
    {
      (void) tiz_srv_timer_watcher_stop (ap_prc, ap_prc->p_session_timer_);
      ap_prc->next_timeout_ = 0;
    }
}

static void
stop_spotify (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->p_sp_track_)
    {
      sp_session_player_unload (ap_prc->p_sp_session_);
      ap_prc->p_sp_track_ = NULL;
      stop_spotify_session_timer (ap_prc);
    }
}

/* Decide if spotify music delivery needs pause/re-start */
static void
reevaluate_cache (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_sp_session_ && !ap_prc->initial_cache_bytes_)
    {
      const int current_cache_bytes = tiz_buffer_available (ap_prc->p_store_);
      if (current_cache_bytes > ap_prc->max_cache_bytes_
          && !ap_prc->spotify_paused_)
        {
          /* Temporarily pause spotify music delivery */
          pause_spotify (ap_prc);
        }
      else if (current_cache_bytes <= ap_prc->min_cache_bytes_
               && ap_prc->spotify_paused_)
        {
          /* Re-start spotify music delivery */
          start_spotify (ap_prc);
        }
    }
}

static OMX_ERRORTYPE
consume_cache (spfysrc_prc_t * ap_prc)
{
  assert (ap_prc);

  /* Also, control here the delivery of the next eos flag */
  if (ap_prc->eos_ && ap_prc->bytes_till_eos_ <= 0)
    {
      ap_prc->bytes_till_eos_ = tiz_buffer_available (ap_prc->p_store_);
    }

  TIZ_TRACE (handleOf (ap_prc),
             "store [%d] initial_cache [%d] min_cache [%d] max_cache [%d]",
             tiz_buffer_available (ap_prc->p_store_),
             ap_prc->initial_cache_bytes_, ap_prc->min_cache_bytes_,
             ap_prc->max_cache_bytes_);

  if (tiz_buffer_available (ap_prc->p_store_) > ap_prc->initial_cache_bytes_)
    {
      int nbytes_stored = 0;
      OMX_BUFFERHEADERTYPE * p_out = NULL;

      /* Reset the initial size */
      ap_prc->initial_cache_bytes_ = 0;

      while ((nbytes_stored = tiz_buffer_available (ap_prc->p_store_)) > 0
             && (p_out = buffer_needed (ap_prc)) != NULL)
        {
          int nbytes_copied = copy_to_omx_buffer (
            p_out, tiz_buffer_get (ap_prc->p_store_), nbytes_stored);
          tiz_check_omx (release_buffer (ap_prc));
          (void) tiz_buffer_advance (ap_prc->p_store_, nbytes_copied);
          p_out = NULL;
        }
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
process_spotify_session_events (spfysrc_prc_t * ap_prc)
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

      rc = tiz_srv_timer_watcher_start (ap_prc, ap_prc->p_session_timer_,
                                        ap_prc->next_timeout_ / 1000, 0);
    }
  return rc;
}

/**
 * Called on various events to start playback if it hasn't been started already.
 *
 * The function simply starts playing the first track of the playlist.
 */
static void
start_playback (spfysrc_prc_t * ap_prc)
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

  sp_track * p_track = NULL;

  assert (ap_prc);

  TIZ_TRACE (
    handleOf (ap_prc),
    "ap_prc->track_index_ [%d] ap_prc->transfering_[%s] ap_prc->ntracks_[%d]",
    ap_prc->track_index_, (ap_prc->transfering_ ? "YES" : "NO"),
    ap_prc->ntracks_);

  if (ap_prc->transfering_)
    {
      ap_prc->ntracks_
        = tiz_spotify_get_current_queue_length_as_int (ap_prc->p_spfy_web_);
      verify_or_return ((ap_prc->ntracks_ > 0),
                        "No tracks in playlist. Waiting.");

      /* Retrieve the spotify track from its textual representation */
      ap_prc->p_sp_link_ = sp_link_create_from_string (
        (char *) ap_prc->p_uri_param_->contentURI);
      verify_or_return ((ap_prc->p_sp_link_ != NULL),
                        "No link retrieved. Waiting.");
      p_track = sp_link_as_track (ap_prc->p_sp_link_);

      TIZ_TRACE (handleOf (ap_prc), "ap_prc->p_sp_track_ [%p] p_track [%p]",
                 ap_prc->p_sp_track_, p_track);

      if (ap_prc->p_sp_track_ && p_track != ap_prc->p_sp_track_)
        {
          /* Someone changed the current track */
          TIZ_DEBUG (handleOf (ap_prc), "Someone changed the current track");
          stop_spotify (ap_prc);
        }

      verify_or_return ((p_track != NULL), "Track is NULL. Waiting.");
      TIZ_DEBUG (handleOf (ap_prc), "Track error code [%s]",
                 sp_error_message (sp_track_error (p_track)));
      verify_or_return ((sp_track_error (p_track) == SP_ERROR_OK),
                        "Track error. Waiting.");

      if (ap_prc->p_sp_track_ != p_track)
        {
          if (ap_prc->p_sp_track_)
            {
              sp_track_release (ap_prc->p_sp_track_);
              sp_link_release (ap_prc->p_sp_link_);
            }
          ap_prc->p_sp_track_ = p_track;
          sp_track_availability avail
            = sp_track_get_availability (ap_prc->p_sp_session_, p_track);
          if (SP_TRACK_AVAILABILITY_AVAILABLE == avail)
            {
              store_relevant_track_metadata (ap_prc);
              sp_session_player_load (ap_prc->p_sp_session_, p_track);
              start_spotify (ap_prc);
              ap_prc->spotify_inited_ = true;
            }
          else
            {
              TIZ_PRINTF_RED ("[Spotify] [INFO] '%s' not available\n",
                              sp_track_name (ap_prc->p_sp_track_));

              /* Let's process a fake end of track event */
              process_spotify_event (ap_prc, end_of_track_handler,
                                     ap_prc->p_sp_session_);
            }
        }
    }
}

static void
login_failure_handler (OMX_PTR ap_prc, tiz_event_pluggable_t * ap_event)
{
  spfysrc_prc_t * p_prc = ap_prc;
  assert (p_prc);
  assert (ap_event);

  if (ap_event->p_data)
    {
      spfy_login_failure_data_t * p_data = ap_event->p_data;
      switch (p_data->login_error)
        {
        case SP_ERROR_CLIENT_TOO_OLD:
          {
            TIZ_PRINTF_RED ("[Spotify] [FATAL] Login attempt failed. "
                            "Client too old.");
          }
          break;
        case SP_ERROR_UNABLE_TO_CONTACT_SERVER:
          {
            TIZ_PRINTF_RED ("[Spotify] [FATAL] Login attempt failed. "
                            "Unable to contact server.");
          }
          break;
        case SP_ERROR_BAD_USERNAME_OR_PASSWORD:
          {
            TIZ_PRINTF_RED ("[Spotify] [FATAL] Login attempt failed. "
                            "Bad user name or password.");
          }
          break;
        case SP_ERROR_USER_BANNED:
          {
            TIZ_PRINTF_RED ("[Spotify] [FATAL] Login attempt failed. "
                            "User banned.");
          }
          break;
        case SP_ERROR_USER_NEEDS_PREMIUM:
          {
            TIZ_PRINTF_RED ("[Spotify] [FATAL] Login attempt failed. "
                            "User needs premium.");
          }
          break;
        case SP_ERROR_OTHER_TRANSIENT:
        case SP_ERROR_OTHER_PERMANENT:
        default:
          {
            TIZ_PRINTF_RED ("[Spotify] [FATAL] Login attempt failed.");
          }
          break;
        };
      tiz_mem_free (ap_event->p_data);
    }
  (void) tiz_srv_issue_err_event ((OMX_PTR) ap_prc,
                                  OMX_ErrorInsufficientResources);
  tiz_mem_free (ap_event);
}

/**
 * This callback is called when an attempt to login has succeeded or failed.
 *
 */
static void
logged_in (sp_session * sess, sp_error error)
{
  spfysrc_prc_t * p_prc = sp_session_userdata (sess);
  assert (p_prc);

  if (SP_ERROR_OK == error)
    {
      sp_error sp_rc = SP_ERROR_OK;
      TIZ_PRINTF_BLU ("[Spotify] [Login] '%s' logged in.\n",
                      sp_user_display_name (sp_session_user (sess)));

      set_spotify_session_options (p_prc);
      start_playback (p_prc);
      (void) sp_rc;
    }
  else if (SP_ERROR_BAD_USERNAME_OR_PASSWORD == error)
    {
      spfy_login_failure_data_t * p_data
        = tiz_mem_calloc (1, sizeof (spfy_login_failure_data_t));
      if (p_data)
        {
          p_data->p_sess = sess;
          p_data->login_error = error;
          post_spotify_event (sp_session_userdata (sess), login_failure_handler, p_data);
        }
    }
}

static void
notify_main_thread_handler (OMX_PTR ap_prc, tiz_event_pluggable_t * ap_event)
{
  spfysrc_prc_t * p_prc = ap_prc;
  assert (p_prc);
  assert (ap_event);
  tiz_mem_free (ap_event);
  p_prc->keep_processing_sp_events_ = true;
  if (!p_prc->stopping_)
    {
      (void) process_spotify_session_events (p_prc);
    }
}

/**
 * This callback is called from an internal libspotify thread to ask us to
 * reiterate the main loop.
 *
 * @note This function is called from an internal session thread!
 */
static void
notify_main_thread (sp_session * sess)
{

  if (tiz_comp_event_queue_unused_spaces (handleOf (sp_session_userdata (sess)))
      >= SPFYSRC_MIN_QUEUE_UNUSED_SPACES)
    {
      post_spotify_event (sp_session_userdata (sess),
                          notify_main_thread_handler, sess);
    }
}

/**
 * Callback called when libspotify has new metadata available
 *
 */
static void
metadata_updated (sp_session * sess)
{
  spfysrc_prc_t * p_prc = sp_session_userdata (sess);
  assert (p_prc);
  TIZ_DEBUG (handleOf (p_prc), "metadata updated");
  if (p_prc->p_sp_track_)
    {
      if (sp_track_error (p_prc->p_sp_track_) == SP_ERROR_OK)
        {
          TIZ_TRACE (handleOf (p_prc), "Track = [%s]",
                     sp_track_name (p_prc->p_sp_track_));
        }
    }
  start_playback (p_prc);
}

static void
music_delivery_handler (OMX_PTR ap_prc, tiz_event_pluggable_t * ap_event)
{
  spfysrc_prc_t * p_prc = ap_prc;

  assert (p_prc);
  assert (ap_event);

  TIZ_TRACE (handleOf (ap_prc), "p_data [%p] ", ap_event->p_data);

  /* Decide if spotify music delivery needs pause/re-start */
  reevaluate_cache (ap_prc);

  if (!p_prc->stopping_ && ap_event->p_data)
    {
      spfy_music_delivery_data_t * p_data = ap_event->p_data;
      int nbytes
        = p_data->num_frames * sizeof (int16_t) * p_data->format.channels;
      OMX_U8 * p_in = (OMX_U8 *) p_data->p_frames;
      OMX_BUFFERHEADERTYPE * p_out = NULL;

      TIZ_TRACE (handleOf (ap_prc),
                 "nbytes [%d] spotify_paused_ [%s] store [%d]", nbytes,
                 p_prc->spotify_paused_ ? "YES" : "NO",
                 tiz_buffer_available (p_prc->p_store_));

      consume_cache (p_prc);
      if (!p_prc->spotify_paused_)
        {
          while (nbytes > 0 && (p_out = buffer_needed (p_prc)) != NULL
                 && !p_prc->initial_cache_bytes_)
            {
              int nbytes_copied = copy_to_omx_buffer (p_out, p_in, nbytes);
              (void) release_buffer (p_prc);
              nbytes -= nbytes_copied;
              p_in += nbytes_copied;
            }
        }

      if (nbytes > 0)
        {
          int nbytes_stored = 0;
          if ((nbytes_stored = tiz_buffer_push (p_prc->p_store_, p_in, nbytes))
              < nbytes)
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
      tiz_mem_free (p_data->p_frames);
      tiz_mem_free (ap_event->p_data);
    }
  tiz_mem_free (ap_event);
}

/**
 * This callback is used from libspotify whenever there is PCM data available.
 *
 * @note This function is called from an internal session thread!
 */
static int
music_delivery (sp_session * sess, const sp_audioformat * format,
                const void * frames, int num_frames)
{
  int num_frames_delivered = 0;
  if (num_frames > 0)
    {
      /* We'll only send another pluggable event into the component's main
         event queue if there are a few spaces available. Otherwise, we'll ask
         Spotify to wait a little. */
      if (tiz_comp_event_queue_unused_spaces (
            handleOf (sp_session_userdata (sess)))
          >= SPFYSRC_MIN_QUEUE_UNUSED_SPACES)
        {
          spfy_music_delivery_data_t * p_data
            = tiz_mem_calloc (1, sizeof (spfy_music_delivery_data_t));
          if (p_data)
            {
              size_t pcm_buffer_len
                = num_frames * sizeof (int16_t) * format->channels;
              assert (sess);
              assert (format);
              assert (frames);
              p_data->p_sess = sess;
              p_data->format.sample_type = format->sample_type;
              p_data->format.sample_rate = format->sample_rate;
              p_data->format.channels = format->channels;
              p_data->p_frames = tiz_mem_alloc (pcm_buffer_len);
              memcpy (p_data->p_frames, frames, pcm_buffer_len);
              p_data->num_frames = num_frames;
              TIZ_PRINTF_DBG_YEL (
                "music_delivery - num frames : %d pcm_buffer_len : %d - "
                "spaces %d\n",
                num_frames, pcm_buffer_len,
                tiz_comp_event_queue_unused_spaces (
                  handleOf (sp_session_userdata (sess))));
              post_spotify_event (sp_session_userdata (sess),
                                  music_delivery_handler, p_data);
              num_frames_delivered = num_frames;
            }
        }
    }
  return num_frames_delivered;
}

static void
end_of_track_handler (OMX_PTR ap_prc, tiz_event_pluggable_t * ap_event)
{
  spfysrc_prc_t * p_prc = ap_prc;
  assert (p_prc);
  assert (ap_event);

  if (!p_prc->stopping_ && ap_event->p_data)
    {
      p_prc->eos_ = true;

      if (p_prc->p_sp_track_)
        {
          TIZ_TRACE (handleOf (ap_prc), "end of track = [%s]",
                     sp_track_name (p_prc->p_sp_track_));
          sp_session_player_unload (p_prc->p_sp_session_);
          p_prc->p_sp_track_ = NULL;
        }

      p_prc->playlist_skip_.nValue > 0 ? obtain_next_url (p_prc, 1)
        : obtain_next_url (p_prc, -1);
      p_prc->playlist_skip_.nValue = 1;

      start_playback (p_prc);
      (void) process_spotify_session_events (p_prc);
    }
  tiz_mem_free (ap_event);
}

/**
 * This callback is used from libspotify when the current track has ended
 *
 * @note This function is called from an internal session thread!
 */
static void
end_of_track (sp_session * sess)
{
  TIZ_PRINTF_DBG_YEL ("end_of_track\n");
  post_spotify_event (sp_session_userdata (sess), end_of_track_handler, sess);
}

static void
play_token_lost_handler (OMX_PTR ap_prc, tiz_event_pluggable_t * ap_event)
{
  spfysrc_prc_t * p_prc = ap_prc;
  assert (p_prc);
  assert (ap_event);

  if (p_prc)
    {
      if (p_prc->session_.bRecoverLostToken)
        {
          /* The user wants to continue listening to music on this device */
          process_spotify_event (p_prc, end_of_track_handler,
                                 p_prc->p_sp_session_);
        }
      else
        {
          stop_spotify (p_prc);
          TIZ_PRINTF_RED ("\n[Spotify] [FATAL] The play token has been lost\n");
          TIZ_PRINTF_YEL ("To force recovery of the token when it gets lost, add the\n");
          TIZ_PRINTF_YEL ("'--spotify-recover-lost-token' command-line flag\n");
          TIZ_PRINTF_YEL ("or add 'spotify.recover_lost_token = true' in 'tizonia.conf'\n");
          (void) tiz_srv_issue_err_event ((OMX_PTR) ap_prc,
                                          OMX_ErrorInsufficientResources);
        }
    }
  tiz_mem_free (ap_event);
}

/**
 * Notification that some other connection has started playing on this account.
 * Playback has been stopped.
 *
 */
static void
play_token_lost (sp_session * sess)
{
  post_spotify_event (sp_session_userdata (sess), play_token_lost_handler, sess);
}

static void
log_message (sp_session * sess, const char * msg)
{
  if (strstr (msg, "Request for file") || strstr (msg, "locked")
      || strstr (msg, "ChannelError") || strstr (msg, "handleApErrorCode"))
    {
      TIZ_PRINTF_DBG_RED ("[Spotify] : %s", msg);
      return;
    }
  TIZ_PRINTF_DBG_MAG ("[Spotify] : %s", msg);
}

static OMX_ERRORTYPE
enqueue_playlist_items (spfysrc_prc_t * ap_prc)
{
  int rc = 1;
  assert (ap_prc);
  assert (ap_prc->p_spfy_web_);

  {
    const char * p_playlist = (const char *) ap_prc->playlist_.cPlaylistName;
    const char * p_owner = (const char *) ap_prc->playlist_.cPlaylistOwner;
    const OMX_BOOL shuffle = ap_prc->playlist_.bShuffle;

    tiz_spotify_set_playback_mode (
      ap_prc->p_spfy_web_,
      (shuffle == OMX_TRUE ? ETIZSpotifyPlaybackModeShuffle
                           : ETIZSpotifyPlaybackModeNormal));

    switch (ap_prc->playlist_.ePlaylistType)
      {
        case OMX_AUDIO_SpotifyPlaylistTypeUnknown:
          {
            /* TODO */
            assert (0);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeTracks:
          {
            rc = tiz_spotify_play_tracks (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeArtist:
          {
            rc = tiz_spotify_play_artist (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeAlbum:
          {
            rc = tiz_spotify_play_album (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypePlaylist:
          {
            rc = tiz_spotify_play_playlist (ap_prc->p_spfy_web_, p_playlist,
                                            p_owner);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeTrackId:
          {
            rc = tiz_spotify_play_track_by_id (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeArtistId:
          {
            rc = tiz_spotify_play_artist_by_id (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeAlbumId:
          {
            rc = tiz_spotify_play_album_by_id (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypePlaylistId:
          {
            rc = tiz_spotify_play_playlist_by_id (ap_prc->p_spfy_web_,
                                                  p_playlist, p_owner);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeRelatedArtists:
          {
            rc = tiz_spotify_play_related_artists (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeFeaturedPlaylist:
          {
            rc = tiz_spotify_play_featured_playlist (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeNewReleases:
          {
            rc = tiz_spotify_play_new_releases (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByTrackId:
          {
            rc = tiz_spotify_play_recommendations_by_track_id (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByArtistId:
          {
            rc = tiz_spotify_play_recommendations_by_artist_id (ap_prc->p_spfy_web_, p_playlist);
          }
          break;
        case OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByGenre:
          {
            rc = tiz_spotify_play_recommendations_by_genre (ap_prc->p_spfy_web_, p_playlist);
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

static OMX_ERRORTYPE
obtain_next_url (spfysrc_prc_t * ap_prc, int a_skip_value)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);
  assert (ap_prc->p_spfy_web_);

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
    const bool need_url_removed = false;
    const char * p_next_url
      = a_skip_value > 0
          ? tiz_spotify_get_next_uri (ap_prc->p_spfy_web_, need_url_removed)
          : tiz_spotify_get_prev_uri (ap_prc->p_spfy_web_, need_url_removed);
    tiz_check_null_ret_oom (p_next_url);

    {
      const OMX_U32 url_len = strnlen (p_next_url, pathname_max);
      TIZ_TRACE (handleOf (ap_prc), "URL [%s]", p_next_url);

      /* Verify we are getting an http scheme */
      if (!p_next_url || !url_len
          || (strncasecmp (p_next_url, "spotify", 7) != 0))
        {
          rc = OMX_ErrorContentURIError;
        }
      else
        {
          strncpy ((char *) ap_prc->p_uri_param_->contentURI, p_next_url,
                   url_len);
          ap_prc->p_uri_param_->contentURI[url_len] = '\0';

          /* Song metadata is now available, update the IL client */
          /*           rc = update_metadata (ap_prc); */
        }
    }
  }

  return rc;
}

/*
 * spfysrcprc
 */

static void *
spfysrc_prc_ctor (void * ap_obj, va_list * app)
{
  spfysrc_prc_t * p_prc
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
  p_prc->p_session_timer_ = NULL;
  p_prc->p_shuffle_lst_ = NULL;
  TIZ_INIT_OMX_STRUCT (p_prc->session_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_);
  p_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
  p_prc->num_channels_ = 2;
  p_prc->samplerate_ = 44100;
  p_prc->auto_detect_on_ = false;

  p_prc->ntracks_ = 0;
  p_prc->p_sp_session_ = NULL;

  /* Init the spotify config struct */
  tiz_mem_set ((OMX_PTR) &p_prc->sp_config_, 0, sizeof (p_prc->sp_config_));
  p_prc->sp_config_.api_version = SPOTIFY_API_VERSION;
  p_prc->sp_config_.cache_location = NULL;
  p_prc->sp_config_.settings_location = NULL;
  p_prc->sp_config_.application_key = g_appkey;
  p_prc->sp_config_.application_key_size = g_appkey_size;
  p_prc->sp_config_.user_agent = "tizonia-source-component";
  p_prc->sp_config_.callbacks = &(p_prc->sp_cbacks_);
  p_prc->sp_config_.userdata = p_prc;
  p_prc->sp_config_.compress_playlists = false;
  p_prc->sp_config_.dont_save_metadata_for_playlists = true;
  p_prc->sp_config_.initially_unload_playlists = false;

  /* Init the spotify callbacks struct */
  tiz_mem_set ((OMX_PTR) &p_prc->sp_cbacks_, 0, sizeof (p_prc->sp_cbacks_));
  p_prc->sp_cbacks_.logged_in = &logged_in;
  p_prc->sp_cbacks_.notify_main_thread = &notify_main_thread;
  p_prc->sp_cbacks_.music_delivery = &music_delivery;
  p_prc->sp_cbacks_.metadata_updated = &metadata_updated;
  p_prc->sp_cbacks_.play_token_lost = &play_token_lost;
  p_prc->sp_cbacks_.log_message = &log_message;
  p_prc->sp_cbacks_.end_of_track = &end_of_track;

  p_prc->p_sp_track_ = NULL;
  p_prc->p_sp_link_ = NULL;
  p_prc->p_spfy_web_ = NULL;
  p_prc->keep_processing_sp_events_ = false;
  p_prc->next_timeout_ = 0;

  return p_prc;
}

static void *
spfysrc_prc_dtor (void * ap_obj)
{
  (void) spfysrc_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "spfysrcprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
spfysrc_prc_allocate_resources (void * ap_prc, OMX_U32 a_pid)
{
  spfysrc_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;

  assert (p_prc);
  assert (NULL == p_prc->p_session_timer_);
  assert (NULL == p_prc->p_uri_param_);
  assert (NULL == p_prc->p_shuffle_lst_);

  tiz_check_omx (allocate_temp_data_store (p_prc));
  tiz_check_omx (retrieve_session_configuration (p_prc));
  tiz_check_omx (retrieve_playlist (p_prc));
  tiz_check_omx (
    tiz_srv_timer_watcher_init (p_prc, &(p_prc->p_session_timer_)));

  /* Instantiate the spotify web api proxy */
  on_spotifyweb_error_ret_omx_oom (tiz_spotify_init (&(p_prc->p_spfy_web_)));

  tiz_check_omx (enqueue_playlist_items (p_prc));
  tiz_check_omx (obtain_next_url (p_prc, 1));

  /* Create a spotify session */
  p_prc->sp_config_.cache_location
    = get_cache_location ((char *) p_prc->session_.cUserName);
  p_prc->sp_config_.settings_location = p_prc->sp_config_.cache_location;
  goto_end_on_sp_error (
    sp_session_create (&(p_prc->sp_config_), &(p_prc->p_sp_session_)));

  TIZ_PRINTF_BLU ("[Spotify] [Cache]: '%s'\n",
                  p_prc->sp_config_.cache_location);

  /* Initiate the spotify session */
  goto_end_on_sp_error (sp_session_login (
    p_prc->p_sp_session_, (const char *) p_prc->session_.cUserName,
    (const char *) p_prc->session_.cUserPassword, 0, /* If true, the username /
               password will be remembered
               by libspotify */
    NULL));

  /* All OK */
  rc = OMX_ErrorNone;

end:

  return rc;
}

static OMX_ERRORTYPE
spfysrc_prc_deallocate_resources (void * ap_prc)
{
  spfysrc_prc_t * p_prc = ap_prc;
  assert (p_prc);

  if (p_prc->p_spfy_web_)
    {
      tiz_spotify_destroy (p_prc->p_spfy_web_);
      p_prc->p_spfy_web_ = NULL;
    }

  if (p_prc->p_session_timer_)
    {
      stop_spotify_session_timer (p_prc);
      tiz_srv_timer_watcher_destroy (p_prc, p_prc->p_session_timer_);
      p_prc->p_session_timer_ = NULL;
    }

  (void) sp_session_release (p_prc->p_sp_session_);
  p_prc->p_sp_session_ = NULL;
  tiz_mem_free ((void *) p_prc->sp_config_.cache_location);
  p_prc->sp_config_.cache_location = p_prc->sp_config_.settings_location = NULL;
  p_prc->spotify_inited_ = false;

  deallocate_temp_data_store (p_prc);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
spfysrc_prc_prepare_to_transfer (void * ap_obj, OMX_U32 a_pid)
{
  reset_stream_parameters (ap_obj);
  return prepare_for_port_auto_detection (ap_obj);
}

static OMX_ERRORTYPE
spfysrc_prc_transfer_and_process (void * ap_obj, OMX_U32 a_pid)
{
  spfysrc_prc_t * p_prc = ap_obj;
  assert (p_prc);
  p_prc->transfering_ = true;
  start_playback (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
spfysrc_prc_stop_and_return (void * ap_obj)
{
  spfysrc_prc_t * p_prc = ap_obj;
  assert (p_prc);
  p_prc->transfering_ = false;
  p_prc->stopping_ = true;
  stop_spotify (p_prc);
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
spfysrc_prc_buffers_ready (const void * ap_obj)
{
  spfysrc_prc_t * p_prc = (spfysrc_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (!p_prc->spotify_inited_)
    {
      start_playback (p_prc);
    }
  if (p_prc->transfering_ && p_prc->spotify_paused_)
    {
      rc = consume_cache (p_prc);
      /* Decide if spotify music delivery needs pause/re-start */
      reevaluate_cache (p_prc);
    }
  else
    {
      rc = process_spotify_session_events (p_prc);
    }
  return rc;
}

static OMX_ERRORTYPE
spfysrc_prc_timer_ready (void * ap_prc, tiz_event_timer_t * ap_ev_timer,
                         void * ap_arg, const uint32_t a_id)
{
  spfysrc_prc_t * p_prc = (spfysrc_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  p_prc->next_timeout_ = 0;
  if (ap_ev_timer == p_prc->p_session_timer_ && !p_prc->stopping_)
    {
      rc = process_spotify_session_events (p_prc);
    }

  return rc;
}

static OMX_ERRORTYPE
spfysrc_prc_pause (const void * ap_prc)
{
  spfysrc_prc_t * p_prc = (spfysrc_prc_t *) ap_prc;
  assert (p_prc);
  if (p_prc->transfering_ && !p_prc->spotify_paused_)
    {
      /* pause spotify music delivery */
      pause_spotify (p_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
spfysrc_prc_resume (const void * ap_prc)
{
  spfysrc_prc_t * p_prc = (spfysrc_prc_t *) ap_prc;
  assert (p_prc);
  if (p_prc->transfering_ && p_prc->spotify_paused_)
    {
      /* Re-start spotify music delivery */
      start_spotify (p_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
spfysrc_prc_port_flush (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  spfysrc_prc_t * p_prc = (spfysrc_prc_t *) ap_obj;
  assert (p_prc);
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
spfysrc_prc_port_disable (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  spfysrc_prc_t * p_prc = (spfysrc_prc_t *) ap_obj;
  assert (p_prc);
  p_prc->port_disabled_ = true;
  stop_spotify_session_timer (p_prc);
  /* Release any buffers held  */
  return release_buffer ((spfysrc_prc_t *) ap_obj);
}

static OMX_ERRORTYPE
spfysrc_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  spfysrc_prc_t * p_prc = (spfysrc_prc_t *) ap_prc;
  assert (p_prc);
  if (p_prc->port_disabled_)
    {
      p_prc->port_disabled_ = false;
      reset_stream_parameters (p_prc);
      start_playback (p_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
spfysrc_prc_config_change (void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid),
                           OMX_INDEXTYPE a_config_idx)
{
  spfysrc_prc_t * p_prc = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_prc);

  if (OMX_TizoniaIndexConfigPlaylistSkip == a_config_idx
      && p_prc->p_sp_session_)
    {
      TIZ_INIT_OMX_STRUCT (p_prc->playlist_skip_);
      tiz_check_omx (tiz_api_GetConfig (
        tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
        OMX_TizoniaIndexConfigPlaylistSkip, &p_prc->playlist_skip_));

      if (p_prc->spotify_paused_)
        {
          /* Re-start spotify music delivery */
          start_spotify (p_prc);
        }

      /* Let's process a fake end of track event */
      process_spotify_event (p_prc, end_of_track_handler, p_prc->p_sp_session_);
    }
  return rc;
}

/*
 * spfysrc_prc_class
 */

static void *
spfysrc_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "spfysrcprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
spfysrc_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * spfysrcprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "spfysrcprc_class", classOf (tizprc),
     sizeof (spfysrc_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, spfysrc_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return spfysrcprc_class;
}

void *
spfysrc_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * spfysrcprc_class = tiz_get_type (ap_hdl, "spfysrcprc_class");
  TIZ_LOG_CLASS (spfysrcprc_class);
  void * spfysrcprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (spfysrcprc_class, "spfysrcprc", tizprc, sizeof (spfysrc_prc_t),
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

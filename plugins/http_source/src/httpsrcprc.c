/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   httpsrcprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - HTTP streaming client processor
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

#include "httpsrc.h"
#include "httpsrcprc.h"
#include "httpsrcprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_source.prc"
#endif

/* forward declarations */
static OMX_ERRORTYPE httpsrc_prc_deallocate_resources (void *);
static OMX_BUFFERHEADERTYPE *buffer_needed (httpsrc_prc_t *);
static OMX_ERRORTYPE release_buffer (httpsrc_prc_t *);
static OMX_ERRORTYPE prepare_for_port_auto_detection (httpsrc_prc_t *ap_prc);

/* These macros assume the existence of an "ap_prc" local variable */
#define bail_on_curl_error(expr)                                             \
  do                                                                         \
    {                                                                        \
      CURLcode curl_error = CURLE_OK;                                        \
      if (CURLE_OK != (curl_error = (expr)))                                 \
        {                                                                    \
          TIZ_ERROR (handleOf (ap_prc),                                      \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "curl (%s)",                                            \
                     curl_easy_strerror (curl_error));                       \
          goto end;                                                          \
        }                                                                    \
    }                                                                        \
  while (0)

#define bail_on_curl_multi_error(expr)                                       \
  do                                                                         \
    {                                                                        \
      CURLMcode curl_error = CURLM_OK;                                       \
      if (CURLM_OK != (curl_error = (expr)))                                 \
        {                                                                    \
          TIZ_ERROR (handleOf (ap_prc),                                      \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "curl multi (%s)",                                      \
                     curl_multi_strerror (curl_error));                      \
          goto end;                                                          \
        }                                                                    \
    }                                                                        \
  while (0)

#define bail_on_oom(expr)                                                    \
  do                                                                         \
    {                                                                        \
      if (NULL == (expr))                                                    \
        {                                                                    \
          TIZ_ERROR (handleOf (ap_prc), "[OMX_ErrorInsufficientResources]"); \
          goto end;                                                          \
        }                                                                    \
    }                                                                        \
  while (0)

#define on_curl_error_ret_omx_oom(expr)                                      \
  do                                                                         \
    {                                                                        \
      CURLcode curl_error = CURLE_OK;                                        \
      if (CURLE_OK != (curl_error = (expr)))                                 \
        {                                                                    \
          TIZ_ERROR (handleOf (ap_prc),                                      \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "curl easy (%s)",                                       \
                     curl_easy_strerror (curl_error));                       \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

#define on_curl_multi_error_ret_omx_oom(expr)                                \
  do                                                                         \
    {                                                                        \
      CURLMcode curl_error = CURLM_OK;                                       \
      if (CURLM_OK != (curl_error = (expr)))                                 \
        {                                                                    \
          TIZ_ERROR (handleOf (ap_prc),                                      \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "curl multi (%s)",                                      \
                     curl_multi_strerror (curl_error));                      \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

static inline int copy_to_omx_buffer (OMX_BUFFERHEADERTYPE *ap_hdr,
                                      void *ap_src, const int nbytes)
{
  int n = MIN (nbytes, ap_hdr->nAllocLen - ap_hdr->nFilledLen);
  (void)memcpy (ap_hdr->pBuffer + ap_hdr->nOffset, ap_src, n);
  ap_hdr->nFilledLen += n;
  return n;
}

static OMX_ERRORTYPE allocate_temp_data_store (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (ap_prc->p_store_ == NULL);
  tiz_check_omx_err (tiz_buffer_init (&(ap_prc->p_store_),
                                      ARATELIA_HTTP_SOURCE_PORT_MIN_BUF_SIZE));
  return OMX_ErrorNone;
}

static inline void deallocate_temp_data_store (
    /*@special@ */ httpsrc_prc_t *ap_prc)
/*@releases ap_prc->p_store_@ */
/*@ensures isnull ap_prc->p_store_@ */
{
  assert (NULL != ap_prc);
  tiz_buffer_destroy (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
}

static inline OMX_ERRORTYPE start_io_watcher (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_io_);
  ap_prc->awaiting_io_ev_ = true;
  TIZ_DEBUG (handleOf (ap_prc), "awaiting_io_ev [%s]", "TRUE");
  return tiz_event_io_start (ap_prc->p_ev_io_);
}

static inline OMX_ERRORTYPE stop_io_watcher (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_prc);
  ap_prc->awaiting_io_ev_ = false;
  TIZ_DEBUG (handleOf (ap_prc), "awaiting_io_ev [%s]", "FALSE");
  if (NULL != ap_prc->p_ev_io_)
    {
      rc = tiz_event_io_stop (ap_prc->p_ev_io_);
    }
  return rc;
}

static inline OMX_ERRORTYPE start_curl_timer_watcher (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_curl_timer_);
  if (!ap_prc->awaiting_curl_timer_ev_)
    {
      ap_prc->awaiting_curl_timer_ev_ = true;
      TIZ_DEBUG (handleOf (ap_prc), "awaiting_curl_timer_ev [%p] [TRUE]",
                 ap_prc->p_ev_curl_timer_);
      tiz_event_timer_set (ap_prc->p_ev_curl_timer_, ap_prc->curl_timeout_, 0.);
      rc = tiz_event_timer_start (ap_prc->p_ev_curl_timer_);
    }
  return rc;
}

static inline OMX_ERRORTYPE restart_curl_timer_watcher (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_curl_timer_);
  ap_prc->awaiting_curl_timer_ev_ = true;
  TIZ_DEBUG (handleOf (ap_prc), "awaiting_curl_timer_ev [TRUE]");
  return tiz_event_timer_restart (ap_prc->p_ev_curl_timer_);
}

static inline OMX_ERRORTYPE stop_curl_timer_watcher (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_prc);
  ap_prc->awaiting_curl_timer_ev_ = false;
  TIZ_DEBUG (handleOf (ap_prc), "awaiting_curl_timer_ev [FALSE]");
  if (NULL != ap_prc->p_ev_curl_timer_)
    {
      rc = tiz_event_timer_stop (ap_prc->p_ev_curl_timer_);
    }
  return rc;
}

static inline OMX_ERRORTYPE start_reconnect_timer_watcher (
    httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_reconnect_timer_);
  if (!ap_prc->awaiting_reconnect_timer_ev_)
    {
      ap_prc->awaiting_reconnect_timer_ev_ = true;
      TIZ_DEBUG (handleOf (ap_prc), "awaiting_reconnect_timer_ev_ [%p] [TRUE]",
                 ap_prc->p_ev_reconnect_timer_);
      tiz_event_timer_set (ap_prc->p_ev_reconnect_timer_,
                           ap_prc->reconnect_timeout_,
                           ap_prc->reconnect_timeout_);
      rc = tiz_event_timer_start (ap_prc->p_ev_reconnect_timer_);
    }
  return rc;
}

static inline OMX_ERRORTYPE restart_reconnect_timer_watcher (
    httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_ev_reconnect_timer_);
  ap_prc->awaiting_reconnect_timer_ev_ = true;
  TIZ_DEBUG (handleOf (ap_prc), "awaiting_reconnect_timer_ev [TRUE]");
  return tiz_event_timer_restart (ap_prc->p_ev_reconnect_timer_);
}

static inline OMX_ERRORTYPE stop_reconnect_timer_watcher (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_prc);
  ap_prc->awaiting_reconnect_timer_ev_ = false;
  TIZ_DEBUG (handleOf (ap_prc), "awaiting_reconnect_timer_ev [FALSE]");
  if (NULL != ap_prc->p_ev_reconnect_timer_)
    {
      rc = tiz_event_timer_stop (ap_prc->p_ev_reconnect_timer_);
    }
  return rc;
}

static OMX_ERRORTYPE resume_curl (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);

  TIZ_NOTICE (handleOf (ap_prc), "Resuming curl. Was paused [%s]",
              ECurlStatePaused == ap_prc->curl_state_ ? "YES" : "NO");

  if (ECurlStatePaused == ap_prc->curl_state_)
    {
      int running_handles = 0;
      ap_prc->curl_state_ = ECurlStateTransfering;
      on_curl_error_ret_omx_oom (
          curl_easy_pause (ap_prc->p_curl_, CURLPAUSE_CONT));
      if (ap_prc->curl_version_ < 0x072000)
        {
          /* USAGE WITH THE MULTI-SOCKET INTERFACE */
          /* Before libcurl 7.32.0, when a specific handle was unpaused with
             this function, there was no particular forced rechecking or
             similar of the socket's state, which made the continuation of the
             transfer get delayed until next multi-socket call invoke or even
             longer. Alternatively, the user could forcibly call for example
             curl_multi_socket_all(3) - with a rather hefty performance
             penalty. */
          /* Starting in libcurl 7.32.0, unpausing a transfer will schedule a
             timeout trigger for that handle 1 millisecond into the future, so
             that a curl_multi_socket_action( ... CURL_SOCKET_TIMEOUT) can be
             used immediately afterwards to get the transfer going again as
             desired.  */
          on_curl_multi_error_ret_omx_oom (
              curl_multi_socket_all (ap_prc->p_curl_multi_, &running_handles));
        }
      on_curl_multi_error_ret_omx_oom (curl_multi_socket_action (
          ap_prc->p_curl_multi_, CURL_SOCKET_TIMEOUT, 0, &running_handles));
    }
  return OMX_ErrorNone;
}

static inline bool is_valid_character (const char c)
{
  return (unsigned char)c > 0x20;
}

static void obtain_coding_type (httpsrc_prc_t *ap_prc, char *ap_info)
{
  assert (NULL != ap_prc);
  assert (NULL != ap_info);

  TIZ_TRACE (handleOf (ap_prc), "encoding type  : [%s]", ap_info);

  if (memcmp (ap_info, "audio/mpeg", 10) == 0
      || memcmp (ap_info, "audio/mpg", 9) == 0
      || memcmp (ap_info, "audio/mp3", 9) == 0)
    {
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingMP3;
    }
  else if (memcmp (ap_info, "audio/aac", 9) == 0
           || memcmp (ap_info, "audio/aacp", 10) == 0)
    {
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingAAC;
    }
  else if (memcmp (ap_info, "audio/vorbis", 12) == 0)
    {
      /* This is vorbis without container */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingVORBIS;
    }
  else if (memcmp (ap_info, "audio/speex", 11) == 0)
    {
      /* This is speex without container */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingSPEEX;
    }
  else if (memcmp (ap_info, "audio/flac", 10) == 0)
    {
      /* This is flac without container */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingFLAC;
    }
  else if (memcmp (ap_info, "audio/opus", 10) == 0)
    {
      /* This is opus without container */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingOPUS;
    }
  else if (memcmp (ap_info, "application/ogg", 15) == 0
           || memcmp (ap_info, "audio/ogg", 9) == 0)
    {
      /* This is for audio with ogg container (may be OPUS, Vorbis, Opus,
         etc) */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingOGA;
    }
  else
    {
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
    }
}

static int convert_str_to_int (httpsrc_prc_t *ap_prc, const char *ap_start,
                               char **ap_end)
{
  long val = -1;
  assert (NULL != ap_prc);
  assert (NULL != ap_start);
  assert (NULL != ap_end);

  errno = 0;
  val = strtol (ap_start, ap_end, 0);

  if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
      || (errno != 0 && val == 0))
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "Error retrieving the number of channels : [%s]",
                 strerror (errno));
    }
  else if (*ap_end == ap_start)
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "Error retrieving the number of channels : "
                 "[No digits were found]");
    }

  TIZ_TRACE (handleOf (ap_prc), "Value : [%d]", val);
  return val;
}

static void obtain_audio_info (httpsrc_prc_t *ap_prc, char *ap_info)
{
  const char *channels = "channels";
  const char *samplerate = "samplerate";
  const char *p_start = NULL;
  char *p_end = NULL;
  const char *p_value = NULL;
  assert (NULL != ap_prc);
  assert (NULL != ap_info);

  TIZ_TRACE (handleOf (ap_prc), "audio info  : [%s]", ap_info);

  /* Find the number of channels */
  if (NULL != (p_value = (const char *)strstr (ap_info, channels)))
    {
      if (NULL != (p_start = (const char *)strchr (p_value, '=')))
        {
          /* skip the equal sign */
          p_start++;
          ap_prc->num_channels_ = convert_str_to_int (ap_prc, p_start, &p_end);
        }
    }

  /* Find the sampling rate */
  if (NULL != (p_value = (const char *)strstr (ap_info, samplerate)))
    {
      if (NULL != (p_start = (const char *)strchr (p_value, '=')))
        {
          /* skip the equal sign */
          p_start++;
          ap_prc->samplerate_ = convert_str_to_int (ap_prc, p_start, &p_end);
        }
    }
}

static void obtain_bit_rate (httpsrc_prc_t *ap_prc, char *ap_info)
{
  char *p_end = NULL;

  assert (NULL != ap_prc);
  assert (NULL != ap_info);

  TIZ_TRACE (handleOf (ap_prc), "bit rate  : [%s]", ap_info);

  ap_prc->bitrate_ = convert_str_to_int (ap_prc, ap_info, &p_end);
}

static OMX_ERRORTYPE set_audio_coding_on_port (httpsrc_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_HTTP_SOURCE_PORT_INDEX);
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

static OMX_ERRORTYPE set_mp3_audio_info_on_port (httpsrc_prc_t *ap_prc)
{
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (mp3type, ARATELIA_HTTP_SOURCE_PORT_INDEX);
  tiz_check_omx_err (tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)),
                                           handleOf (ap_prc),
                                           OMX_IndexParamAudioMp3, &mp3type));

  /* Set the new values */
  mp3type.nChannels = ap_prc->num_channels_;
  mp3type.nSampleRate = ap_prc->samplerate_;

  tiz_check_omx_err (tiz_krn_SetParameter_internal (
      tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
      OMX_IndexParamAudioMp3, &mp3type));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE set_aac_audio_info_on_port (httpsrc_prc_t *ap_prc)
{
  OMX_AUDIO_PARAM_AACPROFILETYPE aactype;
  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (aactype, ARATELIA_HTTP_SOURCE_PORT_INDEX);
  tiz_check_omx_err (tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)),
                                           handleOf (ap_prc),
                                           OMX_IndexParamAudioAac, &aactype));

  /* Set the new values */
  aactype.nChannels = ap_prc->num_channels_;
  aactype.nSampleRate = ap_prc->samplerate_;

  tiz_check_omx_err (tiz_krn_SetParameter_internal (
      tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
      OMX_IndexParamAudioAac, &aactype));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE set_opus_audio_info_on_port (httpsrc_prc_t *ap_prc)
{
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype;
  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (opustype, ARATELIA_HTTP_SOURCE_PORT_INDEX);
  tiz_check_omx_err (
      tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_TizoniaIndexParamAudioOpus, &opustype));

  /* Set the new values */
  opustype.nChannels = ap_prc->num_channels_;
  opustype.nSampleRate = ap_prc->samplerate_;

  tiz_check_omx_err (tiz_krn_SetParameter_internal (
      tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
      OMX_TizoniaIndexParamAudioOpus, &opustype));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE set_audio_info_on_port (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_prc);
  switch (ap_prc->audio_coding_type_)
    {
      case OMX_AUDIO_CodingMP3:
        {
          rc = set_mp3_audio_info_on_port (ap_prc);
        }
        break;
      case OMX_AUDIO_CodingAAC:
        {
          rc = set_aac_audio_info_on_port (ap_prc);
        }
        break;
      case OMX_AUDIO_CodingOPUS:
        {
          rc = set_opus_audio_info_on_port (ap_prc);
        }
        break;
      default:
        assert (0);
        break;
    };
  return rc;
}

static void update_cache_size (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (ap_prc->bitrate_ > 0);
  ap_prc->cache_bytes_ = ((ap_prc->bitrate_ * 1000) / 8)
                         * ARATELIA_HTTP_SOURCE_DEFAULT_CACHE_SECONDS;
}

static OMX_ERRORTYPE store_metadata (httpsrc_prc_t *ap_prc,
                                     const char *ap_header_name,
                                     const char *ap_header_info)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_CONFIG_METADATAITEMTYPE *p_meta = NULL;
  size_t metadata_len = 0;
  size_t info_len = 0;

  assert (NULL != ap_prc);
  assert (NULL != ap_header_name);
  assert (NULL != ap_header_info);

  info_len = strnlen (ap_header_info, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
  metadata_len = sizeof(OMX_CONFIG_METADATAITEMTYPE) + info_len;

  if (NULL == (p_meta = (OMX_CONFIG_METADATAITEMTYPE *)tiz_mem_calloc (
                   1, metadata_len)))
    {
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      const size_t name_len
          = strnlen (ap_header_name, OMX_MAX_STRINGNAME_SIZE - 1) + 1;
      strncpy ((char *)p_meta->nKey, ap_header_name, name_len - 1);
      p_meta->nKey[name_len - 1] = '\0';
      p_meta->nKeySizeUsed = name_len;

      strncpy ((char *)p_meta->nValue, ap_header_info, info_len - 1);
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

static void obtain_audio_encoding_from_headers (httpsrc_prc_t *ap_prc,
                                                const char *ap_header,
                                                const size_t a_size)
{
  const char *p_end = ap_header + a_size;
  const char *p_value = (const char *)memchr (ap_header, ':', a_size);
  char name[64];

  if (p_value != NULL && (size_t)(p_value - ap_header) < sizeof(name))
    {
      memcpy (name, ap_header, p_value - ap_header);
      name[p_value - ap_header] = 0;

      /* skip the colon */
      ++p_value;

      /* strip the value */
      while (p_value < p_end && !is_valid_character (*p_value))
        {
          ++p_value;
        }

      while (p_end > p_value && !is_valid_character (p_end[-1]))
        {
          --p_end;
        }

      {
        char *p_info = tiz_mem_calloc (1, (p_end - p_value) + 1);
        memcpy (p_info, p_value, p_end - p_value);
        p_info[(p_end - p_value)] = '\000';
        TIZ_TRACE (handleOf (ap_prc), "header name  : [%s]", name);
        TIZ_TRACE (handleOf (ap_prc), "header value : [%s]", p_info);

        (void)store_metadata (ap_prc, name, p_info);

        if (memcmp (name, "Content-Type", 12) == 0
            || memcmp (name, "content-type", 12) == 0)
          {
            obtain_coding_type (ap_prc, p_info);
            /* Now set the new coding type value on the output port */
            (void)set_audio_coding_on_port (ap_prc);
          }
        else if (memcmp (name, "ice-audio-info", 14) == 0)
          {
            obtain_audio_info (ap_prc, p_info);
            /* Now set the pcm info on the output port */
            (void)set_audio_info_on_port (ap_prc);
            /* Sometimes, the bitrate is provided in the ice-audio-info
               header */
            update_cache_size (ap_prc);
          }
        else if (memcmp (name, "icy-br", 6) == 0)
          {
            obtain_bit_rate (ap_prc, p_info);
            update_cache_size (ap_prc);
          }
        tiz_mem_free (p_info);
      }
    }
}

static void send_port_auto_detect_events (httpsrc_prc_t *ap_prc)
{

  if (ap_prc->audio_coding_type_ != OMX_AUDIO_CodingUnused
      || ap_prc->audio_coding_type_ != OMX_AUDIO_CodingAutoDetect)
    {
      TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_EventPortFormatDetected");
      tiz_srv_issue_event ((OMX_PTR)ap_prc, OMX_EventPortFormatDetected, 0, 0,
                           NULL);
      TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_EventPortSettingsChanged");
      tiz_srv_issue_event ((OMX_PTR)ap_prc, OMX_EventPortSettingsChanged,
                           ARATELIA_HTTP_SOURCE_PORT_INDEX, /* port 0 */
                           OMX_IndexParamPortDefinition,    /* the index of the
                                                            struct that has
                                                            been modififed */
                           NULL);
    }
  else
    {
      /* Oops... could not detect the stream format */
      tiz_srv_issue_err_event ((OMX_PTR)ap_prc, OMX_ErrorFormatNotDetected);
    }
}

/* This function gets called by libcurl as soon as it has received header
   data. The header callback will be called once for each header and only
   complete header lines are passed on to the callback. Parsing headers is very
   easy using this. The size of the data pointed to by ptr is size multiplied
   with nmemb. Do not assume that the header line is zero terminated! The
   pointer named userdata is the one you set with the CURLOPT_WRITEHEADER
   option. The callback function must return the number of bytes actually taken
   care of. If that amount differs from the amount passed to your function,
   it'll signal an error to the library. This will abort the transfer and
   return CURL_WRITE_ERROR. */
static size_t curl_header_cback (void *ptr, size_t size, size_t nmemb,
                                 void *userdata)
{
  httpsrc_prc_t *p_prc = userdata;
  size_t nbytes = size * nmemb;
  assert (NULL != p_prc);

  stop_reconnect_timer_watcher (p_prc);

  if (p_prc->auto_detect_on_)
    {
      obtain_audio_encoding_from_headers (p_prc, ptr, nbytes);
    }

  return nbytes;
}

OMX_ERRORTYPE process_cache (httpsrc_prc_t *p_prc)
{
  OMX_BUFFERHEADERTYPE *p_out = NULL;
  int nbytes_stored = 0;
  assert (NULL != p_prc);

  while ((nbytes_stored = tiz_buffer_bytes_available (p_prc->p_store_)) > 0
         && (p_out = buffer_needed (p_prc)) != NULL)
    {
      int nbytes_copied = copy_to_omx_buffer (
          p_out, tiz_buffer_get_data (p_prc->p_store_), nbytes_stored);
      TIZ_PRINTF_DBG_MAG (
          "Releasing buffer with size [%u] available [%u].",
          (unsigned int)p_out->nFilledLen,
          tiz_buffer_bytes_available (p_prc->p_store_) - nbytes_copied);
      tiz_check_omx_err (release_buffer (p_prc));
      (void)tiz_buffer_advance (p_prc->p_store_, nbytes_copied);
      p_out = NULL;
    }
  return OMX_ErrorNone;
}

/* This function gets called by libcurl as soon as there is data received that
   needs to be saved. The size of the data pointed to by ptr is size multiplied
   with nmemb, it will not be zero terminated. Return the number of bytes
   actually taken care of. If that amount differs from the amount passed to
   your function, it'll signal an error to the library. This will abort the
   transfer and return CURLE_WRITE_ERROR.  */
static size_t curl_write_cback (void *ptr, size_t size, size_t nmemb,
                                void *userdata)
{
  httpsrc_prc_t *p_prc = userdata;
  size_t nbytes = size * nmemb;
  size_t rc = nbytes;
  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "size [%d] nmemb [%d] sockfd [%d] store [%d]",
             size, nmemb, p_prc->sockfd_,
             tiz_buffer_bytes_available (p_prc->p_store_));

  if (nbytes > 0)
    {
      p_prc->curl_state_ = ECurlStateTransfering;
      OMX_BUFFERHEADERTYPE *p_out = NULL;

      if (p_prc->auto_detect_on_)
        {
          p_prc->auto_detect_on_ = false;

          /* This is to pause curl */
          TIZ_TRACE (handleOf (p_prc), "Pausing curl");
          rc = CURL_WRITEFUNC_PAUSE;
          p_prc->curl_state_ = ECurlStatePaused;

          /* Also stop the watchers */
          stop_io_watcher (p_prc);
          stop_curl_timer_watcher (p_prc);

          /* And now trigger the OMX_EventPortFormatDetected and
             OMX_EventPortSettingsChanged events or a
             OMX_ErrorFormatNotDetected event */
          send_port_auto_detect_events (p_prc);
        }
      else
        {

          if (tiz_buffer_bytes_available (p_prc->p_store_)
              > p_prc->cache_bytes_)
            {
              /* Reset the cache size */
              p_prc->cache_bytes_ = 0;

              process_cache (p_prc);

              while (nbytes > 0 && (p_out = buffer_needed (p_prc)) != NULL)
                {
                  int nbytes_copied = copy_to_omx_buffer (p_out, ptr, nbytes);
                  TIZ_PRINTF_DBG_CYN ("Releasing buffer with size [%u]",
                                      (unsigned int)p_out->nFilledLen);
                  tiz_check_omx_err (release_buffer (p_prc));
                  nbytes -= nbytes_copied;
                  ptr += nbytes_copied;
                }
            }

          if (nbytes > 0)
            {
              if (tiz_buffer_bytes_available (p_prc->p_store_)
                  > ((2 * (p_prc->bitrate_ * 1000) / 8)
                     * ARATELIA_HTTP_SOURCE_DEFAULT_CACHE_SECONDS))
                {
                  /* This is to pause curl */
                  TIZ_PRINTF_DBG_GRN (
                      "Pausing curl - cache size [%d]",
                      tiz_buffer_bytes_available (p_prc->p_store_));
                  rc = CURL_WRITEFUNC_PAUSE;
                  p_prc->curl_state_ = ECurlStatePaused;

                  /* Also stop the watchers */
                  stop_io_watcher (p_prc);
                  stop_curl_timer_watcher (p_prc);
                }
              else
                {
                  int nbytes_stored = 0;
                  if ((nbytes_stored = tiz_buffer_store_data (
                           p_prc->p_store_, ptr, nbytes)) < nbytes)
                    {
                      TIZ_ERROR (handleOf (p_prc),
                                 "Unable to store all the data (wanted %d, "
                                 "stored %d).",
                                 nbytes, nbytes_stored);
                    }
                  nbytes -= nbytes_stored;
                  TIZ_PRINTF_DBG_BLU (
                      "No more omx buffers, using the store : [%u] bytes "
                      "cached.",
                      tiz_buffer_bytes_available (p_prc->p_store_));
                  TIZ_TRACE (
                      handleOf (p_prc),
                      "Unable to get an omx buffer, using the temp store : "
                      "[%d] bytes stored.",
                      tiz_buffer_bytes_available (p_prc->p_store_));
                }
            }
        }
    }

  return rc;
}

/* #ifdef _DEBUG */
/* Pass a pointer to a function that matches the following prototype: int
   curl_debug_callback (CURL *, curl_infotype, char *, size_t, void *);
   CURLOPT_DEBUGFUNCTION replaces the standard debug function used when
   CURLOPT_VERBOSE is in effect. This callback receives debug information, as
   specified with the curl_infotype argument. This function must return 0. The
   data pointed to by the char * passed to this function WILL NOT be zero
   terminated, but will be exactly of the size as told by the size_t
   argument.  */
static size_t curl_debug_cback (CURL *p_curl, curl_infotype type, char *buf,
                                size_t nbytes, void *userdata)
{
  if (CURLINFO_TEXT == type || CURLINFO_HEADER_IN == type || CURLINFO_HEADER_OUT
                                                             == type)
    {
      httpsrc_prc_t *p_prc = userdata;
      char *p_info = tiz_mem_calloc (1, nbytes + 1);
      memcpy (p_info, buf, nbytes);
      TIZ_TRACE (handleOf (p_prc), "libcurl : [%s]", p_info);
      tiz_mem_free (p_info);
    }
  return 0;
}
/* #endif */

/* The curl_multi_socket_action(3) function informs the application
   about updates in the socket (file descriptor) status by doing none, one, or
   multiple calls to the curl_socket_callback given in the param argument. They
   update the status with changes since the previous time a
   curl_multi_socket(3) function was called. If the given callback pointer is
   NULL, no callback will be called. Set the callback's userp argument with
   CURLMOPT_SOCKETDATA. See curl_multi_socket(3) for more callback details. */

/* The callback MUST return 0. */

/* The easy argument is a pointer to the easy handle that deals with this
   particular socket. Note that a single handle may work with several sockets
   simultaneously. */

/* The s argument is the actual socket value as you use it within your
   system. */

/* The action argument to the callback has one of five values: */
/* CURL_POLL_NONE (0) */
/* register, not interested in readiness (yet) */
/* CURL_POLL_IN (1) */
/* register, interested in read readiness */
/* CURL_POLL_OUT (2) */
/* register, interested in write readiness */
/* CURL_POLL_INOUT (3) */
/* register, interested in both read and write readiness */
/* CURL_POLL_REMOVE (4) */
/* unregister */

/* The socketp argument is a private pointer you have previously set with
   curl_multi_assign(3) to be associated with the s socket. If no pointer has
   been set, socketp will be NULL. This argument is of course a service to
   applications that want to keep certain data or structs that are strictly
   associated to the given socket. */

/* The userp argument is a private pointer you have previously set with
   curl_multi_setopt(3) and the CURLMOPT_SOCKETDATA option.  */

static int curl_socket_cback (CURL *easy, curl_socket_t s, int action,
                              void *userp, void *socketp)
{
  httpsrc_prc_t *p_prc = userp;
  assert (NULL != p_prc);
  TIZ_DEBUG (
      handleOf (p_prc),
      "socket [%d] action [%d] (1 READ, 2 WRITE, 3 READ/WRITE, 4 REMOVE)", s,
      action);
  if (CURL_POLL_IN == action)
    {
      p_prc->sockfd_ = s;
      tiz_event_io_set (p_prc->p_ev_io_, s, TIZ_EVENT_READ, true);
      (void)start_io_watcher (p_prc);
    }
  else if (CURL_POLL_REMOVE == action)
    {
      (void)stop_io_watcher (p_prc);
      (void)stop_curl_timer_watcher (p_prc);
    }
  return 0;
}

/* This function will then be called when the timeout value changes. The
   timeout value is at what latest time the application should call one of the
   "performing" functions of the multi interface (curl_multi_socket_action(3)
   and curl_multi_perform(3)) - to allow libcurl to keep timeouts and retries
   etc to work. A timeout value of -1 means that there is no timeout at all,
   and 0 means that the timeout is already reached. Libcurl attempts to limit
   calling this only when the fixed future timeout time actually changes. See
   also CURLMOPT_TIMERDATA. The callback should return 0 on success, and -1 on
   error. This callback can be used instead of, or in addition to,
   curl_multi_timeout(3). (Added in 7.16.0) */
static int curl_timer_cback (CURLM *multi, long timeout_ms, void *userp)
{
  httpsrc_prc_t *p_prc = userp;
  assert (NULL != p_prc);

  TIZ_DEBUG (handleOf (p_prc), "timeout_ms : %d - curl_state_ [%d]", timeout_ms,
             p_prc->curl_state_);

  if (timeout_ms < 0 && ECurlStateTransfering == p_prc->curl_state_)
    {
      stop_curl_timer_watcher (p_prc);
      p_prc->curl_timeout_ = 0;
    }
  else if (timeout_ms > 0)
    {
      if (timeout_ms < 10)
        {
          timeout_ms = 10;
        }
      p_prc->curl_timeout_ = ((double)timeout_ms / (double)1000);
      stop_curl_timer_watcher (p_prc);
      (void)start_curl_timer_watcher (p_prc);
      if (ECurlStatePaused == p_prc->curl_state_)
        {
          (void)resume_curl (p_prc);
        }
    }
  return 0;
}

static void destroy_curl_resources (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  curl_slist_free_all (ap_prc->p_http_ok_aliases_);
  ap_prc->p_http_ok_aliases_ = NULL;
  curl_slist_free_all (ap_prc->p_http_headers_);
  ap_prc->p_http_headers_ = NULL;
  curl_multi_cleanup (ap_prc->p_curl_multi_);
  ap_prc->p_curl_multi_ = NULL;
  curl_easy_cleanup (ap_prc->p_curl_);
  ap_prc->p_curl_ = NULL;
}

static OMX_ERRORTYPE allocate_curl_global_resources (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  bail_on_curl_error (curl_global_init (CURL_GLOBAL_ALL));
  /* All well */
  rc = OMX_ErrorNone;
end:
  return rc;
}

static OMX_ERRORTYPE allocate_curl_resources (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  curl_version_info_data *p_version_info = NULL;

  assert (NULL == ap_prc->p_curl_);
  assert (NULL == ap_prc->p_curl_multi_);

  tiz_check_omx_err (allocate_curl_global_resources (ap_prc));

  TIZ_DEBUG (handleOf (ap_prc), "%s", curl_version ());

  p_version_info = curl_version_info (CURLVERSION_NOW);
  if (NULL != p_version_info)
    {
      ap_prc->curl_version_ = p_version_info->version_num;
    }

  /* Init the curl easy handle */
  tiz_check_null_ret_oom ((ap_prc->p_curl_ = curl_easy_init ()));
  /* Now init the curl multi handle */
  bail_on_oom ((ap_prc->p_curl_multi_ = curl_multi_init ()));
  /* this is to ask libcurl to accept ICY OK headers*/
  bail_on_oom ((ap_prc->p_http_ok_aliases_ = curl_slist_append (
                    ap_prc->p_http_ok_aliases_, "ICY 200 OK")));
  /* and this is to not ask the server for Icy metadata, for now */
  bail_on_oom ((ap_prc->p_http_headers_ = curl_slist_append (
                    ap_prc->p_http_headers_, "Icy-Metadata: 0")));

  /* all ok */
  rc = OMX_ErrorNone;

end:

  if (OMX_ErrorNone != rc)
    {
      /* Clean-up */
      destroy_curl_resources (ap_prc);
    }

  return rc;
}

static OMX_ERRORTYPE start_curl (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;

  TIZ_NOTICE (handleOf (ap_prc), "starting curl : curl_state_ [%d]",
              ap_prc->curl_state_);

  assert (NULL != ap_prc->p_curl_);
  assert (NULL != ap_prc->p_curl_multi_);
  assert (ECurlStateStopped == ap_prc->curl_state_ || ECurlStateConnecting
                                                      == ap_prc->curl_state_);

  ap_prc->curl_state_ = ECurlStateConnecting;

  /* associate the processor with the curl handle */
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_PRIVATE, ap_prc));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_USERAGENT,
                                        ARATELIA_HTTP_SOURCE_COMPONENT_NAME));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_HEADERFUNCTION,
                                        curl_header_cback));
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_WRITEHEADER, ap_prc));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_WRITEFUNCTION,
                                        curl_write_cback));
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_WRITEDATA, ap_prc));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_HTTP200ALIASES,
                                        ap_prc->p_http_ok_aliases_));
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_FOLLOWLOCATION, 1));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_NETRC, 1));
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_MAXREDIRS, 5));
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_FAILONERROR, 1)); /* true */
  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_ERRORBUFFER,
                                        ap_prc->curl_err));
  /* no progress meter */
  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_NOPROGRESS, 1));

  bail_on_curl_error (
      curl_easy_setopt (ap_prc->p_curl_, CURLOPT_CONNECTTIMEOUT, 10));

  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_URL,
                                        ap_prc->p_uri_param_->contentURI));

  bail_on_curl_error (curl_easy_setopt (ap_prc->p_curl_, CURLOPT_HTTPHEADER,
                                        ap_prc->p_http_headers_));

  /* #ifdef _DEBUG */
  curl_easy_setopt (ap_prc->p_curl_, CURLOPT_VERBOSE, 1);
  curl_easy_setopt (ap_prc->p_curl_, CURLOPT_DEBUGDATA, ap_prc);
  curl_easy_setopt (ap_prc->p_curl_, CURLOPT_DEBUGFUNCTION, curl_debug_cback);
  /* #endif */

  /* Set the socket callback with CURLMOPT_SOCKETFUNCTION */
  bail_on_curl_multi_error (curl_multi_setopt (
      ap_prc->p_curl_multi_, CURLMOPT_SOCKETFUNCTION, curl_socket_cback));
  bail_on_curl_multi_error (
      curl_multi_setopt (ap_prc->p_curl_multi_, CURLMOPT_SOCKETDATA, ap_prc));
  /* Set the timeout callback with CURLMOPT_TIMERFUNCTION, to get to know what
     timeout value to use when waiting for socket activities. */
  bail_on_curl_multi_error (curl_multi_setopt (
      ap_prc->p_curl_multi_, CURLMOPT_TIMERFUNCTION, curl_timer_cback));
  bail_on_curl_multi_error (
      curl_multi_setopt (ap_prc->p_curl_multi_, CURLMOPT_TIMERDATA, ap_prc));
  /* Add the easy handle to the multi */
  bail_on_curl_multi_error (
      curl_multi_add_handle (ap_prc->p_curl_multi_, ap_prc->p_curl_));

  /* all ok */
  rc = OMX_ErrorNone;

end:

  return rc;
}

static inline void delete_uri (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static OMX_ERRORTYPE obtain_uri (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (NULL != ap_prc);
  assert (NULL == ap_prc->p_uri_param_);

  ap_prc->p_uri_param_
      = tiz_mem_calloc (1, sizeof(OMX_PARAM_CONTENTURITYPE) + pathname_max + 1);

  if (NULL == ap_prc->p_uri_param_)
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "Error allocating memory for the content uri struct");
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      ap_prc->p_uri_param_->nSize = sizeof(OMX_PARAM_CONTENTURITYPE)
                                    + pathname_max + 1;
      ap_prc->p_uri_param_->nVersion.nVersion = OMX_VERSION;

      tiz_check_omx_err (tiz_api_GetParameter (
          tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
          OMX_IndexParamContentURI, ap_prc->p_uri_param_));
      TIZ_NOTICE (handleOf (ap_prc), "URI [%s]",
                  ap_prc->p_uri_param_->contentURI);
      /* Verify we are getting an http scheme */
      if (memcmp (ap_prc->p_uri_param_->contentURI, "http://", 7) != 0
          && memcmp (ap_prc->p_uri_param_->contentURI, "https://", 8) != 0)
        {
          rc = OMX_ErrorContentURIError;
        }
    }

  return rc;
}

static OMX_ERRORTYPE allocate_events (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  assert (NULL == ap_prc->p_ev_io_);
  assert (NULL == ap_prc->p_ev_curl_timer_);

  /* Allocate the io event */
  tiz_check_omx_err (tiz_event_io_init (&(ap_prc->p_ev_io_), handleOf (ap_prc),
                                        tiz_comp_event_io));

  /* Allocate the reconnect timer event */
  tiz_check_omx_err (tiz_event_timer_init (&(ap_prc->p_ev_reconnect_timer_),
                                           handleOf (ap_prc),
                                           tiz_comp_event_timer, ap_prc));

  /* Allocate the curl timer event */
  tiz_check_omx_err (tiz_event_timer_init (&(ap_prc->p_ev_curl_timer_),
                                           handleOf (ap_prc),
                                           tiz_comp_event_timer, ap_prc));

  return OMX_ErrorNone;
}

static void destroy_events (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_event_io_destroy (ap_prc->p_ev_io_);
  ap_prc->p_ev_io_ = NULL;
  tiz_event_timer_destroy (ap_prc->p_ev_curl_timer_);
  ap_prc->p_ev_curl_timer_ = NULL;
  tiz_event_timer_destroy (ap_prc->p_ev_reconnect_timer_);
  ap_prc->p_ev_reconnect_timer_ = NULL;
}

static OMX_ERRORTYPE release_buffer (httpsrc_prc_t *ap_prc)
{
  assert (NULL != ap_prc);

  if (ap_prc->p_outhdr_)
    {
      TIZ_NOTICE (handleOf (ap_prc), "releasing HEADER [%p] nFilledLen [%d]",
                  ap_prc->p_outhdr_, ap_prc->p_outhdr_->nFilledLen);
      tiz_check_omx_err (tiz_krn_release_buffer (
          tiz_get_krn (handleOf (ap_prc)), 0, ap_prc->p_outhdr_));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_BUFFERHEADERTYPE *buffer_needed (httpsrc_prc_t *ap_prc)
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
                                        ARATELIA_HTTP_SOURCE_PORT_INDEX, 0,
                                        &ap_prc->p_outhdr_)))
            {
              if (NULL != ap_prc->p_outhdr_)
                {
                  TIZ_TRACE (handleOf (ap_prc),
                             "Claimed HEADER [%p]...nFilledLen [%d]",
                             ap_prc->p_outhdr_, ap_prc->p_outhdr_->nFilledLen);
                  p_hdr = ap_prc->p_outhdr_;
                }
            }
        }
    }
  return p_hdr;
}

static OMX_ERRORTYPE prepare_for_port_auto_detection (httpsrc_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_HTTP_SOURCE_PORT_INDEX);
  tiz_check_omx_err (
      tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_IndexParamPortDefinition, &port_def));
  ap_prc->audio_coding_type_ = port_def.format.audio.eEncoding;
  ap_prc->auto_detect_on_
      = (OMX_AUDIO_CodingAutoDetect == ap_prc->audio_coding_type_) ? true
                                                                   : false;

  TIZ_TRACE (
      handleOf (ap_prc), "auto_detect_on_ [%s]...audio_coding_type_ [%d]",
      ap_prc->auto_detect_on_ ? "true" : "false", ap_prc->audio_coding_type_);

  return OMX_ErrorNone;
}

/*
 * httpsrcprc
 */

static void *httpsrc_prc_ctor (void *ap_obj, va_list *app)
{
  httpsrc_prc_t *p_prc
      = super_ctor (typeOf (ap_obj, "httpsrcprc"), ap_obj, app);
  p_prc->p_outhdr_ = NULL;
  p_prc->p_uri_param_ = NULL;
  p_prc->eos_ = false;
  p_prc->port_disabled_ = false;
  p_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
  p_prc->num_channels_ = 2;
  p_prc->samplerate_ = 44100;
  p_prc->auto_detect_on_ = false;
  p_prc->p_ev_io_ = NULL;
  p_prc->sockfd_ = -1;
  p_prc->awaiting_io_ev_ = false;
  p_prc->p_ev_curl_timer_ = NULL;
  p_prc->awaiting_curl_timer_ev_ = false;
  p_prc->curl_timeout_ = 0;
  p_prc->p_ev_reconnect_timer_ = NULL;
  p_prc->bitrate_ = ARATELIA_HTTP_SOURCE_DEFAULT_BIT_RATE_KBITS;
  p_prc->cache_bytes_ = ((ARATELIA_HTTP_SOURCE_DEFAULT_BIT_RATE_KBITS * 1000)
                         / 8) * ARATELIA_HTTP_SOURCE_DEFAULT_CACHE_SECONDS;
  p_prc->awaiting_reconnect_timer_ev_ = false;
  p_prc->reconnect_timeout_ = ARATELIA_HTTP_SOURCE_DEFAULT_RECONNECT_TIMEOUT;
  p_prc->p_store_ = NULL;
  p_prc->p_curl_ = NULL;
  p_prc->p_curl_multi_ = NULL;
  p_prc->p_http_ok_aliases_ = NULL;
  p_prc->p_http_headers_ = NULL;
  p_prc->curl_state_ = ECurlStateStopped;
  p_prc->curl_version_ = 0;
  return p_prc;
}

static void *httpsrc_prc_dtor (void *ap_obj)
{
  (void)httpsrc_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "httpsrcprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE httpsrc_prc_allocate_resources (void *ap_obj,
                                                     OMX_U32 a_pid)
{
  httpsrc_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  assert (NULL == p_prc->p_uri_param_);
  tiz_check_omx_err (allocate_temp_data_store (p_prc));
  tiz_check_omx_err (obtain_uri (p_prc));
  tiz_check_omx_err (allocate_events (p_prc));
  tiz_check_omx_err (allocate_curl_resources (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_deallocate_resources (void *ap_obj)
{
  deallocate_temp_data_store (ap_obj);
  destroy_events (ap_obj);
  destroy_curl_resources (ap_obj);
  delete_uri (ap_obj);
  curl_global_cleanup ();
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_prepare_to_transfer (void *ap_obj,
                                                      OMX_U32 a_pid)
{
  httpsrc_prc_t *p_prc = ap_obj;
  assert (NULL != ap_obj);

  p_prc->eos_ = false;
  p_prc->sockfd_ = -1;
  p_prc->awaiting_io_ev_ = false;
  p_prc->awaiting_curl_timer_ev_ = false;
  p_prc->curl_timeout_ = 0;

  return prepare_for_port_auto_detection (p_prc);
}

static OMX_ERRORTYPE httpsrc_prc_transfer_and_process (void *ap_prc,
                                                       OMX_U32 a_pid)
{
  httpsrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);

  if (p_prc->auto_detect_on_ && ECurlStateStopped == p_prc->curl_state_)
    {
      int running_handles = 0;
      tiz_check_omx_err (start_curl (p_prc));
      assert (NULL != p_prc->p_curl_multi_);
      /* Kickstart curl to get one or more callbacks called. */
      on_curl_multi_error_ret_omx_oom (curl_multi_socket_action (
          p_prc->p_curl_multi_, CURL_SOCKET_TIMEOUT, 0, &running_handles));
      TIZ_NOTICE (handleOf (p_prc), "running handles [%d]", running_handles);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_stop_and_return (void *ap_obj)
{
  stop_io_watcher (ap_obj);
  stop_curl_timer_watcher (ap_obj);
  stop_reconnect_timer_watcher (ap_obj);
  return release_buffer (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE httpsrc_prc_buffers_ready (const void *ap_prc)
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_prc;
  assert (NULL != p_prc);

  TIZ_TRACE (handleOf (ap_prc), "Received buffer event : curl_state_ [%d]",
             p_prc->curl_state_);

  if (ECurlStatePaused == p_prc->curl_state_)
    {
      if (tiz_buffer_bytes_available (p_prc->p_store_)
          > (((p_prc->bitrate_ * 1000) / 8)
             * ARATELIA_HTTP_SOURCE_DEFAULT_CACHE_SECONDS))
        {
          process_cache (p_prc);
        }
      else
        {
          tiz_check_omx_err (resume_curl (p_prc));
        }
    }
  else if (ECurlStateTransfering == p_prc->curl_state_)
    {
      process_cache (p_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_io_ready (void *ap_prc,
                                           tiz_event_io_t *ap_ev_io, int a_fd,
                                           int a_events)
{
  httpsrc_prc_t *p_prc = ap_prc;
  assert (NULL != p_prc);

  TIZ_TRACE (handleOf (ap_prc), "awaiting_io_ev_ [%s]",
             p_prc->awaiting_io_ev_ ? "TRUE" : "FALSE");

  if (p_prc->awaiting_io_ev_)
    {
      int running_handles = 0;
      int curl_ev_bitmask = 0;
      if (TIZ_EVENT_READ == a_events || TIZ_EVENT_READ_OR_WRITE == a_events)
        {
          curl_ev_bitmask |= CURL_CSELECT_IN;
        }
      if (TIZ_EVENT_WRITE == a_events || TIZ_EVENT_READ_OR_WRITE == a_events)
        {
          curl_ev_bitmask |= CURL_CSELECT_OUT;
        }
      tiz_check_omx_err (stop_io_watcher (ap_prc));
      tiz_check_omx_err (restart_curl_timer_watcher (ap_prc));
      on_curl_multi_error_ret_omx_oom (curl_multi_socket_action (
          p_prc->p_curl_multi_, a_fd, curl_ev_bitmask, &running_handles));
      TIZ_TRACE (
          handleOf (ap_prc),
          "Received io event on fd [%d] events [%d] running handles [%d]", a_fd,
          a_events, running_handles);
      if (ECurlStatePaused != p_prc->curl_state_ && running_handles > 0)
        {
          tiz_check_omx_err (start_io_watcher (ap_prc));
        }
      else if (0 == running_handles)
        {
          prepare_for_port_auto_detection (p_prc);
          p_prc->curl_state_ = ECurlStateStopped;
          update_cache_size (p_prc);
          (void)start_reconnect_timer_watcher (p_prc);
        }
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_timer_ready (void *ap_prc,
                                              tiz_event_timer_t *ap_ev_timer,
                                              void *ap_arg)
{
  httpsrc_prc_t *p_prc = ap_prc;
  int running_handles = 0;

  assert (NULL != p_prc);

  TIZ_NOTICE (
      handleOf (p_prc),
      "01 - Received timer event [%p] : curl_state_ [%d] curl_timeout_ [%f]",
      ap_ev_timer, p_prc->curl_state_, p_prc->curl_timeout_);

  if (p_prc->awaiting_curl_timer_ev_ && ap_ev_timer == p_prc->p_ev_curl_timer_)
    {
      TIZ_NOTICE (handleOf (p_prc),
                  "01 - Received curl timer event [%p] : curl_state_ [%d] "
                  "curl_timeout_ [%f]",
                  ap_ev_timer, p_prc->curl_state_, p_prc->curl_timeout_);

      if (ECurlStateTransfering == p_prc->curl_state_ || ECurlStateConnecting
                                                         == p_prc->curl_state_)
        {
          tiz_check_omx_err (restart_curl_timer_watcher (ap_arg));
          /* stop_curl_timer_watcher (p_prc); */
          /* start_curl_timer_watcher (p_prc); */
          on_curl_multi_error_ret_omx_oom (curl_multi_socket_action (
              p_prc->p_curl_multi_, CURL_SOCKET_TIMEOUT, 0, &running_handles));
          TIZ_NOTICE (handleOf (p_prc),
                      "02 - running handles [%d] curl_timeout_ [%f]",
                      running_handles, p_prc->curl_timeout_);
          if (0 == running_handles)
            {
              p_prc->curl_state_ = ECurlStateStopped;
              stop_curl_timer_watcher (p_prc);
              update_cache_size (p_prc);
              start_reconnect_timer_watcher (p_prc);
            }
        }
    }
  else if (p_prc->awaiting_reconnect_timer_ev_
           && ap_ev_timer == p_prc->p_ev_reconnect_timer_)
    {
      TIZ_NOTICE (handleOf (p_prc),
                  "01 - Received reconnect timer event [%p] : curl_state_ [%d] "
                  "reconnect_timeout_ [%f]",
                  ap_ev_timer, p_prc->curl_state_, p_prc->reconnect_timeout_);
      TIZ_PRINTF_RED ("\rFailed to connect to '%s'.",
                      p_prc->p_uri_param_->contentURI);
      TIZ_PRINTF_RED ("Re-connecting radio station in %.1f seconds.\n",
                      p_prc->reconnect_timeout_);
      curl_multi_remove_handle (p_prc->p_curl_multi_, p_prc->p_curl_);
      start_curl (p_prc);
      on_curl_multi_error_ret_omx_oom (curl_multi_socket_action (
          p_prc->p_curl_multi_, CURL_SOCKET_TIMEOUT, 0, &running_handles));
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_pause (const void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_resume (const void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_port_flush (const void *ap_obj,
                                             OMX_U32 TIZ_UNUSED (a_pid))
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_obj;
  assert (NULL != p_prc);
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE httpsrc_prc_port_disable (const void *ap_obj,
                                               OMX_U32 TIZ_UNUSED (a_pid))
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_obj;
  assert (NULL != p_prc);
  p_prc->port_disabled_ = true;
  /* Release any buffers held  */
  return release_buffer ((httpsrc_prc_t *)ap_obj);
}

static OMX_ERRORTYPE httpsrc_prc_port_enable (const void *ap_prc, OMX_U32 a_pid)
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_prc;
  assert (NULL != p_prc);
  TIZ_NOTICE (handleOf (p_prc), "Enabling port [%d] was disabled? [%s]", a_pid,
              p_prc->port_disabled_ ? "YES" : "NO");
  if (p_prc->port_disabled_)
    {
      int running_handles = 0;
      p_prc->port_disabled_ = false;
      tiz_check_omx_err (restart_curl_timer_watcher (p_prc));
      on_curl_multi_error_ret_omx_oom (curl_multi_socket_action (
          p_prc->p_curl_multi_, CURL_SOCKET_TIMEOUT, 0, &running_handles));
      TIZ_NOTICE (handleOf (p_prc), "running handles [%d]", running_handles);
    }
  return OMX_ErrorNone;
}

/*
 * httpsrc_prc_class
 */

static void *httpsrc_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "httpsrcprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *httpsrc_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *httpsrcprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizprc), "httpsrcprc_class", classOf (tizprc),
       sizeof(httpsrc_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, httpsrc_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return httpsrcprc_class;
}

void *httpsrc_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *httpsrcprc_class = tiz_get_type (ap_hdl, "httpsrcprc_class");
  TIZ_LOG_CLASS (httpsrcprc_class);
  void *httpsrcprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (httpsrcprc_class, "httpsrcprc", tizprc, sizeof(httpsrc_prc_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, httpsrc_prc_ctor,
       /* TIZ_CLASS_COMMENT: class destructor */
       dtor, httpsrc_prc_dtor,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_allocate_resources, httpsrc_prc_allocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_deallocate_resources, httpsrc_prc_deallocate_resources,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_prepare_to_transfer, httpsrc_prc_prepare_to_transfer,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_transfer_and_process, httpsrc_prc_transfer_and_process,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_stop_and_return, httpsrc_prc_stop_and_return,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, httpsrc_prc_buffers_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_io_ready, httpsrc_prc_io_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_timer_ready, httpsrc_prc_timer_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_pause, httpsrc_prc_pause,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_resume, httpsrc_prc_resume,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_flush, httpsrc_prc_port_flush,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_disable, httpsrc_prc_port_disable,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_port_enable, httpsrc_prc_port_enable,
       /* TIZ_CLASS_COMMENT: stop value */
       0);

  return httpsrcprc;
}

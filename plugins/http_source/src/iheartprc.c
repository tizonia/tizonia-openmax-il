/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   iheartprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Iheart streaming client - processor class
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
#include "iheartprc.h"
#include "iheartprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_source.prc.iheart"
#endif

#define IHEARTSRC_MAX_STRING_SIZE 2 * OMX_MAX_STRINGNAME_SIZE
#define IGNORE_VALUE INT_MAX

/* forward declarations */
static OMX_ERRORTYPE
iheart_prc_deallocate_resources (void *);
static OMX_ERRORTYPE
release_buffer (iheart_prc_t *);
static OMX_ERRORTYPE
prepare_for_port_auto_detection (iheart_prc_t * ap_prc);
static OMX_ERRORTYPE
iheart_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid);
static OMX_ERRORTYPE
iheart_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid);

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

static inline bool
is_valid_character (const char c)
{
  return (unsigned char) c > 0x20;
}

static OMX_ERRORTYPE
obtain_coding_type (iheart_prc_t * ap_prc, char * ap_info)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);
  assert (ap_info);

  TIZ_TRACE (handleOf (ap_prc), "encoding type  : [%s]", ap_info);

  if (strncasecmp (ap_info, "audio/mpeg", 10) == 0
      || strncasecmp (ap_info, "audio/mpg", 9) == 0
      || strncasecmp (ap_info, "audio/mp3", 9) == 0)
    {
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingMP3;
    }
  else if (strncasecmp (ap_info, "audio/aac", 9) == 0
           || strncasecmp (ap_info, "audio/aacp", 10) == 0)
    {
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingAAC;
    }
  else if (strncasecmp (ap_info, "audio/vorbis", 12) == 0)
    {
      /* This is vorbis without container */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingVORBIS;
    }
  else if (strncasecmp (ap_info, "audio/speex", 11) == 0)
    {
      /* This is speex without container */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingSPEEX;
    }
  else if (strncasecmp (ap_info, "audio/flac", 10) == 0)
    {
      /* This is flac without container */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingFLAC;
    }
  else if (strncasecmp (ap_info, "audio/opus", 10) == 0)
    {
      /* This is opus without container */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingOPUS;
    }
  else if (strncasecmp (ap_info, "application/ogg", 15) == 0
           || strncasecmp (ap_info, "audio/ogg", 9) == 0)
    {
      /* This is for audio with ogg container (may be FLAC, Vorbis, Opus,
         etc). We'll have to identify the actual codec when the first bytes
         from the stream arrive */
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingOGA;
    }
  else
    {
      ap_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
      rc = OMX_ErrorInsufficientResources;
    }
  return rc;
}

static OMX_ERRORTYPE
set_audio_coding_on_port (iheart_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_HTTP_SOURCE_PORT_INDEX);
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
set_auto_detect_on_port (iheart_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_HTTP_SOURCE_PORT_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));

  /* Set the new value */
  port_def.format.audio.eEncoding = OMX_AUDIO_CodingAutoDetect;

  tiz_check_omx (tiz_krn_SetParameter_internal (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_IndexParamPortDefinition, &port_def));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
store_metadata (iheart_prc_t * ap_prc, const char * ap_header_name,
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

static void
obtain_audio_encoding_from_headers (iheart_prc_t * ap_prc,
                                    const char * ap_header, const size_t a_size)
{
  assert (ap_prc);
  assert (ap_header);
  {
    const char * p_end = ap_header + a_size;
    const char * p_value = (const char *) memchr (ap_header, ':', a_size);
    char name[64];

    if (p_value && (size_t) (p_value - ap_header) < sizeof (name))
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
          char * p_info = tiz_mem_calloc (1, (p_end - p_value) + 1);
          memcpy (p_info, p_value, p_end - p_value);
          p_info[(p_end - p_value)] = '\0';
          TIZ_TRACE (handleOf (ap_prc), "header name  : [%s]", name);
          TIZ_TRACE (handleOf (ap_prc), "header value : [%s]", p_info);

          if (strncasecmp (name, "content-type", 12) == 0)
            {
              if (OMX_ErrorNone == obtain_coding_type (ap_prc, p_info))
                {
                  /* Now set the new coding type value on the output port */
                  (void) set_audio_coding_on_port (ap_prc);
                }
            }
          tiz_mem_free (p_info);
        }
      }
  }
}

static void
send_port_auto_detect_events (iheart_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->audio_coding_type_ != OMX_AUDIO_CodingUnused
      && ap_prc->audio_coding_type_ != OMX_AUDIO_CodingAutoDetect)
    {
      TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_EventPortFormatDetected");
      tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortFormatDetected, 0, 0,
                           NULL);
      TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_EventPortSettingsChanged");
      tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortSettingsChanged,
                           ARATELIA_HTTP_SOURCE_PORT_INDEX, /* port 0 */
                           OMX_IndexParamPortDefinition,    /* the index of the
                                                         struct that has
                                                         been modififed */
                           NULL);
    }
  else
    {
      /* Oops... could not detect the stream format */

      /* This is to make sure this url will not get processed again... */
      ap_prc->remove_current_url_ = true;

      /* Get ready to auto-detect another stream */
      set_auto_detect_on_port (ap_prc);
      prepare_for_port_auto_detection (ap_prc);

      /* Finally, signal the client */
      TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_ErrorFormatNotDetected");
      tiz_srv_issue_err_event ((OMX_PTR) ap_prc, OMX_ErrorFormatNotDetected);
    }
}

static inline void
delete_uri (iheart_prc_t * ap_prc)
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static OMX_ERRORTYPE
update_metadata (iheart_prc_t * ap_prc)
{
  assert (ap_prc);

  /* Clear previous metatada items */
  tiz_krn_clear_metadata (tiz_get_krn (handleOf (ap_prc)));

  /* Station Name */
  {
    char name_str[IHEARTSRC_MAX_STRING_SIZE];
    snprintf (name_str, IHEARTSRC_MAX_STRING_SIZE - 1, "%s  (%s)",
              tiz_iheart_get_current_radio_name (ap_prc->p_iheart_),
              tiz_iheart_get_current_queue_progress (ap_prc->p_iheart_));

    tiz_check_omx (store_metadata (ap_prc, "Station", name_str));
  }

  /* Station Description */
  tiz_check_omx (
    store_metadata (ap_prc, "Description",
                    tiz_iheart_get_current_radio_description (ap_prc->p_iheart_)));

  /* City */
  tiz_check_omx (
    store_metadata (ap_prc, "City",
                    tiz_iheart_get_current_radio_city (ap_prc->p_iheart_)));

  /* State */
  tiz_check_omx (
    store_metadata (ap_prc, "State",
                    tiz_iheart_get_current_radio_state (ap_prc->p_iheart_)));

  /* Audio Encoding */
  tiz_check_omx (
    store_metadata (ap_prc, "Encoding",
                    tiz_iheart_get_current_radio_audio_encoding (ap_prc->p_iheart_)));

  /* Website */
  tiz_check_omx (
    store_metadata (ap_prc, "Website",
                    tiz_iheart_get_current_radio_website_url (ap_prc->p_iheart_)));

  /* Streaming URL */
  tiz_check_omx (store_metadata (
    ap_prc, "Streaming URL", (const char *) ap_prc->p_uri_param_->contentURI));

  /* Thumbnail */
  tiz_check_omx (store_metadata (
    ap_prc, "Thumbnail URL",
    tiz_iheart_get_current_radio_thumbnail_url (ap_prc->p_iheart_)));

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
obtain_next_url (iheart_prc_t * ap_prc, int a_skip_value,
                 const int a_position_value)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);
  assert (ap_prc->p_iheart_);

  if (0 == tiz_iheart_get_current_queue_length_as_int (ap_prc->p_iheart_))
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "No more URLs in queue (OMX_ErrorInsufficientResources)");
      return OMX_ErrorInsufficientResources;
    }

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
    const char * p_next_url = NULL;

    if (IGNORE_VALUE != a_skip_value)
      {
        p_next_url = a_skip_value > 0
                       ? tiz_iheart_get_next_url (ap_prc->p_iheart_,
                                                  ap_prc->remove_current_url_)
                       : tiz_iheart_get_prev_url (ap_prc->p_iheart_,
                                                  ap_prc->remove_current_url_);
        ap_prc->remove_current_url_ = false;
      }
    else if (IGNORE_VALUE != a_position_value)
      {
        p_next_url = tiz_iheart_get_url (ap_prc->p_iheart_, a_position_value);
      }
    else
      {
        assert (0);
      }

    tiz_check_null_ret_oom (p_next_url);

    {
      const OMX_U32 url_len = strnlen (p_next_url, pathname_max);
      TIZ_TRACE (handleOf (ap_prc), "URL [%s]", p_next_url);

      /* Verify we are getting an http scheme */
      if (!p_next_url || !url_len
          || (strncasecmp (p_next_url, "http://", 7) != 0
              && strncasecmp (p_next_url, "https://", 8) != 0))
        {
          rc = OMX_ErrorContentURIError;
        }
      else
        {
          strncpy ((char *) ap_prc->p_uri_param_->contentURI, p_next_url,
                   url_len);
          ap_prc->p_uri_param_->contentURI[url_len] = '\0';

          /* Song metadata is now available, update the IL client */
          rc = update_metadata (ap_prc);
        }
    }
  }

  return rc;
}

static OMX_ERRORTYPE
release_buffer (iheart_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_outhdr_)
    {
      if (ap_prc->eos_)
        {
          ap_prc->eos_ = false;
          ap_prc->p_outhdr_->nFlags |= OMX_BUFFERFLAG_EOS;
        }
      tiz_check_omx (tiz_krn_release_buffer (
        tiz_get_krn (handleOf (ap_prc)), ARATELIA_HTTP_SOURCE_PORT_INDEX,
        ap_prc->p_outhdr_));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static void
buffer_filled (OMX_BUFFERHEADERTYPE * ap_hdr, void * ap_arg)
{
  iheart_prc_t * p_prc = ap_arg;
  assert (p_prc);
  assert (ap_hdr);
  assert (p_prc->p_outhdr_ == ap_hdr);
  if (ARATELIA_HTTP_SOURCE_PORT_MIN_BUF_SIZE <= ap_hdr->nFilledLen
      || p_prc->connection_closed_ || p_prc->first_buffer_delivered_)
    {
      (void) release_buffer (p_prc);
      p_prc->first_buffer_delivered_ = true;
    }
}

static OMX_BUFFERHEADERTYPE *
buffer_emptied (OMX_PTR ap_arg)
{
  iheart_prc_t * p_prc = ap_arg;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  assert (p_prc);

  if (!p_prc->port_disabled_)
    {
      if (p_prc->p_outhdr_)
        {
          p_hdr = p_prc->p_outhdr_;
        }
      else
        {
          if (OMX_ErrorNone
              == (tiz_krn_claim_buffer (tiz_get_krn (handleOf (p_prc)),
                                        ARATELIA_HTTP_SOURCE_PORT_INDEX, 0,
                                        &p_prc->p_outhdr_)))
            {
              if (p_prc->p_outhdr_)
                {
                  TIZ_TRACE (handleOf (p_prc),
                             "Claimed HEADER [%p]...nFilledLen [%d]",
                             p_prc->p_outhdr_, p_prc->p_outhdr_->nFilledLen);
                  p_hdr = p_prc->p_outhdr_;
                }
              else
                {
                  TIZ_TRACE (handleOf (p_prc), "No more headers available");
                }
            }
        }
    }
  return p_hdr;
}

static void
header_available (OMX_PTR ap_arg, const void * ap_ptr, const size_t a_nbytes)
{
  iheart_prc_t * p_prc = ap_arg;
  assert (p_prc);
  assert (ap_ptr);
  obtain_audio_encoding_from_headers (p_prc, ap_ptr, a_nbytes);
}

static bool
data_available (OMX_PTR ap_arg, const void * ap_ptr, const size_t a_nbytes)
{
  iheart_prc_t * p_prc = ap_arg;
  bool pause_needed = false;
  assert (p_prc);
  assert (ap_ptr);

  TIZ_DEBUG (handleOf (p_prc), "p_prc->auto_detect_on_ [%s]",
             (p_prc->auto_detect_on_ ? "TRUE" : "FALSE"));

  if (p_prc->auto_detect_on_ && a_nbytes > 0)
    {
      p_prc->auto_detect_on_ = false;

      /* This will pause the http transfer */
      pause_needed = true;

      /* And now trigger the OMX_EventPortFormatDetected and
         OMX_EventPortSettingsChanged events or a
         OMX_ErrorFormatNotDetected event */
      send_port_auto_detect_events (p_prc);
    }
  return pause_needed;
}

static bool
connection_lost (OMX_PTR ap_arg)
{
  iheart_prc_t * p_prc = ap_arg;
  assert (p_prc);
  TIZ_PRINTF_DBG_RED ("connection_lost\n");

  if (p_prc->auto_detect_on_)
    {
      /* Oops... unable to connect to the station */
      TIZ_PRINTF_C01("[Iheart] Unable to connect/connection lost.");
      /* Make sure this url will not get processed again... */
      p_prc->remove_current_url_ = true;

      /* Get ready to auto-detect another stream */
      set_auto_detect_on_port (p_prc);
      prepare_for_port_auto_detection (p_prc);

      /* Signal the client */
      tiz_srv_issue_err_event ((OMX_PTR) p_prc, OMX_ErrorFormatNotDetected);
    }

  p_prc->connection_closed_ = true;

  /* Return false to indicate that there is no need to start the automatic
     reconnection procedure */
  return false;
}

static OMX_ERRORTYPE
prepare_for_port_auto_detection (iheart_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_HTTP_SOURCE_PORT_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));
  ap_prc->audio_coding_type_ = port_def.format.audio.eEncoding;
  ap_prc->auto_detect_on_
    = (OMX_AUDIO_CodingAutoDetect == ap_prc->audio_coding_type_) ? true : false;

  TIZ_TRACE (
    handleOf (ap_prc), "auto_detect_on_ [%s]...audio_coding_type_ [%d]",
    ap_prc->auto_detect_on_ ? "true" : "false", ap_prc->audio_coding_type_);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
retrieve_session_configuration (iheart_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioIheartSession, &(ap_prc->session_));
}

static OMX_ERRORTYPE
retrieve_playlist (iheart_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioIheartPlaylist, &(ap_prc->playlist_));
}

static OMX_ERRORTYPE
retrieve_buffer_size (iheart_prc_t * ap_prc)
{
  TIZ_INIT_OMX_PORT_STRUCT (ap_prc->buffer_size_, ARATELIA_HTTP_SOURCE_PORT_INDEX);
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamStreamingBuffer, &(ap_prc->buffer_size_));
}

static OMX_ERRORTYPE
enqueue_playlist_items (iheart_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_iheart_);

  {
    const char * p_playlist = (const char *) ap_prc->playlist_.cPlaylistName;
    const char * p_keywords1 = (const char *) ap_prc->playlist_.cAdditionalKeywords1;
    const char * p_keywords2 = (const char *) ap_prc->playlist_.cAdditionalKeywords2;
    const char * p_keywords3 = (const char *) ap_prc->playlist_.cAdditionalKeywords3;
    const OMX_BOOL shuffle = ap_prc->playlist_.bShuffle;

    tiz_iheart_set_playback_mode (
      ap_prc->p_iheart_, (shuffle == OMX_TRUE ? ETIZIheartPlaybackModeShuffle
                                              : ETIZIheartPlaybackModeNormal));

    switch (ap_prc->playlist_.ePlaylistType)
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
 * iheartprc
 */

static void *
iheart_prc_ctor (void * ap_obj, va_list * app)
{
  iheart_prc_t * p_prc = super_ctor (typeOf (ap_obj, "iheartprc"), ap_obj, app);
  p_prc->p_outhdr_ = NULL;
  TIZ_INIT_OMX_STRUCT (p_prc->session_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_skip_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_position_);
  p_prc->p_uri_param_ = NULL;
  p_prc->p_trans_ = NULL;
  p_prc->p_iheart_ = NULL;
  p_prc->eos_ = false;
  p_prc->port_disabled_ = false;
  p_prc->uri_changed_ = false;
  p_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
  p_prc->num_channels_ = 2;
  p_prc->samplerate_ = 44100;
  p_prc->auto_detect_on_ = false;
  p_prc->bitrate_ = ARATELIA_HTTP_SOURCE_DEFAULT_BIT_RATE_KBITS;
  p_prc->buffer_bytes_ = ((p_prc->bitrate_ * 1000) / 8)
    * ARATELIA_HTTP_SOURCE_DEFAULT_BUFFER_SECONDS_IHEART;
  p_prc->remove_current_url_ = false;
  p_prc->connection_closed_ = false;
  p_prc->first_buffer_delivered_ = false;
  return p_prc;
}

static void *
iheart_prc_dtor (void * ap_obj)
{
  (void) iheart_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "iheartprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
iheart_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  iheart_prc_t * p_prc = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  assert (p_prc);
  tiz_check_omx (retrieve_session_configuration (p_prc));
  tiz_check_omx (retrieve_playlist (p_prc));
  tiz_check_omx (retrieve_buffer_size (p_prc));
  if (p_prc->buffer_size_.nCapacity)
    {
      p_prc->buffer_bytes_ = ((p_prc->bitrate_ * 1000) / 8)
        * p_prc->buffer_size_.nCapacity;
    }

  on_iheart_error_ret_omx_oom (tiz_iheart_init (
    &(p_prc->p_iheart_)));

  tiz_check_omx (enqueue_playlist_items (p_prc));
  tiz_check_omx (obtain_next_url (p_prc, 1, IGNORE_VALUE));

  {
    const tiz_urltrans_buffer_cbacks_t buffer_cbacks
      = {buffer_filled, buffer_emptied};
    const tiz_urltrans_info_cbacks_t info_cbacks
      = {header_available, data_available, connection_lost};
    const tiz_urltrans_event_io_cbacks_t io_cbacks
      = {tiz_srv_io_watcher_init, tiz_srv_io_watcher_destroy,
         tiz_srv_io_watcher_start, tiz_srv_io_watcher_stop};
    const tiz_urltrans_event_timer_cbacks_t timer_cbacks
      = {tiz_srv_timer_watcher_init, tiz_srv_timer_watcher_destroy,
         tiz_srv_timer_watcher_start, tiz_srv_timer_watcher_stop,
         tiz_srv_timer_watcher_restart};
    rc
      = tiz_urltrans_init (&(p_prc->p_trans_), p_prc, p_prc->p_uri_param_,
                           ARATELIA_HTTP_SOURCE_COMPONENT_NAME,
                           p_prc->buffer_bytes_,
                           ARATELIA_HTTP_SOURCE_DEFAULT_RECONNECT_TIMEOUT,
                           buffer_cbacks, info_cbacks, io_cbacks, timer_cbacks);
    if (OMX_ErrorNone == rc)
      {
        tiz_urltrans_set_connect_timeout(p_prc->p_trans_, 3L);
      }
  }
  return rc;
}

static OMX_ERRORTYPE
iheart_prc_deallocate_resources (void * ap_prc)
{
  iheart_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_urltrans_destroy (p_prc->p_trans_);
  p_prc->p_trans_ = NULL;
  delete_uri (p_prc);
  tiz_iheart_destroy (p_prc->p_iheart_);
  p_prc->p_iheart_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
iheart_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  iheart_prc_t * p_prc = ap_prc;
  assert (ap_prc);
  p_prc->eos_ = false;
  tiz_urltrans_cancel (p_prc->p_trans_);
  tiz_urltrans_set_internal_buffer_size (p_prc->p_trans_, p_prc->buffer_bytes_);
  return prepare_for_port_auto_detection (p_prc);
}

static OMX_ERRORTYPE
iheart_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
{
  iheart_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->auto_detect_on_)
    {
      p_prc->connection_closed_ = false;
      p_prc->first_buffer_delivered_ = false;
      rc = tiz_urltrans_start (p_prc->p_trans_);
    }
  return rc;
}

static OMX_ERRORTYPE
iheart_prc_stop_and_return (void * ap_prc)
{
  iheart_prc_t * p_prc = ap_prc;
  assert (p_prc);
  if (p_prc->p_trans_)
    {
      tiz_urltrans_pause (p_prc->p_trans_);
      tiz_urltrans_flush_buffer (p_prc->p_trans_);
    }
  return release_buffer (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
iheart_prc_buffers_ready (const void * ap_prc)
{
  iheart_prc_t * p_prc = (iheart_prc_t *) ap_prc;
  assert (p_prc);
  return tiz_urltrans_on_buffers_ready (p_prc->p_trans_);
}

static OMX_ERRORTYPE
iheart_prc_io_ready (void * ap_prc, tiz_event_io_t * ap_ev_io, int a_fd,
                     int a_events)
{
  iheart_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return tiz_urltrans_on_io_ready (p_prc->p_trans_, ap_ev_io, a_fd, a_events);
}

static OMX_ERRORTYPE
iheart_prc_timer_ready (void * ap_prc, tiz_event_timer_t * ap_ev_timer,
                        void * ap_arg, const uint32_t a_id)
{
  iheart_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return tiz_urltrans_on_timer_ready (p_prc->p_trans_, ap_ev_timer);
}

static OMX_ERRORTYPE
iheart_prc_pause (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
iheart_prc_resume (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
iheart_prc_port_flush (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  iheart_prc_t * p_prc = (iheart_prc_t *) ap_obj;
  if (p_prc->p_trans_)
    {
      tiz_urltrans_flush_buffer (p_prc->p_trans_);
    }
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
iheart_prc_port_disable (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  iheart_prc_t * p_prc = (iheart_prc_t *) ap_obj;
  assert (p_prc);
  p_prc->port_disabled_ = true;
  if (p_prc->p_trans_)
    {
      tiz_urltrans_pause (p_prc->p_trans_);
      tiz_urltrans_flush_buffer (p_prc->p_trans_);
    }
  /* Release any buffers held  */
  return release_buffer ((iheart_prc_t *) ap_obj);
}

static OMX_ERRORTYPE
iheart_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  iheart_prc_t * p_prc = (iheart_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->port_disabled_)
    {
      p_prc->port_disabled_ = false;
      if (!p_prc->uri_changed_)
        {
          rc = tiz_urltrans_unpause (p_prc->p_trans_);
        }
      else
        {
          p_prc->uri_changed_ = false;
          /* rc = tiz_urltrans_start (p_prc->p_trans_); */
        }
    }
  return rc;
}

static OMX_ERRORTYPE
iheart_prc_config_change (void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid),
                          OMX_INDEXTYPE a_config_idx)
{
  iheart_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_prc);

  if (OMX_TizoniaIndexConfigPlaylistSkip == a_config_idx && p_prc->p_trans_)
    {
      int skip_value = 0;
      TIZ_INIT_OMX_STRUCT (p_prc->playlist_skip_);
      tiz_check_omx (tiz_api_GetConfig (
        tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
        OMX_TizoniaIndexConfigPlaylistSkip, &p_prc->playlist_skip_));

      p_prc->playlist_skip_.nValue > 0 ? (skip_value = 1) : (skip_value = -1);
      tiz_check_omx (obtain_next_url (p_prc, skip_value, IGNORE_VALUE));

      /* Changing the URL has the side effect of halting the current
         download */
      tiz_urltrans_set_uri (p_prc->p_trans_, p_prc->p_uri_param_);
      if (p_prc->port_disabled_)
        {
          /* Record that the URI has changed, so that when the port is
             re-enabled, we restart the transfer */
          p_prc->uri_changed_ = true;
        }

      /* Get ready to auto-detect another stream */
      set_auto_detect_on_port (p_prc);
      prepare_for_port_auto_detection (p_prc);

      /* Re-start the transfer */
      p_prc->connection_closed_ = false;
      p_prc->first_buffer_delivered_ = false;
      tiz_urltrans_start (p_prc->p_trans_);
    }
  else if (OMX_TizoniaIndexConfigPlaylistPosition == a_config_idx
           && p_prc->p_trans_)
    {
      TIZ_INIT_OMX_STRUCT (p_prc->playlist_position_);
      tiz_check_omx (tiz_api_GetConfig (
        tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
        OMX_TizoniaIndexConfigPlaylistPosition, &p_prc->playlist_position_));

      /* Check that the requested position actually refers to a track in the
         queue */
      if (p_prc->playlist_position_.nPosition >= 0
          && p_prc->playlist_position_.nPosition
               <= tiz_iheart_get_current_queue_length_as_int (p_prc->p_iheart_))
        {
          obtain_next_url (p_prc, IGNORE_VALUE,
                           p_prc->playlist_position_.nPosition);

          /* Changing the URL has the side effect of halting the current
             download */
          tiz_urltrans_set_uri (p_prc->p_trans_, p_prc->p_uri_param_);
          if (p_prc->port_disabled_)
            {
              /* Record that the URI has changed, so that when the port is
                 re-enabled, we restart the transfer */
              p_prc->uri_changed_ = true;
            }

          /* Get ready to auto-detect another stream */
          set_auto_detect_on_port (p_prc);
          prepare_for_port_auto_detection (p_prc);

          /* Re-start the transfer */
          p_prc->connection_closed_ = false;
          p_prc->first_buffer_delivered_ = false;
          tiz_urltrans_start (p_prc->p_trans_);
        }
    }
  else if (OMX_TizoniaIndexConfigPlaylistPrintAction == a_config_idx
           && p_prc->p_trans_)
    {
      tiz_iheart_print_queue (p_prc->p_iheart_);
    }

  return rc;
}

/*
 * iheart_prc_class
 */

static void *
iheart_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "iheartprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
iheart_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * iheartprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "iheartprc_class", classOf (tizprc),
     sizeof (iheart_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, iheart_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return iheartprc_class;
}

void *
iheart_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * iheartprc_class = tiz_get_type (ap_hdl, "iheartprc_class");
  TIZ_LOG_CLASS (iheartprc_class);
  void * iheartprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (iheartprc_class, "iheartprc", tizprc, sizeof (iheart_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, iheart_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, iheart_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, iheart_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, iheart_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, iheart_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, iheart_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, iheart_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_io_ready, iheart_prc_io_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_timer_ready, iheart_prc_timer_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, iheart_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, iheart_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, iheart_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, iheart_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, iheart_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, iheart_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_config_change, iheart_prc_config_change,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return iheartprc;
}

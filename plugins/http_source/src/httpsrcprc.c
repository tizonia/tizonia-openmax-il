/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @brief  HTTP streaming client - processor class
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
static OMX_ERRORTYPE release_buffer (httpsrc_prc_t *);
static OMX_ERRORTYPE prepare_for_port_auto_detection (httpsrc_prc_t *ap_prc);

typedef struct ogg_codec_id ogg_codec_id_t;
struct ogg_codec_id
{
  const char *p_bos_str;
  int bos_str_len;
  const char *p_coding_type;
  OMX_S32 omx_coding_type;
};

static const ogg_codec_id_t ogg_codec_type_tbl[]
    = { { "\200theora", 7, "Theora", OMX_AUDIO_CodingUnused },
        { "\001vorbis", 7, "Vorbis", OMX_AUDIO_CodingVORBIS },
        { "Speex", 5, "Speex", OMX_AUDIO_CodingSPEEX },
        { "PCM     ", 8, "PCM", OMX_AUDIO_CodingPCM },
        { "CMML\0\0\0\0", 8, "CMML", OMX_AUDIO_CodingUnused },
        { "Annodex", 7, "Annodex", OMX_AUDIO_CodingUnused },
        { "fishead", 7, "Skeleton", OMX_AUDIO_CodingUnused },
        { "fLaC", 4, "Flac0", OMX_AUDIO_CodingFLAC },
        { "\177FLAC", 5, "Flac", OMX_AUDIO_CodingFLAC },
        { "AnxData", 7, "AnxData", OMX_VIDEO_CodingUnused },
        { "CELT    ", 8, "CELT", OMX_AUDIO_CodingUnused },
        { "\200kate\0\0\0", 8, "Kate", OMX_AUDIO_CodingUnused },
        { "BBCD\0", 5, "Dirac", OMX_AUDIO_CodingUnused },
        { "OpusHead", 8, "Opus", OMX_AUDIO_CodingOPUS },
        { "\x4fVP80", 5, "VP8", OMX_VIDEO_CodingVP8 },
        { "", 0, "Unknown", OMX_AUDIO_CodingUnused } };

static OMX_S32 identify_ogg_codec (httpsrc_prc_t *ap_prc,
                                   unsigned char *ap_data, long a_len)
{
  OMX_S32 rc = OMX_AUDIO_CodingUnused;
  const size_t id_count = sizeof(ogg_codec_type_tbl) / sizeof(ogg_codec_id_t);
  size_t i = 0;

  assert (ap_prc);

  TIZ_TRACE (handleOf (ap_prc), "len [%d] data [%s]", a_len, ap_data);

  for (i = 0; i < id_count; ++i)
    {
      const ogg_codec_id_t *p_id = ogg_codec_type_tbl + i;

      if (a_len >= p_id->bos_str_len
          && memcmp (ap_data + 28, p_id->p_bos_str, p_id->bos_str_len) == 0)
        {
          rc = p_id->omx_coding_type;
          TIZ_TRACE (handleOf (ap_prc), "Identified codec : [%s]",
                     p_id->p_coding_type);
          break;
        }
    }
  TIZ_TRACE (handleOf (ap_prc), "coding type  : [%X]", rc);
  return rc;
}

static inline bool is_valid_character (const char c)
{
  return (unsigned char)c > 0x20;
}

static void obtain_coding_type (httpsrc_prc_t *ap_prc, char *ap_info)
{
  assert (ap_prc);
  assert (ap_info);

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
      /* This is for audio with ogg container (may be FLAC, Vorbis, Opus,
         etc). We'll have to identify the actual codec when the first bytes
         from the stream arrive */
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
  assert (ap_prc);
  assert (ap_start);
  assert (ap_end);

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
  assert (ap_prc);
  assert (ap_info);

  TIZ_TRACE (handleOf (ap_prc), "audio info  : [%s]", ap_info);

  /* Find the number of channels */
  if ((p_value = (const char *)strstr (ap_info, channels)))
    {
      if ((p_start = (const char *)strchr (p_value, '=')))
        {
          /* skip the equal sign */
          p_start++;
          ap_prc->num_channels_ = convert_str_to_int (ap_prc, p_start, &p_end);
        }
    }

  /* Find the sampling rate */
  if ((p_value = (const char *)strstr (ap_info, samplerate)))
    {
      if ((p_start = (const char *)strchr (p_value, '=')))
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

  assert (ap_prc);
  assert (ap_info);

  TIZ_TRACE (handleOf (ap_prc), "bit rate  : [%s]", ap_info);

  ap_prc->bitrate_ = convert_str_to_int (ap_prc, ap_info, &p_end);
}

static OMX_ERRORTYPE set_audio_coding_on_port (httpsrc_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

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
  assert (ap_prc);

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
  assert (ap_prc);

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
  assert (ap_prc);

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
  assert (ap_prc);
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
      case OMX_AUDIO_CodingFLAC:
        {
          /* TODO */
        }
        break;
      case OMX_AUDIO_CodingVORBIS:
        {
          /* TODO */
        }
        break;
      case OMX_AUDIO_CodingOPUS:
        {
          rc = set_opus_audio_info_on_port (ap_prc);
        }
        break;
      case OMX_AUDIO_CodingOGA:
        {
          /* Nothing to do here */
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
  assert (ap_prc);
  assert (ap_prc->bitrate_ > 0);
  ap_prc->cache_bytes_ = ((ap_prc->bitrate_ * 1000) / 8)
                         * ARATELIA_HTTP_SOURCE_DEFAULT_CACHE_SECONDS;
  if (ap_prc->p_trans_)
    {
      httpsrc_trans_set_internal_buffer_size (ap_prc->p_trans_, ap_prc->cache_bytes_);
    }
}

static OMX_ERRORTYPE store_metadata (httpsrc_prc_t *ap_prc,
                                     const char *ap_header_name,
                                     const char *ap_header_info)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_CONFIG_METADATAITEMTYPE *p_meta = NULL;
  size_t metadata_len = 0;
  size_t info_len = 0;

  assert (ap_prc);
  assert (ap_header_name);
  assert (ap_header_info);

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
  assert (ap_prc);
  assert (ap_header);
  {
    const char *p_end = ap_header + a_size;
    const char *p_value = (const char *)memchr (ap_header, ':', a_size);
    char name[64];

    if (p_value && (size_t)(p_value - ap_header) < sizeof(name))
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
}

static void send_port_auto_detect_events (httpsrc_prc_t *ap_prc)
{
  assert (ap_prc);
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

static inline void delete_uri (httpsrc_prc_t *ap_prc)
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_uri_param_);
  ap_prc->p_uri_param_ = NULL;
}

static OMX_ERRORTYPE obtain_uri (httpsrc_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const long pathname_max = PATH_MAX + NAME_MAX;

  assert (ap_prc);
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

static OMX_ERRORTYPE release_buffer (httpsrc_prc_t *ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_outhdr_)
    {
      TIZ_NOTICE (handleOf (ap_prc), "releasing HEADER [%p] nFilledLen [%d]",
                  ap_prc->p_outhdr_, ap_prc->p_outhdr_->nFilledLen);
      tiz_check_omx_err (tiz_krn_release_buffer (
          tiz_get_krn (handleOf (ap_prc)), ARATELIA_HTTP_SOURCE_PORT_INDEX,
          ap_prc->p_outhdr_));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static void buffer_filled (OMX_BUFFERHEADERTYPE *ap_hdr, void *ap_arg)
{
  httpsrc_prc_t *p_prc = ap_arg;
  assert (p_prc);
  assert (ap_hdr);
  assert (p_prc->p_outhdr_ == ap_hdr);
  ap_hdr->nOffset = 0;
  (void)release_buffer (p_prc);
}

static OMX_BUFFERHEADERTYPE *buffer_wanted (OMX_PTR ap_arg)
{
  httpsrc_prc_t *p_prc = ap_arg;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
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
            }
        }
    }
  return p_hdr;
}

static void header_available (OMX_PTR ap_arg, const void *ap_ptr,
                              const size_t a_nbytes)
{
  httpsrc_prc_t *p_prc = ap_arg;
  assert (p_prc);
  assert (ap_ptr);

  if (p_prc->auto_detect_on_)
    {
      obtain_audio_encoding_from_headers (p_prc, ap_ptr, a_nbytes);
    }
}

static bool data_available (OMX_PTR ap_arg, const void *ap_ptr,
                            const size_t a_nbytes)
{
  httpsrc_prc_t *p_prc = ap_arg;
  bool pause_needed = false;
  assert (p_prc);
  assert (ap_ptr);

  if (p_prc->auto_detect_on_ && a_nbytes > 0)
    {
      p_prc->auto_detect_on_ = false;

      /* This will pause the http transfer */
      pause_needed = true;

      if (OMX_AUDIO_CodingOGA == p_prc->audio_coding_type_)
        {
          /* Try to identify the actual codec from the ogg stream */
          p_prc->audio_coding_type_
            = identify_ogg_codec (p_prc, (unsigned char *)ap_ptr, a_nbytes);
          if (OMX_AUDIO_CodingUnused != p_prc->audio_coding_type_)
            {
              set_audio_coding_on_port (p_prc);
              set_audio_info_on_port (p_prc);
            }
        }
      /* And now trigger the OMX_EventPortFormatDetected and
         OMX_EventPortSettingsChanged events or a
         OMX_ErrorFormatNotDetected event */
      send_port_auto_detect_events (p_prc);
    }
  return pause_needed;
}

static bool connection_lost (OMX_PTR ap_arg)
{
  httpsrc_prc_t *p_prc = ap_arg;
  assert (p_prc);
  prepare_for_port_auto_detection (p_prc);
  /* Return true to indicate that the automatic reconnection procedure needs to
     be started */
  return true;
}

static OMX_ERRORTYPE prepare_for_port_auto_detection (httpsrc_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

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
  p_prc->bitrate_ = ARATELIA_HTTP_SOURCE_DEFAULT_BIT_RATE_KBITS;
  update_cache_size (p_prc);
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
  assert (p_prc);
  assert (NULL == p_prc->p_uri_param_);
  tiz_check_omx_err (obtain_uri (p_prc));
  return httpsrc_trans_init (&(p_prc->p_trans_), p_prc,
                             p_prc->p_uri_param_,
                             buffer_filled, buffer_wanted,
                             header_available,
                             data_available,
                             connection_lost);
}

static OMX_ERRORTYPE httpsrc_prc_deallocate_resources (void *ap_prc)
{
  httpsrc_prc_t *p_prc = ap_prc;
  assert (p_prc);
  httpsrc_trans_destroy (p_prc->p_trans_);
  p_prc->p_trans_ = NULL;
  delete_uri (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE httpsrc_prc_prepare_to_transfer (void *ap_prc,
                                                      OMX_U32 a_pid)
{
  httpsrc_prc_t *p_prc = ap_prc;
  assert (ap_prc);
  p_prc->eos_ = false;
  httpsrc_trans_cancel (p_prc->p_trans_);
  httpsrc_trans_set_internal_buffer_size (p_prc->p_trans_, p_prc->cache_bytes_);
  return prepare_for_port_auto_detection (p_prc);
}

static OMX_ERRORTYPE httpsrc_prc_transfer_and_process (void *ap_prc,
                                                       OMX_U32 a_pid)
{
  httpsrc_prc_t *p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->auto_detect_on_)
    {
      rc = httpsrc_trans_start (p_prc->p_trans_);
    }
  return rc;
}

static OMX_ERRORTYPE httpsrc_prc_stop_and_return (void *ap_prc)
{
  httpsrc_prc_t *p_prc = ap_prc;
  assert (p_prc);
  if (p_prc->p_trans_)
    {
      httpsrc_trans_pause (p_prc->p_trans_);
      httpsrc_trans_flush_buffer (p_prc->p_trans_);
    }
  return release_buffer (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE httpsrc_prc_buffers_ready (const void *ap_prc)
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_prc;
  assert (p_prc);
  return httpsrc_trans_on_buffers_ready (p_prc->p_trans_);
}

static OMX_ERRORTYPE httpsrc_prc_io_ready (void *ap_prc,
                                           tiz_event_io_t *ap_ev_io, int a_fd,
                                           int a_events)
{
  httpsrc_prc_t *p_prc = ap_prc;
  assert (p_prc);
  return httpsrc_trans_on_io_ready (p_prc->p_trans_, ap_ev_io, a_fd, a_events);
}

static OMX_ERRORTYPE httpsrc_prc_timer_ready (void *ap_prc,
                                              tiz_event_timer_t *ap_ev_timer,
                                              void *ap_arg, const uint32_t a_id)
{
  httpsrc_prc_t *p_prc = ap_prc;
  assert (p_prc);
  return httpsrc_trans_on_timer_ready (p_prc->p_trans_, ap_ev_timer);
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
  if (p_prc->p_trans_)
    {
      httpsrc_trans_flush_buffer (p_prc->p_trans_);
    }
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE httpsrc_prc_port_disable (const void *ap_obj,
                                               OMX_U32 TIZ_UNUSED (a_pid))
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_obj;
  assert (p_prc);
  p_prc->port_disabled_ = true;
  if (p_prc->p_trans_)
    {
      httpsrc_trans_pause (p_prc->p_trans_);
      httpsrc_trans_flush_buffer (p_prc->p_trans_);
    }
  /* Release any buffers held  */
  return release_buffer ((httpsrc_prc_t *)ap_obj);
}

static OMX_ERRORTYPE httpsrc_prc_port_enable (const void *ap_prc, OMX_U32 a_pid)
{
  httpsrc_prc_t *p_prc = (httpsrc_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  TIZ_NOTICE (handleOf (p_prc), "Enabling port [%d] was disabled? [%s]", a_pid,
              p_prc->port_disabled_ ? "YES" : "NO");
  if (p_prc->port_disabled_)
    {
      p_prc->port_disabled_ = false;
      rc = httpsrc_trans_unpause (p_prc->p_trans_);
    }
  return rc;
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
       tiz_srv_io_ready, httpsrc_prc_io_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_srv_timer_ready, httpsrc_prc_timer_ready,
       /* TIZ_CLASS_COMMENT: */
       tiz_prc_buffers_ready, httpsrc_prc_buffers_ready,
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

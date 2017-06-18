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
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   deezerprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Deezer streaming client - processor class
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
#include "deezerprc.h"
#include "deezerprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_source.prc.deezer"
#endif

/* forward declarations */
static OMX_ERRORTYPE
deezer_prc_deallocate_resources (void *);
static OMX_ERRORTYPE
release_buffer (deezer_prc_t *);
static OMX_ERRORTYPE
prepare_for_port_auto_detection (deezer_prc_t * ap_prc);
static OMX_ERRORTYPE
deezer_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid);
static OMX_ERRORTYPE
deezer_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid);

#define on_deezer_error_ret_omx_oom(expr)                                    \
  do                                                                         \
    {                                                                        \
      int deezer_error = 0;                                                  \
      if (0 != (deezer_error = (expr)))                                      \
        {                                                                    \
          TIZ_ERROR (handleOf (p_prc),                                       \
                     "[OMX_ErrorInsufficientResources] : error while using " \
                     "libtizdeezer [error %d]",                              \
                     deezer_error);                                          \
          return OMX_ErrorInsufficientResources;                             \
        }                                                                    \
    }                                                                        \
  while (0)

/* static inline bool */
/* is_valid_character (const char c) */
/* { */
/*   return (unsigned char) c > 0x20; */
/* } */

/* static OMX_ERRORTYPE */
/* obtain_coding_type (deezer_prc_t * ap_prc, char * ap_info) */
/* { */
/*   OMX_ERRORTYPE rc = OMX_ErrorNone; */
/*   assert (ap_prc); */
/*   assert (ap_info); */
/*   if (memcmp (ap_info, "audio/webm", 10) == 0) */
/*     { */
/*       /\* The webm container *\/ */
/*       ap_prc->audio_coding_type_ = OMX_AUDIO_CodingWEBM; */
/*     } */
/*   else if (memcmp (ap_info, "audio/mp4", 9) == 0) */
/*     { */
/*       /\* This is .mp4 .m4a  *\/ */
/*       ap_prc->audio_coding_type_ = OMX_AUDIO_CodingMP4; */
/*     } */
/*   else */
/*     { */
/*       ap_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused; */
/*       rc = OMX_ErrorInsufficientResources; */
/*     } */
/*   TIZ_TRACE (handleOf (ap_prc), "encoding type  : [%s] - [%s]", ap_info, */
/*              tiz_audio_coding_to_str (ap_prc->audio_coding_type_)); */
/*   return rc; */
/* } */

/* static int */
/* convert_str_to_int (deezer_prc_t * ap_prc, const char * ap_start, */
/*                     char ** ap_end) */
/* { */
/*   long val = -1; */
/*   assert (ap_prc); */
/*   assert (ap_start); */
/*   assert (ap_end); */

/*   errno = 0; */
/*   val = strtol (ap_start, ap_end, 0); */

/*   if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) */
/*       || (errno != 0 && val == 0)) */
/*     { */
/*       TIZ_ERROR (handleOf (ap_prc), */
/*                  "Error retrieving the number of channels : [%s]", */
/*                  strerror (errno)); */
/*     } */
/*   else if (*ap_end == ap_start) */
/*     { */
/*       TIZ_ERROR (handleOf (ap_prc), */
/*                  "Error retrieving the number of channels : " */
/*                  "[No digits were found]"); */
/*     } */
/*   return val; */
/* } */

/* static void */
/* obtain_content_length (deezer_prc_t * ap_prc, char * ap_info) */
/* { */
/*   char * p_end = NULL; */

/*   assert (ap_prc); */
/*   assert (ap_info); */
/*   ap_prc->content_length_bytes_ = convert_str_to_int (ap_prc, ap_info, &p_end); */
/*   ap_prc->bytes_before_eos_ = ap_prc->content_length_bytes_; */
/* } */

/* static OMX_ERRORTYPE */
/* set_audio_coding_on_port (deezer_prc_t * ap_prc) */
/* { */
/*   OMX_PARAM_PORTDEFINITIONTYPE port_def; */
/*   assert (ap_prc); */

/*   TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_HTTP_SOURCE_PORT_INDEX); */
/*   tiz_check_omx ( */
/*     tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc), */
/*                           OMX_IndexParamPortDefinition, &port_def)); */

/*   /\* Set the new value *\/ */
/*   port_def.format.audio.eEncoding = ap_prc->audio_coding_type_; */

/*   tiz_check_omx (tiz_krn_SetParameter_internal ( */
/*     tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc), */
/*     OMX_IndexParamPortDefinition, &port_def)); */
/*   TIZ_DEBUG (handleOf (ap_prc), "audio_coding_type_ [%s]", */
/*              tiz_audio_coding_to_str (ap_prc->audio_coding_type_)); */
/*   return OMX_ErrorNone; */
/* } */

static OMX_ERRORTYPE
set_auto_detect_on_port (deezer_prc_t * ap_prc)
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
store_metadata (deezer_prc_t * ap_prc, const char * ap_header_name,
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

/* static void */
/* obtain_audio_encoding_from_headers (deezer_prc_t * ap_prc, */
/*                                     const char * ap_header, const size_t a_size) */
/* { */
/*   assert (ap_prc); */
/*   assert (ap_header); */
/*   { */
/*     const char * p_end = ap_header + a_size; */
/*     const char * p_value = (const char *) memchr (ap_header, ':', a_size); */
/*     char name[64]; */

/*     if (p_value && (size_t) (p_value - ap_header) < sizeof (name)) */
/*       { */
/*         memcpy (name, ap_header, p_value - ap_header); */
/*         name[p_value - ap_header] = 0; */

/*         /\* skip the colon *\/ */
/*         ++p_value; */

/*         /\* strip the value *\/ */
/*         while (p_value < p_end && !is_valid_character (*p_value)) */
/*           { */
/*             ++p_value; */
/*           } */

/*         while (p_end > p_value && !is_valid_character (p_end[-1])) */
/*           { */
/*             --p_end; */
/*           } */

/*         { */
/*           char * p_info = tiz_mem_calloc (1, (p_end - p_value) + 1); */
/*           memcpy (p_info, p_value, p_end - p_value); */
/*           p_info[(p_end - p_value)] = '\000'; */
/*           TIZ_TRACE (handleOf (ap_prc), "header name  : [%s]", name); */
/*           TIZ_TRACE (handleOf (ap_prc), "header value : [%s]", p_info); */

/*           if (memcmp (name, "Content-Type", 12) == 0 */
/*               || memcmp (name, "content-type", 12) == 0) */
/*             { */
/*               if (OMX_ErrorNone == obtain_coding_type (ap_prc, p_info)) */
/*                 { */
/*                   /\* Now set the new coding type value on the output port *\/ */
/*                   (void) set_audio_coding_on_port (ap_prc); */
/*                 } */
/*             } */
/*           else if (memcmp (name, "Content-Length", 14) == 0) */
/*             { */
/*               obtain_content_length (ap_prc, p_info); */
/*             } */
/*           tiz_mem_free (p_info); */
/*         } */
/*       } */
/*   } */
/* } */

/* static void */
/* send_port_auto_detect_events (deezer_prc_t * ap_prc) */
/* { */
/*   assert (ap_prc); */
/*   if (ap_prc->audio_coding_type_ != OMX_AUDIO_CodingUnused */
/*       && ap_prc->audio_coding_type_ != OMX_AUDIO_CodingAutoDetect) */
/*     { */
/*       TIZ_DEBUG ( */
/*         handleOf (ap_prc), */
/*         "Issuing OMX_EventPortFormatDetected - audio_coding_type_ [%s]", */
/*         tiz_audio_coding_to_str (ap_prc->audio_coding_type_)); */
/*       tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortFormatDetected, 0, 0, */
/*                            NULL); */
/*       TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_EventPortSettingsChanged"); */
/*       tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortSettingsChanged, */
/*                            ARATELIA_HTTP_SOURCE_PORT_INDEX, /\* port 0 *\/ */
/*                            OMX_IndexParamPortDefinition,    /\* the index of the */
/*                                                          struct that has */
/*                                                          been modififed *\/ */
/*                            NULL); */
/*     } */
/*   else */
/*     { */
/*       /\* Oops... could not detect the stream format *\/ */

/*       /\* Get ready to auto-detect another stream *\/ */
/*       set_auto_detect_on_port (ap_prc); */
/*       prepare_for_port_auto_detection (ap_prc); */

/*       /\* Finally, signal the client *\/ */
/*       TIZ_DEBUG (handleOf (ap_prc), "Issuing OMX_ErrorFormatNotDetected"); */
/*       tiz_srv_issue_err_event ((OMX_PTR) ap_prc, OMX_ErrorFormatNotDetected); */
/*     } */
/* } */

static OMX_ERRORTYPE
update_metadata (deezer_prc_t * ap_prc)
{
  assert (ap_prc);

  /* Clear previous metadata items */
  tiz_krn_clear_metadata (tiz_get_krn (handleOf (ap_prc)));

  /* author and title  */
  tiz_check_omx (store_metadata (
    ap_prc, tiz_deezer_get_current_track_artist (ap_prc->p_deezer_),
    tiz_deezer_get_current_track_title (ap_prc->p_deezer_)));

  /* Album */
  tiz_check_omx (store_metadata (
    ap_prc, "Album", tiz_deezer_get_current_track_album (ap_prc->p_deezer_)));

  /* Duration */
  tiz_check_omx (store_metadata (
    ap_prc, "Duration", tiz_deezer_get_current_track_duration (ap_prc->p_deezer_)));

  /* File Size */
  tiz_check_omx (store_metadata (
    ap_prc, "File Size", tiz_deezer_get_current_track_file_size (ap_prc->p_deezer_)));

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
obtain_next_track (deezer_prc_t * ap_prc, int a_skip_value)
{
  deezer_prc_t * p_prc = ap_prc;

  assert (ap_prc);
  assert (ap_prc->p_deezer_);

  on_deezer_error_ret_omx_oom (tiz_deezer_next_track (ap_prc->p_deezer_));

  /* Song metadata is now available, update the IL client */
  return update_metadata (ap_prc);
}

static OMX_ERRORTYPE
release_buffer (deezer_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->p_outhdr_)
    {
      if (ap_prc->bytes_before_eos_ > ap_prc->p_outhdr_->nFilledLen)
        {
          ap_prc->bytes_before_eos_ -= ap_prc->p_outhdr_->nFilledLen;
        }
      else
        {
          ap_prc->bytes_before_eos_ = 0;
          ap_prc->eos_ = true;
        }

      if (ap_prc->eos_)
        {
          ap_prc->eos_ = false;
          ap_prc->p_outhdr_->nFlags |= OMX_BUFFERFLAG_EOS;
        }
      tiz_check_omx (tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)),
                                             ARATELIA_HTTP_SOURCE_PORT_INDEX,
                                             ap_prc->p_outhdr_));
      ap_prc->p_outhdr_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
deliver_buffer (deezer_prc_t * p_prc)
{
  assert (p_prc);
  assert (p_prc->p_outhdr_);

  if (p_prc->deezer_data_len_)
    {
      OMX_BUFFERHEADERTYPE * p_hdr = p_prc->p_outhdr_;
      unsigned char * p_dst = p_hdr->pBuffer + p_hdr->nOffset;
      size_t dst_capacity = p_hdr->nAllocLen - p_hdr->nFilledLen;
      const unsigned char * p_data = p_prc->p_deezer_data_ + p_prc->deezer_data_offset_;
      size_t deezer_len = p_prc->deezer_data_len_ - p_prc->deezer_data_offset_;
      size_t len = MIN(dst_capacity, deezer_len);

      assert (p_data);
      assert (len);

      memcpy(p_dst, p_data, len);

      /* Update deezer data pointers */
      p_prc->deezer_data_offset_ += len;
      if (p_prc->deezer_data_offset_ == p_prc->deezer_data_len_)
        {
          p_prc->p_deezer_data_ = NULL;
          p_prc->deezer_data_offset_ = 0;
          p_prc->deezer_data_len_= 0;
        }

      /* Update omx buffer pointers */
      p_hdr->nFilledLen += len;
      p_hdr->nOffset += len;
      if ((p_hdr->nAllocLen == p_hdr->nFilledLen) || p_prc->eos_)
        {
          p_hdr->nOffset = 0;
          tiz_check_omx (release_buffer (p_prc));
        }
    }
  return OMX_ErrorNone;
}

static OMX_BUFFERHEADERTYPE *
obtain_buffer (OMX_PTR ap_arg)
{
  deezer_prc_t * p_prc = ap_arg;
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

/* static void */
/* header_available (OMX_PTR ap_arg, const void * ap_ptr, const size_t a_nbytes) */
/* { */
/*   deezer_prc_t * p_prc = ap_arg; */
/*   assert (p_prc); */
/*   assert (ap_ptr); */
/*   obtain_audio_encoding_from_headers (p_prc, ap_ptr, a_nbytes); */
/* } */

/* static bool */
/* data_available (OMX_PTR ap_arg, const void * ap_ptr, const size_t a_nbytes) */
/* { */
/*   deezer_prc_t * p_prc = ap_arg; */
/*   bool pause_needed = false; */
/*   assert (p_prc); */
/*   assert (ap_ptr); */

/*   TIZ_DEBUG (handleOf (p_prc), "p_prc->auto_detect_on_ [%s]", */
/*              (p_prc->auto_detect_on_ ? "TRUE" : "FALSE")); */

/*   if (p_prc->auto_detect_on_ && a_nbytes > 0) */
/*     { */
/*       p_prc->auto_detect_on_ = false; */

/*       /\* This will pause the http transfer *\/ */
/*       pause_needed = true; */

/*       /\* And now trigger the OMX_EventPortFormatDetected and */
/*          OMX_EventPortSettingsChanged events or a */
/*          OMX_ErrorFormatNotDetected event *\/ */
/*       send_port_auto_detect_events (p_prc); */
/*     } */
/*   return pause_needed; */
/* } */

/* static bool */
/* connection_lost (OMX_PTR ap_arg) */
/* { */
/*   deezer_prc_t * p_prc = ap_arg; */
/*   assert (p_prc); */
/*   TIZ_PRINTF_DBG_RED ("connection_lost - bytes_before_eos_ [%d]\n", */
/*                       p_prc->bytes_before_eos_); */

/*   if (p_prc->auto_detect_on_) */
/*     { */
/*       /\* Oops... unable to connect to the station *\/ */

/*       /\* Get ready to auto-detect another stream *\/ */
/*       set_auto_detect_on_port (p_prc); */
/*       prepare_for_port_auto_detection (p_prc); */

/*       /\* Signal the client *\/ */
/*       tiz_srv_issue_err_event ((OMX_PTR) p_prc, OMX_ErrorFormatNotDetected); */
/*     } */

/*   /\* Return false to indicate that there is no need to start the automatic */
/*      reconnection procedure *\/ */
/*   return false; */
/* } */

static OMX_ERRORTYPE
prepare_for_port_auto_detection (deezer_prc_t * ap_prc)
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

  TIZ_TRACE (handleOf (ap_prc),
             "auto_detect_on_ [%s]...audio_coding_type_ [%s]",
             ap_prc->auto_detect_on_ ? "true" : "false",
             tiz_audio_coding_to_str (ap_prc->audio_coding_type_));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
retrieve_session_configuration (deezer_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioDeezerSession, &(ap_prc->session_));
}

static OMX_ERRORTYPE
retrieve_playlist (deezer_prc_t * ap_prc)
{
  return tiz_api_GetParameter (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_TizoniaIndexParamAudioDeezerPlaylist, &(ap_prc->playlist_));
}

static OMX_ERRORTYPE
enqueue_playlist_items (deezer_prc_t * ap_prc)
{
  int rc = 1;

  assert (ap_prc);
  assert (ap_prc->p_deezer_);

  {
    const char * p_playlist = (const char *) ap_prc->playlist_.cPlaylistName;
    const OMX_BOOL shuffle = ap_prc->playlist_.bShuffle;

    tiz_deezer_set_playback_mode (
      ap_prc->p_deezer_, (shuffle == OMX_TRUE ? ETIZDeezerPlaybackModeShuffle
                                              : ETIZDeezerPlaybackModeNormal));

    switch (ap_prc->playlist_.ePlaylistType)
      {
        case OMX_AUDIO_DeezerPlaylistTypeUnknown:
          {
            /* TODO */
            assert (0);
          }
          break;
        case OMX_AUDIO_DeezerPlaylistTypeTracks:
          {
            rc = tiz_deezer_play_tracks (ap_prc->p_deezer_, p_playlist);
          }
          break;
        case OMX_AUDIO_DeezerPlaylistTypeAlbum:
          {
            rc = tiz_deezer_play_album (ap_prc->p_deezer_, p_playlist);
          }
          break;
        case OMX_AUDIO_DeezerPlaylistTypeArtist:
          {
            rc = tiz_deezer_play_artist (ap_prc->p_deezer_, p_playlist);
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
 * deezerprc
 */

static void *
deezer_prc_ctor (void * ap_obj, va_list * app)
{
  deezer_prc_t * p_prc = super_ctor (typeOf (ap_obj, "deezerprc"), ap_obj, app);
  p_prc->p_outhdr_ = NULL;
  TIZ_INIT_OMX_STRUCT (p_prc->session_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_);
  TIZ_INIT_OMX_STRUCT (p_prc->playlist_skip_);
  p_prc->p_deezer_ = NULL;
  p_prc->p_deezer_data_ = NULL;
  p_prc->deezer_data_len_ = 0;
  p_prc->deezer_data_offset_ = 0;
  p_prc->eos_ = false;
  p_prc->port_disabled_ = false;
  p_prc->uri_changed_ = false;
  p_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
  p_prc->num_channels_ = 2;
  p_prc->samplerate_ = 44100;
  p_prc->content_length_bytes_ = 0;
  p_prc->bytes_before_eos_ = 0;
  p_prc->auto_detect_on_ = false;
  return p_prc;
}

static void *
deezer_prc_dtor (void * ap_obj)
{
  (void) deezer_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "deezerprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
deezer_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  deezer_prc_t * p_prc = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  assert (p_prc);
  tiz_check_omx (retrieve_session_configuration (p_prc));
  tiz_check_omx (retrieve_playlist (p_prc));

  on_deezer_error_ret_omx_oom (tiz_deezer_init (
    &(p_prc->p_deezer_), (const char *) p_prc->session_.cUserName));

  tiz_check_omx (enqueue_playlist_items (p_prc));
  tiz_check_omx (obtain_next_track (p_prc, 1));

  return rc;
}

static OMX_ERRORTYPE
deezer_prc_deallocate_resources (void * ap_prc)
{
  deezer_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_deezer_destroy (p_prc->p_deezer_);
  p_prc->p_deezer_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
deezer_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  deezer_prc_t * p_prc = ap_prc;
  assert (ap_prc);
  p_prc->eos_ = false;
  return prepare_for_port_auto_detection (p_prc);
}

static OMX_ERRORTYPE
deezer_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
{
  deezer_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  return rc;
}

static OMX_ERRORTYPE
deezer_prc_stop_and_return (void * ap_prc)
{
  deezer_prc_t * p_prc = ap_prc;
  assert (p_prc);
  return release_buffer (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
deezer_prc_buffers_ready (const void * ap_prc)
{
  deezer_prc_t * p_prc = (deezer_prc_t *) ap_prc;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  assert (p_prc);

  while ((p_hdr = obtain_buffer (p_prc)))
    {
      if (0 == p_prc->deezer_data_len_)
        {
          assert (!p_prc->p_deezer_data_);
          p_prc->deezer_data_len_
            = tiz_deezer_get_mp3_data (p_prc->p_deezer_, &p_prc->p_deezer_data_);
        }
      tiz_check_omx (deliver_buffer (p_prc));
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
deezer_prc_pause (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
deezer_prc_resume (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
deezer_prc_port_flush (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  deezer_prc_t * p_prc = (deezer_prc_t *) ap_obj;
  return release_buffer (p_prc);
}

static OMX_ERRORTYPE
deezer_prc_port_disable (const void * ap_obj, OMX_U32 TIZ_UNUSED (a_pid))
{
  deezer_prc_t * p_prc = (deezer_prc_t *) ap_obj;
  assert (p_prc);
  p_prc->port_disabled_ = true;
  /* Release any buffers held  */
  return release_buffer ((deezer_prc_t *) ap_obj);
}

static OMX_ERRORTYPE
deezer_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  deezer_prc_t * p_prc = (deezer_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (p_prc);
  if (p_prc->port_disabled_)
    {
      p_prc->port_disabled_ = false;
      if (!p_prc->uri_changed_)
        {
          rc = OMX_ErrorNone;
        }
      else
        {
          p_prc->uri_changed_ = false;
        }
    }
  return rc;
}

static OMX_ERRORTYPE
deezer_prc_config_change (void * ap_prc, OMX_U32 TIZ_UNUSED (a_pid),
                          OMX_INDEXTYPE a_config_idx)
{
  deezer_prc_t * p_prc = ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_prc);

  if (OMX_TizoniaIndexConfigPlaylistSkip == a_config_idx)
    {
      TIZ_INIT_OMX_STRUCT (p_prc->playlist_skip_);
      tiz_check_omx (tiz_api_GetConfig (
        tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
        OMX_TizoniaIndexConfigPlaylistSkip, &p_prc->playlist_skip_));
      p_prc->playlist_skip_.nValue > 0 ? obtain_next_track (p_prc, 1)
                                       : obtain_next_track (p_prc, -1);
      if (p_prc->port_disabled_)
        {
          /* Record that the URI has changed, so that when the port is
             re-enabled, we restart the transfer */
          p_prc->uri_changed_ = true;
        }

      /* Get ready to auto-detect another stream */
      set_auto_detect_on_port (p_prc);
      prepare_for_port_auto_detection (p_prc);
    }
  return rc;
}

/*
 * deezer_prc_class
 */

static void *
deezer_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "deezerprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
deezer_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * deezerprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "deezerprc_class", classOf (tizprc),
     sizeof (deezer_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, deezer_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return deezerprc_class;
}

void *
deezer_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * deezerprc_class = tiz_get_type (ap_hdl, "deezerprc_class");
  TIZ_LOG_CLASS (deezerprc_class);
  void * deezerprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (deezerprc_class, "deezerprc", tizprc, sizeof (deezer_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, deezer_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, deezer_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, deezer_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, deezer_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, deezer_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, deezer_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, deezer_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, deezer_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, deezer_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, deezer_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, deezer_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, deezer_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, deezer_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_config_change, deezer_prc_config_change,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return deezerprc;
}

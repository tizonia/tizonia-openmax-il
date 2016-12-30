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
 * @file   webmdmuxfltprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - WebM demuxer filter processor
 *
 * TODO: Support for video demuxing (VP8/VP9 demuxing no handled yet)
 * TODO: Finalise support for audio demuxer (VORBIS not handled yet, only OPUS demuxing)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <alloca.h>

#include <assert.h>
#include <string.h>

#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "webmdmux.h"
#include "webmdmuxfltprc.h"
#include "webmdmuxfltprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.webm_demuxer.filter.prc"
#endif

static OMX_HANDLETYPE g_handle = NULL;

/* Forward declarations */
static OMX_ERRORTYPE
webmdmuxflt_prc_deallocate_resources (void *);
static OMX_ERRORTYPE
store_data (webmdmuxflt_prc_t * ap_prc);

#define on_nestegg_error_ret_omx_oom(expr)                            \
  do                                                                  \
    {                                                                 \
      int nestegg_error = 0;                                          \
      if (0 != (nestegg_error = (expr)))                              \
        {                                                             \
          TIZ_ERROR (handleOf (ap_prc),                               \
                     "[OMX_ErrorInsufficientResources] : while using" \
                     "libnestegg");                                   \
          return OMX_ErrorInsufficientResources;                      \
        }                                                             \
    }                                                                 \
  while (0)

#define WEBMDMUX_LOG_STATE(ap_prc)                                          \
  do                                                                        \
    {                                                                       \
      TIZ_DEBUG (                                                           \
        handleOf (ap_prc),                                                  \
        "store [%d] eos [%s] last read len ? [%d] ne read err [%d] out "    \
        "headers [%s]",                                                     \
        tiz_buffer_available (ap_prc->p_webm_store_),                       \
        (tiz_filter_prc_is_eos (ap_prc) ? "YES" : "NO"),                    \
        ap_prc->ne_last_read_len_, ap_prc->ne_read_err_,                    \
        (tiz_filter_prc_output_headers_available (ap_prc) ? "YES" : "NO")); \
    }                                                                       \
  while (0)

static inline OMX_BUFFERHEADERTYPE *
get_aud_hdr (webmdmuxflt_prc_t * ap_prc)
{
  assert (ap_prc);
  return tiz_filter_prc_get_header (ap_prc,
                                    ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);
}

static inline OMX_BUFFERHEADERTYPE *
get_vid_hdr (webmdmuxflt_prc_t * ap_prc)
{
  assert (ap_prc);
  return tiz_filter_prc_get_header (ap_prc,
                                    ARATELIA_WEBM_DEMUXER_FILTER_PORT_2_INDEX);
}

static inline OMX_BUFFERHEADERTYPE *
get_webm_hdr (webmdmuxflt_prc_t * ap_prc)
{
  assert (ap_prc);
  return tiz_filter_prc_get_header (ap_prc,
                                    ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX);
}

/** User supplied read callback.

    @param buffer   Buffer to read data into.
    @param length   Length of supplied buffer in bytes.
    @param userdata The #userdata supplied by the user.

    @retval  1 Read succeeded.
    @retval  0 End of stream.
    @retval -1 Error.
    */
static int
ne_io_read (void * ap_buffer, size_t a_length, void * a_userdata)
{
  webmdmuxflt_prc_t * p_prc = a_userdata;
  int retval = -1;

  assert (p_prc);
  p_prc->ne_last_read_len_ = a_length;

  WEBMDMUX_LOG_STATE (p_prc);

  if (tiz_filter_prc_is_eos (p_prc)
      && tiz_buffer_available (p_prc->p_webm_store_) == 0)
    {
      return 0;
    }

  (void) store_data (p_prc);

  if (ap_buffer && a_length > 0)
    {
      if (tiz_buffer_available (p_prc->p_webm_store_) >= a_length)
        {
          memcpy (ap_buffer, tiz_buffer_get (p_prc->p_webm_store_), a_length);
          tiz_buffer_advance (p_prc->p_webm_store_, a_length);
          retval = 1;
        }
      else
        {
          TIZ_TRACE (handleOf (p_prc), "out of compressed data");
          retval = -1;
        }
    }

  return retval;
}

/** User supplied seek callback.

    @param offset   Offset within the stream to seek to.
    @param whence   Seek direction.  One of #NESTEGG_SEEK_SET,
                    #NESTEGG_SEEK_CUR, or #NESTEGG_SEEK_END.
    @param userdata The #userdata supplied by the user.

    @retval  0 Seek succeeded.
    @retval -1 Error.
    */
static int
ne_io_seek (int64_t offset, int whence, void * userdata)
{
  webmdmuxflt_prc_t * p_prc = userdata;
  int tb_whence = TIZ_BUFFER_SEEK_SET;
  assert (p_prc);
  TIZ_DEBUG (handleOf (userdata), "offset %lld - whence %d", offset, whence);
  switch (whence)
    {
      case NESTEGG_SEEK_SET:
        {
          tb_whence = TIZ_BUFFER_SEEK_SET;
        }
        break;
      case NESTEGG_SEEK_CUR:
        {
          tb_whence = TIZ_BUFFER_SEEK_CUR;
        }
        break;
      case NESTEGG_SEEK_END:
        {
          tb_whence = TIZ_BUFFER_SEEK_END;
        }
        break;
      default:
        {
          assert (0);
        }
        break;
    };
  return tiz_buffer_seek (p_prc->p_webm_store_, offset, tb_whence);
}

/** User supplied tell callback.

    @param userdata The #userdata supplied by the user.
    @returns Current position within the stream.
    @retval -1 Error.
*/
static int64_t
ne_io_tell (void * userdata)
{
  webmdmuxflt_prc_t * p_prc = userdata;
  assert (p_prc);
  return tiz_buffer_offset (p_prc->p_webm_store_);
}

/** nestegg logging callback function. */
static void
ne_log (nestegg * ctx, unsigned int severity, char const * fmt, ...)
{
  if (g_handle)
    {
      va_list ap;
      char * p_buffer = alloca (4096);

      va_start (ap, fmt);
      vsprintf (p_buffer, fmt, ap);
      va_end (ap);

      switch (severity)
        {
          case NESTEGG_LOG_DEBUG:
            {
              TIZ_DEBUG (g_handle, "%s", p_buffer);
            }
            break;
          case NESTEGG_LOG_WARNING:
            {
              TIZ_WARN (g_handle, "%s", p_buffer);
            }
            break;
          case NESTEGG_LOG_CRITICAL:
            {
              TIZ_ERROR (g_handle, "%s", p_buffer);
            }
            break;
          default:
            {
              TIZ_NOTICE (g_handle, "%s", p_buffer);
            }
            break;
        };
    }
}

static void
print_audio_codec_metadata (webmdmuxflt_prc_t * ap_prc,
                            const unsigned int a_header_idx,
                            const unsigned int a_nheaders,
                            unsigned char * ap_codec_data,
                            size_t codec_data_length)
{
  size_t k = 0;
  assert (ap_prc);
  assert (ap_codec_data);
  TIZ_DEBUG (handleOf (ap_prc), " Audio header [%u] headers [%u] (%p, %u)",
             a_header_idx, a_nheaders, ap_codec_data,
             (unsigned int) codec_data_length);
  for (k = 0; k < codec_data_length; ++k)
    {
      TIZ_DEBUG (handleOf (ap_prc), "   [%c]", ap_codec_data[k]);
    }
}

static void
propagate_eos_if_required (webmdmuxflt_prc_t * ap_prc,
                           OMX_BUFFERHEADERTYPE * ap_out_hdr)
{
  assert (ap_prc);
  assert (ap_out_hdr);

  WEBMDMUX_LOG_STATE (ap_prc);

  /* If EOS, propagate the flag to the next component */
  if (tiz_filter_prc_is_eos (ap_prc)
      && tiz_buffer_available (ap_prc->p_webm_store_) == 0)
    {
      ap_out_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
      tiz_filter_prc_update_eos_flag (ap_prc, false);
    }
}

/* TODO: move this functionality to tiz_filter_prc_t */
static OMX_ERRORTYPE
release_input_header (webmdmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE * p_hdr = get_webm_hdr (ap_prc);

  assert (ap_prc);

  if (p_hdr)
    {
      TIZ_DEBUG (handleOf (ap_prc), "[%p] nFlags [%d]", p_hdr, p_hdr->nFlags);

      if ((p_hdr->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_hdr->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
        }
      rc = tiz_filter_prc_release_header (
        ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX);
    }
  return rc;
}

static OMX_ERRORTYPE
release_output_header (webmdmuxflt_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  if (tiz_filter_prc_is_port_enabled (ap_prc, a_pid))
    {
      OMX_BUFFERHEADERTYPE * p_hdr = tiz_filter_prc_get_header (ap_prc, a_pid);
      assert (ap_prc);
      if (p_hdr)
        {
          TIZ_DEBUG (handleOf (ap_prc), "p_hdr [%p] nFilledLen [%u]", p_hdr,
                     p_hdr->nFilledLen);
          propagate_eos_if_required (ap_prc, p_hdr);
          rc = tiz_filter_prc_release_header (ap_prc, a_pid);
        }
    }
  return rc;
}

OMX_ERRORTYPE
output_metadata_elements (webmdmuxflt_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  tiz_buffer_t * p_out_store
    = ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX == a_pid ? ap_prc->p_aud_store_
                                                         : ap_prc->p_vid_store_;
  assert (ap_prc);

  if (tiz_buffer_available (p_out_store)
      && (p_hdr = tiz_filter_prc_get_header (ap_prc, a_pid)))
    {
      /* Copy the data in the temp audio buffer */
      size_t nbytes_to_copy
        = MIN (TIZ_OMX_BUF_AVAIL (p_hdr), tiz_buffer_available (p_out_store));
      memcpy (TIZ_OMX_BUF_PTR (p_hdr) + p_hdr->nFilledLen,
              tiz_buffer_get (p_out_store), nbytes_to_copy);
      tiz_buffer_advance (p_out_store, nbytes_to_copy);
      p_hdr->nFilledLen += nbytes_to_copy;
      tiz_check_omx (release_output_header (ap_prc, a_pid));
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
output_audio_codec_metadata (webmdmuxflt_prc_t * ap_prc,
                             unsigned char * ap_codec_data, size_t a_length)
{
  OMX_BUFFERHEADERTYPE * p_hdr = get_aud_hdr (ap_prc);

  assert (ap_prc);
  assert (ap_codec_data);
  assert (a_length);

  if (p_hdr)
    {
      memcpy (TIZ_OMX_BUF_PTR (p_hdr) + p_hdr->nFilledLen, ap_codec_data,
              MIN (TIZ_OMX_BUF_AVAIL (p_hdr), a_length));
      p_hdr->nFilledLen += a_length;
      TIZ_DEBUG (handleOf (ap_prc), "copy to buffer p_hdr [%p] - len %u", p_hdr,
                 p_hdr->nFilledLen);
      tiz_check_omx (release_output_header (
        ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX));
    }
  else
    {
      /* There are no omx buffers yet on this port, use a temp buffer */
      int pushed = tiz_buffer_push (
        ap_prc->p_aud_store_, ap_codec_data, a_length);
      tiz_check_true_ret_val ((pushed == a_length),
                              OMX_ErrorInsufficientResources);
    }

  TIZ_DEBUG (handleOf (ap_prc), "copied metadata to buffer - len %u", a_length);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
output_video_codec_metadata (webmdmuxflt_prc_t * ap_prc,
                             unsigned char * ap_codec_data, size_t a_length)
{
  OMX_BUFFERHEADERTYPE * p_hdr = get_vid_hdr (ap_prc);

  assert (ap_prc);
  assert (ap_codec_data);
  assert (a_length);

  if (p_hdr)
    {
      memcpy (TIZ_OMX_BUF_PTR (p_hdr) + p_hdr->nFilledLen, ap_codec_data,
              MIN (TIZ_OMX_BUF_AVAIL (p_hdr), a_length));
      p_hdr->nFilledLen += a_length;
      TIZ_DEBUG (handleOf (ap_prc), "copy to buffer p_hdr [%p] - len %u", p_hdr,
                 p_hdr->nFilledLen);
      tiz_check_omx (release_output_header (
        ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_2_INDEX));
    }
  else
    {
      /* There are no omx buffers yet on this port, use a temp buffer */
      int pushed = tiz_buffer_push (
        ap_prc->p_vid_store_, ap_codec_data, a_length);
      tiz_check_true_ret_val ((pushed == a_length),
                              OMX_ErrorInsufficientResources);
    }

  TIZ_DEBUG (handleOf (ap_prc), "copied metadata to buffer - len %u", a_length);

  return OMX_ErrorNone;
}

static void
print_track_info (webmdmuxflt_prc_t * ap_prc, unsigned int ntracks,
                  unsigned int track_idx, int track_type, int codec_id)
{
  assert (ap_prc);
  TIZ_DEBUG (handleOf (ap_prc),
             "ntracks: %u track: %u type: %d codec: %d duration %llu", ntracks,
             track_idx, track_type, codec_id, ap_prc->ne_duration_ / 1000);
}

static OMX_ERRORTYPE
prepare_port_auto_detection (webmdmuxflt_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  /* Prepare audio port */
  TIZ_INIT_OMX_PORT_STRUCT (port_def,
                            ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));
  ap_prc->audio_coding_type_ = port_def.format.audio.eEncoding;
  ap_prc->audio_auto_detect_on_
    = (OMX_AUDIO_CodingAutoDetect == ap_prc->audio_coding_type_) ? true : false;

  /* Prepare video port */
  TIZ_INIT_OMX_PORT_STRUCT (port_def,
                            ARATELIA_WEBM_DEMUXER_FILTER_PORT_2_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));
  ap_prc->video_coding_type_ = port_def.format.video.eCompressionFormat;
  ap_prc->video_auto_detect_on_
    = (OMX_VIDEO_CodingAutoDetect == ap_prc->video_coding_type_) ? true : false;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
store_data (webmdmuxflt_prc_t * ap_prc)
{
  bool rc = OMX_ErrorNone;
  assert (ap_prc);

  OMX_BUFFERHEADERTYPE * p_in = get_webm_hdr (ap_prc);

  if (p_in)
    {
      int pushed = 0;
      TIZ_TRACE (handleOf (ap_prc), "avail [%d] incoming [%d]",
                 tiz_buffer_available (ap_prc->p_webm_store_),
                 p_in->nFilledLen - p_in->nOffset);
      pushed = tiz_buffer_push (
        ap_prc->p_webm_store_, p_in->pBuffer + p_in->nOffset, p_in->nFilledLen);
      tiz_check_true_ret_val ((pushed == p_in->nFilledLen),
                              OMX_ErrorInsufficientResources);
      rc = release_input_header (ap_prc);
    }
  return rc;
}

static OMX_ERRORTYPE
extract_track_data (webmdmuxflt_prc_t * ap_prc, const unsigned int a_track,
                    const OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNotReady;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;

  if (tiz_filter_prc_is_port_disabled (ap_prc, a_pid))
    {
      return OMX_ErrorNone;
    }

  output_metadata_elements (ap_prc, a_pid);

  p_hdr = tiz_filter_prc_get_header (ap_prc, a_pid);
  if (p_hdr)
    {
      unsigned int chunks = 0;
      unsigned char * p_data = NULL;
      size_t data_size = 0;
      int nestegg_rc = 0;

      assert (ap_prc);
      assert (ap_prc->p_ne_pkt_);

      TIZ_DEBUG (handleOf (ap_prc), "track %d - pid %u", a_track, a_pid);

      nestegg_packet_count (ap_prc->p_ne_pkt_, &chunks);

      /* Extract a packet */
      assert (ap_prc->ne_chunk_ <= chunks);
      if (ap_prc->ne_chunk_ < chunks
          && ((nestegg_rc = nestegg_packet_data (
                 ap_prc->p_ne_pkt_, ap_prc->ne_chunk_, &p_data, &data_size))
              == 0)
          && TIZ_OMX_BUF_AVAIL (p_hdr) >= data_size)
        {
          memcpy (TIZ_OMX_BUF_PTR (p_hdr) + p_hdr->nFilledLen, p_data,
                  data_size);
          p_hdr->nFilledLen += data_size;
          TIZ_DEBUG (handleOf (ap_prc), "copy to buffer p_hdr [%p] - len %u",
                     p_hdr, p_hdr->nFilledLen);
          ++ap_prc->ne_chunk_;
        }

      WEBMDMUX_LOG_STATE (ap_prc);

      /* Release the OMX buffer */
      if (TIZ_OMX_BUF_FILL_LEN (p_hdr) > 0)
        {
          tiz_check_omx (release_output_header (ap_prc, a_pid));
        }

      /* Release the ne packet if all chunks have already been processed */
      if (ap_prc->ne_chunk_ >= chunks)
        {
          /* All chunks extracted, release the packet now. */
          nestegg_free_packet (ap_prc->p_ne_pkt_);
          ap_prc->p_ne_pkt_ = NULL;
          ap_prc->ne_chunk_ = 0;
          /* Return ErrorNone. This is an indication that we will need to keep
             reading data from the internal data store */
          rc = OMX_ErrorNone;
        }
    }
  return rc;
}

static OMX_ERRORTYPE
extract_track_audio_data (webmdmuxflt_prc_t * ap_prc,
                          const unsigned int a_track)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);
  if (a_track == ap_prc->ne_audio_track_)
    {
      rc = extract_track_data (ap_prc, a_track,
                               ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);
    }
  return rc;
}

static OMX_ERRORTYPE
extract_track_video_data (webmdmuxflt_prc_t * ap_prc,
                          const unsigned int a_track)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);
  if (a_track == ap_prc->ne_video_track_)
    {
      rc = extract_track_data (ap_prc, a_track,
                               ARATELIA_WEBM_DEMUXER_FILTER_PORT_2_INDEX);
    }
  return rc;
}

static bool
able_to_demux (webmdmuxflt_prc_t * ap_prc)
{
  bool rc = true;
  bool compressed_data_avail
    = (tiz_buffer_available (ap_prc->p_webm_store_) > 0);
  bool enough_compressed_data_avail
    = compressed_data_avail
      && !(ap_prc->ne_read_err_ < 0
           && tiz_buffer_available (ap_prc->p_webm_store_)
                < ap_prc->ne_last_read_len_);

  if (!compressed_data_avail && tiz_filter_prc_is_eos (ap_prc))
    {
      release_output_header (ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);
      release_output_header (ap_prc, ARATELIA_WEBM_DEMUXER_FILTER_PORT_2_INDEX);
    }

  if (!compressed_data_avail || !enough_compressed_data_avail
      || (!tiz_filter_prc_output_headers_available (ap_prc)))
    {
      rc = false;
    }

  return rc;
}

static OMX_ERRORTYPE
read_packet (webmdmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNotReady;

  assert (ap_prc);

  if (ap_prc->ne_read_err_ < 0)
    {
      ap_prc->ne_read_err_ = 0;
      nestegg_read_reset (ap_prc->p_ne_);
    }

  if (ap_prc->p_ne_pkt_
      || (ap_prc->ne_read_err_
          = nestegg_read_packet (ap_prc->p_ne_, &ap_prc->p_ne_pkt_))
           > 0)
    {
      unsigned int track = 0;
      assert (ap_prc->p_ne_pkt_);

      nestegg_packet_track (ap_prc->p_ne_pkt_, &track);

      tiz_check_omx (extract_track_audio_data (ap_prc, track));
      tiz_check_omx (extract_track_video_data (ap_prc, track));

      if (track != ap_prc->ne_audio_track_ && track != ap_prc->ne_video_track_)
        {
          /* Just in case, read the next packet */
          nestegg_free_packet (ap_prc->p_ne_pkt_);
          ap_prc->p_ne_pkt_ = NULL;
          ap_prc->ne_chunk_ = 0;
        }
    }
  else
    {
      TIZ_DEBUG (handleOf (ap_prc), "read packet return code %d",
                 ap_prc->ne_read_err_);
    }

  return rc;
}

static OMX_ERRORTYPE
demux_stream (webmdmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNotReady;

  assert (ap_prc);
  assert (ap_prc->p_ne_);

  while (able_to_demux (ap_prc))
    {
      rc = read_packet (ap_prc);
    }

  return rc;
}

static OMX_ERRORTYPE
alloc_input_store (webmdmuxflt_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def,
                            ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_webm_store_ == NULL);
  tiz_check_omx (
    tiz_buffer_init (&(ap_prc->p_webm_store_), port_def.nBufferSize * 4));

  /* Will need to seek on this buffer  */
  return tiz_buffer_seek_mode (ap_prc->p_webm_store_, TIZ_BUFFER_SEEKABLE);
}

static OMX_ERRORTYPE
alloc_output_stores (webmdmuxflt_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE aud_port_def;
  OMX_PARAM_PORTDEFINITIONTYPE vid_port_def;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (aud_port_def,
                            ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &aud_port_def));

  TIZ_INIT_OMX_PORT_STRUCT (vid_port_def,
                            ARATELIA_WEBM_DEMUXER_FILTER_PORT_2_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &vid_port_def));

  assert (ap_prc->p_aud_store_ == NULL);
  tiz_check_omx (
    tiz_buffer_init (&(ap_prc->p_aud_store_), aud_port_def.nBufferSize));

  assert (ap_prc->p_vid_store_ == NULL);
  tiz_check_omx (
    tiz_buffer_init (&(ap_prc->p_vid_store_), vid_port_def.nBufferSize));

  return OMX_ErrorNone;
}

static inline void
dealloc_nestegg (
  /*@special@ */ webmdmuxflt_prc_t * ap_prc)
/*@releases ap_prc->p_ne_@ */
/*@ensures isnull ap_prc->p_ne_@ */
{
  assert (ap_prc);
  if (ap_prc->p_ne_)
    {
      nestegg_destroy (ap_prc->p_ne_);
      ap_prc->p_ne_ = NULL;
    }
}

static void
reset_nestegg_members (webmdmuxflt_prc_t * ap_prc)
{
  assert (ap_prc);
  assert (!ap_prc->p_ne_);
  ap_prc->p_ne_ = NULL;
  ap_prc->ne_io_.read = ne_io_read;
  ap_prc->ne_io_.seek = ne_io_seek;
  ap_prc->ne_io_.tell = ne_io_tell;
  ap_prc->ne_io_.userdata = ap_prc;
  tiz_mem_set (&(ap_prc->ne_audio_params_), 0,
               sizeof (ap_prc->ne_audio_params_));
  tiz_mem_set (&(ap_prc->ne_video_params_), 0,
               sizeof (ap_prc->ne_video_params_));
  ap_prc->ne_audio_track_ = NESTEGG_TRACK_UNKNOWN;
  ap_prc->ne_video_track_ = NESTEGG_TRACK_UNKNOWN;
  ap_prc->ne_duration_ = 0;
  ap_prc->p_ne_pkt_ = NULL;
  ap_prc->ne_chunk_ = 0;
  ap_prc->ne_read_err_ = 0;
  ap_prc->ne_last_read_len_ = 0;
}

static void
reset_stream_parameters (webmdmuxflt_prc_t * ap_prc)
{
  assert (ap_prc);

  ap_prc->ne_inited_ = false;
  ap_prc->audio_auto_detect_on_ = false;
  ap_prc->audio_coding_type_ = OMX_AUDIO_CodingUnused;
  ap_prc->video_auto_detect_on_ = false;
  ap_prc->video_coding_type_ = OMX_VIDEO_CodingUnused;

  dealloc_nestegg (ap_prc);
  reset_nestegg_members (ap_prc);

  tiz_buffer_clear (ap_prc->p_webm_store_);
  tiz_buffer_clear (ap_prc->p_aud_store_);
  tiz_buffer_clear (ap_prc->p_vid_store_);
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

static inline void
dealloc_input_store (
  /*@special@ */ webmdmuxflt_prc_t * ap_prc)
/*@releases ap_prc->p_webm_store_@ */
/*@ensures isnull ap_prc->p_webm_store_@ */
{
  assert (ap_prc);
  tiz_buffer_destroy (ap_prc->p_webm_store_);
  ap_prc->p_webm_store_ = NULL;
}

static inline void
dealloc_output_stores (
  /*@special@ */ webmdmuxflt_prc_t * ap_prc)
/*@releases ap_prc->p_webm_store_@ */
/*@ensures isnull ap_prc->p_webm_store_@ */
{
  assert (ap_prc);
  tiz_buffer_destroy (ap_prc->p_aud_store_);
  ap_prc->p_aud_store_ = NULL;
  tiz_buffer_destroy (ap_prc->p_vid_store_);
  ap_prc->p_vid_store_ = NULL;
}

static OMX_ERRORTYPE
set_audio_coding_on_port (webmdmuxflt_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_DEBUG (handleOf (ap_prc),
             " audio: %.2fhz %u bit %u channels %llu preskip %llu preroll",
             ap_prc->ne_audio_params_.rate, ap_prc->ne_audio_params_.depth,
             ap_prc->ne_audio_params_.channels,
             ap_prc->ne_audio_params_.codec_delay,
             ap_prc->ne_audio_params_.seek_preroll);

  TIZ_INIT_OMX_PORT_STRUCT (port_def,
                            ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);
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
set_video_coding_on_port (webmdmuxflt_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  assert (ap_prc);

  TIZ_DEBUG (
    handleOf (ap_prc), " video: %ux%u (d: %ux%u %ux%ux%ux%u)",
    ap_prc->ne_video_params_.width, ap_prc->ne_video_params_.height,
    ap_prc->ne_video_params_.display_width,
    ap_prc->ne_video_params_.display_height, ap_prc->ne_video_params_.crop_top,
    ap_prc->ne_video_params_.crop_left, ap_prc->ne_video_params_.crop_bottom,
    ap_prc->ne_video_params_.crop_right);

  TIZ_INIT_OMX_PORT_STRUCT (port_def,
                            ARATELIA_WEBM_DEMUXER_FILTER_PORT_2_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));

  /* Set the new value */
  port_def.format.video.eCompressionFormat = ap_prc->video_coding_type_;

  tiz_check_omx (tiz_krn_SetParameter_internal (
    tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
    OMX_IndexParamPortDefinition, &port_def));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
read_audio_metadata (webmdmuxflt_prc_t * ap_prc, const unsigned int a_track_idx,
                     const int a_codec_id, const int a_track_type)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);

  /* Do nothing if track type is not audio */
  if (NESTEGG_TRACK_AUDIO == a_track_type
      && NESTEGG_TRACK_UNKNOWN == ap_prc->ne_audio_track_)
    {
      unsigned int nheaders = 0;
      unsigned int header_idx = 0;

      /* WebM audio codecs are OPUS or VORBIS */
      ap_prc->audio_coding_type_
        = (a_codec_id == NESTEGG_CODEC_OPUS ? OMX_AUDIO_CodingOPUS
                                            : OMX_AUDIO_CodingVORBIS);
      ap_prc->ne_audio_track_ = a_track_idx;

      /* Retrieve the audio parameters */
      on_nestegg_error_ret_omx_oom (nestegg_track_audio_params (
        ap_prc->p_ne_, a_track_idx, &ap_prc->ne_audio_params_));

      /* Retrieve the number of codec specific metadata items */
      on_nestegg_error_ret_omx_oom (
        nestegg_track_codec_data_count (ap_prc->p_ne_, a_track_idx, &nheaders));

      TIZ_DEBUG (handleOf (ap_prc), "nheaders [%u]", nheaders);

      for (header_idx = 0; header_idx < nheaders; ++header_idx)
        {
          unsigned char * p_codec_data = NULL;
          size_t length = 0;
          on_nestegg_error_ret_omx_oom (nestegg_track_codec_data (
            ap_prc->p_ne_, a_track_idx, header_idx, &p_codec_data, &length));
          print_audio_codec_metadata (ap_prc, header_idx, nheaders,
                                      p_codec_data, length);
          /* Put the audio specific codec data on an OMX buffer */
          tiz_check_omx (
            output_audio_codec_metadata (ap_prc, p_codec_data, length));
        }
      tiz_check_omx (set_audio_coding_on_port (ap_prc));
    }
  return rc;
}

static OMX_ERRORTYPE
read_video_metadata (webmdmuxflt_prc_t * ap_prc, const unsigned int a_track_idx,
                     const int a_codec_id, const int a_track_type)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);

  /* Do nothing if track type is not video */
  if (NESTEGG_TRACK_VIDEO == a_track_type
      && NESTEGG_TRACK_UNKNOWN == ap_prc->ne_video_track_)
    {
      unsigned int nheaders = 0;
      unsigned int header_idx = 0;
      size_t length = 0;
      unsigned char * p_codec_data = NULL;

      ap_prc->video_coding_type_
        = (a_codec_id == NESTEGG_CODEC_VP8 ? OMX_VIDEO_CodingVP8
                                           : OMX_VIDEO_CodingVP9);
      ap_prc->ne_video_track_ = a_track_idx;

      on_nestegg_error_ret_omx_oom (nestegg_track_video_params (
        ap_prc->p_ne_, a_track_idx, &ap_prc->ne_video_params_));

      on_nestegg_error_ret_omx_oom (
        nestegg_track_codec_data_count (ap_prc->p_ne_, a_track_idx, &nheaders));

      for (header_idx = 0; header_idx < nheaders; ++header_idx)
        {
          on_nestegg_error_ret_omx_oom (nestegg_track_codec_data (
            ap_prc->p_ne_, a_track_idx, header_idx, &p_codec_data, &length));
          /* Put the video specific codec data on an OMX buffer */
          tiz_check_omx (
            output_video_codec_metadata (ap_prc, p_codec_data, length));
        }

      tiz_check_omx (set_video_coding_on_port (ap_prc));
    }
  return rc;
}

static OMX_ERRORTYPE
obtain_track_info (webmdmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  unsigned int ntracks = 0;
  unsigned int track_idx = 0;

  assert (ap_prc);

  on_nestegg_error_ret_omx_oom (nestegg_track_count (ap_prc->p_ne_, &ntracks));

  nestegg_duration (ap_prc->p_ne_, &ap_prc->ne_duration_);

  for (track_idx = 0; track_idx < ntracks; ++track_idx)
    {
      int codec_id = nestegg_track_codec_id (ap_prc->p_ne_, track_idx);
      int track_type = 0;

      tiz_check_true_ret_val ((codec_id != -1), OMX_ErrorInsufficientResources);

      track_type = nestegg_track_type (ap_prc->p_ne_, track_idx);

      print_track_info (ap_prc, ntracks, track_idx, track_type, codec_id);

      tiz_check_omx (
        read_audio_metadata (ap_prc, track_idx, codec_id, track_type));
      tiz_check_omx (
        read_video_metadata (ap_prc, track_idx, codec_id, track_type));
    }
  return rc;
}

static void
send_auto_detect_event (webmdmuxflt_prc_t * ap_prc, OMX_S32 * ap_coding_type,
                        const OMX_S32 a_coding_type1,
                        const OMX_S32 a_coding_type2, const OMX_U32 a_pid)
{
  assert (ap_prc);
  assert (ap_coding_type);
  if (*ap_coding_type != a_coding_type1 && *ap_coding_type != a_coding_type2)
    {
      TIZ_DEBUG (
        handleOf (ap_prc),
        "Issuing OMX_EventPortFormatDetected - audio_coding_type_ [%X]",
        *ap_coding_type);

      /* TODO: update the output port with the corresponding audio or video settings detected */
      tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortFormatDetected, 0, 0,
                           NULL);
      tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortSettingsChanged,
                           a_pid,
                           OMX_IndexParamPortDefinition, /* the index of the
                                                      struct that has
                                                      been modififed */
                           NULL);
    }
  else
    {
      /* Oops... could not detect the stream format */
      tiz_srv_issue_err_event ((OMX_PTR) ap_prc, OMX_ErrorFormatNotDetected);
    }
}

static OMX_ERRORTYPE
send_port_auto_detect_events (webmdmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = obtain_track_info (ap_prc);
  if (OMX_ErrorNone == rc)
    {
      if (NESTEGG_TRACK_UNKNOWN != ap_prc->ne_audio_track_)
        {
          send_auto_detect_event (ap_prc, &(ap_prc->audio_coding_type_),
                                  OMX_AUDIO_CodingUnused,
                                  OMX_AUDIO_CodingAutoDetect,
                                  ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX);
        }
      if (NESTEGG_TRACK_UNKNOWN != ap_prc->ne_video_track_)
        {
          send_auto_detect_event (ap_prc, &(ap_prc->video_coding_type_),
                                  OMX_VIDEO_CodingUnused,
                                  OMX_VIDEO_CodingAutoDetect,
                                  ARATELIA_WEBM_DEMUXER_FILTER_PORT_2_INDEX);
        }
    }
  return rc;
}

static OMX_ERRORTYPE
alloc_nestegg (webmdmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  int nestegg_rc = 0;
  assert (!ap_prc->p_ne_);
  nestegg_rc = nestegg_init (&ap_prc->p_ne_, ap_prc->ne_io_, ne_log, -1);

  if (0 != nestegg_rc)
    {
      dealloc_nestegg (ap_prc);
      tiz_buffer_clear (ap_prc->p_webm_store_);
    }
  else
    {
      rc = send_port_auto_detect_events (ap_prc);
      ap_prc->ne_inited_ = true;
    }
  TIZ_DEBUG (handleOf (ap_prc), "nestegg inited = %s",
             (ap_prc->ne_inited_ ? "TRUE" : "FALSE"));
  return rc;
}

static inline OMX_ERRORTYPE
do_flush (webmdmuxflt_prc_t * ap_prc, OMX_U32 a_pid)
{
  assert (ap_prc);
  if (OMX_ALL == a_pid || ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX == a_pid)
    {
      reset_stream_parameters (ap_prc);
    }
  /* Release any buffers held  */
  return tiz_filter_prc_release_header (ap_prc, a_pid);
}

/*
 * webmdmuxfltprc
 */

static void *
webmdmuxflt_prc_ctor (void * ap_prc, va_list * app)
{
  webmdmuxflt_prc_t * p_prc
    = super_ctor (typeOf (ap_prc, "webmdmuxfltprc"), ap_prc, app);
  assert (p_prc);
  p_prc->p_webm_store_ = NULL;
  p_prc->p_aud_store_ = NULL;
  p_prc->p_vid_store_ = NULL;
  reset_stream_parameters (p_prc);
  g_handle = handleOf (ap_prc);
  return p_prc;
}

static void *
webmdmuxflt_prc_dtor (void * ap_obj)
{
  (void) webmdmuxflt_prc_deallocate_resources (ap_obj);
  g_handle = NULL;
  return super_dtor (typeOf (ap_obj, "webmdmuxfltprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
webmdmuxflt_prc_allocate_resources (void * ap_prc, OMX_U32 a_pid)
{
  webmdmuxflt_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_check_omx (alloc_input_store (p_prc));
  tiz_check_omx (alloc_output_stores (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webmdmuxflt_prc_deallocate_resources (void * ap_prc)
{
  webmdmuxflt_prc_t * p_prc = ap_prc;
  assert (p_prc);
  dealloc_output_stores (p_prc);
  dealloc_input_store (p_prc);
  dealloc_nestegg (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webmdmuxflt_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  webmdmuxflt_prc_t * p_prc = ap_prc;
  assert (ap_prc);
  return prepare_port_auto_detection (p_prc);
}

static OMX_ERRORTYPE
webmdmuxflt_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webmdmuxflt_prc_stop_and_return (void * ap_prc)
{
  /* Do flush on all ports; this will reset the stream parameters and release
     any buffers held */
  return do_flush (ap_prc, OMX_ALL);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
webmdmuxflt_prc_buffers_ready (const void * ap_prc)
{
  webmdmuxflt_prc_t * p_prc = (webmdmuxflt_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_prc);

  tiz_check_omx (store_data (p_prc));

  if (!p_prc->ne_inited_)
    {
      tiz_check_omx (alloc_nestegg (p_prc));
    }

  if (p_prc->ne_inited_)
    {
      rc = demux_stream (p_prc);
      if (OMX_ErrorNotReady == rc)
        {
          rc = OMX_ErrorNone;
        }
    }

  return rc;
}

static OMX_ERRORTYPE
webmdmuxflt_prc_pause (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webmdmuxflt_prc_resume (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
webmdmuxflt_prc_port_flush (const void * ap_prc, OMX_U32 a_pid)
{
  webmdmuxflt_prc_t * p_prc = (webmdmuxflt_prc_t *) ap_prc;
  return do_flush (p_prc, a_pid);
}

static OMX_ERRORTYPE
webmdmuxflt_prc_port_disable (const void * ap_prc, OMX_U32 a_pid)
{
  webmdmuxflt_prc_t * p_prc = (webmdmuxflt_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = tiz_filter_prc_release_header (p_prc, a_pid);
  if (OMX_ALL == a_pid || ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX == a_pid)
    {
      reset_stream_parameters (p_prc);
    }
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

static OMX_ERRORTYPE
webmdmuxflt_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  webmdmuxflt_prc_t * p_prc = (webmdmuxflt_prc_t *) ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

/*
 * webmdmuxflt_prc_class
 */

static void *
webmdmuxflt_prc_class_ctor (void * ap_prc, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_prc, "webmdmuxfltprc_class"), ap_prc, app);
}

/*
 * initialization
 */

void *
webmdmuxflt_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * webmdmuxfltprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizfilterprc), "webmdmuxfltprc_class", classOf (tizfilterprc),
     sizeof (webmdmuxflt_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, webmdmuxflt_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return webmdmuxfltprc_class;
}

void *
webmdmuxflt_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * webmdmuxfltprc_class = tiz_get_type (ap_hdl, "webmdmuxfltprc_class");
  TIZ_LOG_CLASS (webmdmuxfltprc_class);
  void * webmdmuxfltprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (webmdmuxfltprc_class, "webmdmuxfltprc", tizfilterprc,
     sizeof (webmdmuxflt_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, webmdmuxflt_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, webmdmuxflt_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, webmdmuxflt_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, webmdmuxflt_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, webmdmuxflt_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, webmdmuxflt_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, webmdmuxflt_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, webmdmuxflt_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, webmdmuxflt_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, webmdmuxflt_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, webmdmuxflt_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, webmdmuxflt_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, webmdmuxflt_prc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return webmdmuxfltprc;
}

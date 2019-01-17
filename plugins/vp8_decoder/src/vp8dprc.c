/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
 *
 * Portions Copyright (c) 2013 The WebM project authors.
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
 * @file   vp8dprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - VP8 Decoder processor class
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizplatform.h>

#include <tizkernel.h>

#include "vp8d.h"
#include "vp8dprc.h"
#include "vp8dprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.vp8_decoder.prc"
#endif

#define bail_on_vpx_err_with_omx_err(expr, omx_err)                          \
  do                                                                         \
    {                                                                        \
      vpx_codec_err_t vpx_err = VPX_CODEC_OK;                                \
      if (VPX_CODEC_OK != (vpx_err = (expr)))                                \
        {                                                                    \
          const char * detail = vpx_codec_error_detail (&(ap_prc->vp8ctx_)); \
          TIZ_ERROR (handleOf (ap_prc),                                      \
                     "[%s] : vpx error "                                     \
                     "(%s - %s)",                                            \
                     tiz_err_to_str (omx_err),                               \
                     vpx_codec_error (&(ap_prc->vp8ctx_)),                   \
                     detail ? detail : "");                                  \
          rc = omx_err;                                                      \
          goto end;                                                          \
        }                                                                    \
    }                                                                        \
  while (0)

#define VP8DPRC_LOG_STATE(ap_prc)                                           \
  do                                                                        \
    {                                                                       \
      TIZ_DEBUG (handleOf (ap_prc),                                         \
                 "Stream [%s] fourcc [%s] width [%d] height [%d] "          \
                 "fps_den [%d] fps_num [%d]",                               \
                 ap_prc->info_.type == STREAM_RAW                           \
                   ? "RAW"                                                  \
                   : (ap_prc->info_.type == STREAM_IVF ? "IVF" : "UKNOWN"), \
                 ap_prc->info_.fourcc == VP8_FOURCC ? "VP8" : "OTHER",      \
                 ap_prc->info_.width, ap_prc->info_.height,                 \
                 ap_prc->info_.fps_den, ap_prc->info_.fps_num);             \
    }                                                                       \
  while (0)

static const struct
{
  char const * name;
  vpx_codec_iface_t * iface;
  unsigned int fourcc;
  unsigned int fourcc_mask;
} ifaces[] = {
  {"vp8", &vpx_codec_vp8_dx_algo, VP8_FOURCC, 0x00FFFFFF},
};

/* NOTE: Code from libvpx's mem_ops.h */
static unsigned int
mem_get_le16 (const void * vmem)
{
  unsigned int val;
  const unsigned char * mem = (const unsigned char *) vmem;

  val = mem[1] << 8;
  val |= mem[0];
  return val;
}

/* NOTE: Code from libvpx's mem_ops.h */
static unsigned int
mem_get_le32 (const void * vmem)
{
  unsigned int val;
  const unsigned char * mem = (const unsigned char *) vmem;

  val = mem[3] << 24;
  val |= mem[2] << 16;
  val |= mem[1] << 8;
  val |= mem[0];
  return val;
}

/* NOTE: Code from libvpx's ivfdec.c */
static void
fix_framerate (int * num, int * den)
{
  // Some versions of vpxenc used 1/(2*fps) for the timebase, so
  // we can guess the framerate using only the timebase in this
  // case. Other files would require reading ahead to guess the
  // timebase.
  if (*den > 0 && *den < 1000000000 && *num > 0 && *num < 1000)
    {
      // Correct for the factor of 2 applied to the timebase in the encoder.
      if (*num & 1)
        *den *= 2;
      else
        *num /= 2;
    }
  else
    {
      // Don't know FPS for sure, and don't have readahead code
      // (yet?), so just default to 30fps.
      *num = 30;
      *den = 1;
    }
}

/* NOTE: With code from libvpx's ivfdec.c */
static int
is_ivf (vp8d_prc_t * ap_prc, OMX_U8 * ap_buf)
{
  int is_ivf = 0;

  assert (ap_prc);
  assert (ap_buf);

  if (ap_buf[0] == 'D' && ap_buf[1] == 'K' && ap_buf[2] == 'I'
      && ap_buf[3] == 'F')
    {
      is_ivf = 1;

      if (mem_get_le16 (ap_buf + 4) != 0)
        {
          TIZ_ERROR (handleOf (ap_prc),
                     "Error: Unrecognized IVF version! This file may not"
                     " decode properly.");
        }

      ap_prc->info_.fourcc = mem_get_le32 (ap_buf + 8);
      ap_prc->info_.width = mem_get_le16 (ap_buf + 12);
      ap_prc->info_.height = mem_get_le16 (ap_buf + 14);
      ap_prc->info_.fps_num = mem_get_le32 (ap_buf + 16);
      ap_prc->info_.fps_den = mem_get_le32 (ap_buf + 20);
      fix_framerate ((int *) &ap_prc->info_.fps_num,
                     (int *) &ap_prc->info_.fps_den);
    }

  return is_ivf;
}

/* NOTE: With code from libvpx's vpxdec.c */
static int
peek_raw_stream (vp8d_prc_t * ap_prc, const OMX_U8 * ap_buf,
                 const size_t a_size_bytes)
{
  vpx_codec_stream_info_t si;
  int i = 0;
  int is_raw = 0;

  assert (ap_prc);
  assert (ap_buf);

  si.sz = sizeof (si);

  for (i = 0; i < sizeof (ifaces) / sizeof (ifaces[0]); i++)
    {
      if (VPX_CODEC_OK == vpx_codec_peek_stream_info (ifaces[i].iface, ap_buf,
                                                      a_size_bytes, &si))
        {
          is_raw = 1;
          ap_prc->info_.fourcc = ifaces[i].fourcc;
          ap_prc->info_.width = si.w;
          ap_prc->info_.height = si.h;
          /* The framerate is unknown so set it to 0 as per spec. */
          ap_prc->info_.fps_num = 0;
          ap_prc->info_.fps_den = 1;
          break;
        }
      else
        {
          TIZ_TRACE (handleOf (ap_prc), "Not a raw strem");
        }
    }
  return is_raw;
}

static int
is_raw (vp8d_prc_t * ap_prc, OMX_U8 * ap_buf)
{
  return peek_raw_stream (ap_prc, ap_buf, 32);
}

static int
is_raw_with_length_hdr (vp8d_prc_t * ap_prc, OMX_U8 * ap_buf)
{
  int is_raw = 0;

  assert (ap_prc);
  assert (ap_buf);

  if (mem_get_le32 (ap_buf) < CORRUPT_FRAME_THRESHOLD)
    {
      is_raw = peek_raw_stream (ap_prc, ap_buf + 4, 32 - 4);
    }
  else
    {
      TIZ_WARN (handleOf (ap_prc), "Read invalid frame size");
    }
  return is_raw;
}

static int
identify_stream (vp8d_prc_t * ap_prc, OMX_U8 * ap_buf)
{
  int rc = EXIT_SUCCESS;

  assert (ap_prc);
  assert (ap_buf);

  if (is_ivf (ap_prc, ap_buf))
    {
      TIZ_DEBUG (handleOf (ap_prc), "STREAM_IVF");
      ap_prc->info_.type = STREAM_IVF;
    }
  else if (is_raw (ap_prc, ap_buf))
    {
      TIZ_DEBUG (handleOf (ap_prc), "STREAM_RAW");
      ap_prc->info_.type = STREAM_RAW;
    }
  else if (is_raw_with_length_hdr (ap_prc, ap_buf))
    {
      TIZ_DEBUG (handleOf (ap_prc), "STREAM_RAW_WITH_LENGTH_HDR");
      ap_prc->info_.type = STREAM_RAW_WITH_LENGTH_HDR;
    }
  else
    {
      TIZ_DEBUG (handleOf (ap_prc), "STREAM_UNKNOWN");
      ap_prc->info_.type = STREAM_UNKNOWN;
      rc = EXIT_FAILURE;
    }

  return rc;
}

static OMX_ERRORTYPE
update_output_port_params (vp8d_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  vp8d_stream_info_t * p_inf = NULL;
  OMX_VIDEO_PORTDEFINITIONTYPE * p_def = NULL;
  OMX_U32 framerate_q16 = 0;

  assert (ap_prc);

  p_inf = &(ap_prc->info_);
  p_def = &(ap_prc->port_def_.format.video);

  assert (ap_prc->info_.fps_den);

  framerate_q16 = (ap_prc->info_.fps_num << 16) / ap_prc->info_.fps_den;

  if (p_inf->width != p_def->nFrameWidth || p_inf->height != p_def->nFrameHeight
      || p_inf->width != p_def->nStride || p_inf->height != p_def->nSliceHeight
      || (framerate_q16 != 0 && framerate_q16 != p_def->xFramerate))
    {
      TIZ_DEBUG (handleOf (ap_prc),
                 "Updating video port format : nFrameWidth : old [%d] new [%d]",
                 p_def->nFrameWidth, p_inf->width);
      TIZ_DEBUG (handleOf (ap_prc),
                 "Updating video port format : nFrameHeight old [%d] new [%d]",
                 p_def->nFrameHeight, p_inf->height);

      /* Effectively disable the output port, until the stream has been
         identified and IL client is ready to re-enable */
      ap_prc->out_port_disabled_ = true;

      p_def->nFrameHeight = p_inf->height;
      p_def->nFrameWidth = p_inf->width;
      p_def->xFramerate = framerate_q16;
      p_def->nStride = p_inf->width; /* NOTE: The output buffers currently only
                                         contain image data without any padding
                                         even if the stride > image width. See
                                         https://msdn.microsoft.com/en-us/library/windows/desktop/aa473780(v=vs.85).aspx
                                        */
      p_def->nSliceHeight = p_inf->height;

      tiz_check_omx (tiz_krn_SetParameter_internal (
        tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
        OMX_IndexParamPortDefinition, &(ap_prc->port_def_)));

      tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortSettingsChanged,
                           ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX,
                           OMX_IndexParamPortDefinition, /* the index of the
                                                            struct that has
                                                            been modififed */
                           NULL);
    }
  return rc;
}

static size_t
read_from_omx_buffer (const vp8d_prc_t * ap_prc, void * ap_dst,
                      const size_t a_bytes, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  size_t to_read = a_bytes;

  assert (ap_prc);
  assert (ap_dst);
  assert (ap_hdr);

  if (a_bytes)
    {
      if (ap_hdr->nFilledLen < a_bytes)
        {
          to_read = ap_hdr->nFilledLen;
        }

      if (to_read)
        {
          memcpy (ap_dst, ap_hdr->pBuffer + ap_hdr->nOffset, to_read);
        }

      ap_hdr->nFilledLen -= to_read;
      ap_hdr->nOffset += to_read;
    }

  return to_read;
}

static OMX_BUFFERHEADERTYPE *
get_input_buffer (vp8d_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->in_port_disabled_)
    {
      return NULL;
    }

  if (!ap_prc->p_inhdr_)
    {
      if (OMX_ErrorNone
          == tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)),
                                   ARATELIA_VP8_DECODER_INPUT_PORT_INDEX, 0,
                                   &ap_prc->p_inhdr_))
        {
#ifndef NDEBUG
          if (ap_prc->p_inhdr_)
            {
              TIZ_TRACE (handleOf (ap_prc),
                         "Claimed input HEADER [%p]... nFilledLen [%d]",
                         ap_prc->p_inhdr_, ap_prc->p_inhdr_->nFilledLen);
            }
#endif
        }
    }
  return ap_prc->p_inhdr_;
}

static OMX_BUFFERHEADERTYPE *
get_output_buffer (vp8d_prc_t * ap_prc)
{
  assert (ap_prc);

  if (ap_prc->out_port_disabled_)
    {
      return NULL;
    }

  if (!ap_prc->p_outhdr_)
    {
      if (OMX_ErrorNone
          == tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)),
                                   ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX, 0,
                                   &ap_prc->p_outhdr_))
        {
          if (ap_prc->p_outhdr_)
            {
              OMX_PTR p_eglimage = NULL;
#ifndef NDEBUG
              TIZ_TRACE (handleOf (ap_prc),
                         "Claimed output HEADER [%p]...nFilledLen [%d]",
                         ap_prc->p_outhdr_, ap_prc->p_outhdr_->nFilledLen);
#endif
              /* Check pBuffer nullity to know if an eglimage have been registered. */
              if (!ap_prc->p_outhdr_->pBuffer
                  && OMX_ErrorNone == tiz_krn_claim_eglimage (
                                        tiz_get_krn (handleOf (ap_prc)),
                                        ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX,
                                        ap_prc->p_outhdr_, &p_eglimage))
                {
                  (void) tiz_krn_release_buffer (
                    tiz_get_krn (handleOf (ap_prc)),
                    ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX, ap_prc->p_outhdr_);
                  ap_prc->p_outhdr_ = NULL;
                }
            }
        }
    }
  return ap_prc->p_outhdr_;
}

static void
buffer_emptied (vp8d_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  assert (ap_prc);
  assert (ap_prc->p_inhdr_ == ap_hdr);

  if (!ap_prc->out_port_disabled_)
    {
      assert (ap_hdr->nFilledLen == 0);
      ap_hdr->nOffset = 0;

      if ((ap_hdr->nFlags & OMX_BUFFERFLAG_EOS) != 0)
        {
          ap_prc->eos_ = true;
        }

      (void) tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)), 0,
                                     ap_hdr);
      ap_prc->p_inhdr_ = NULL;
    }
}

static void
buffer_filled (vp8d_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  assert (ap_prc);
  assert (ap_hdr);
  assert (ap_prc->p_outhdr_ == ap_hdr);

  if (!ap_prc->in_port_disabled_)
    {
      ap_hdr->nOffset = 0;

      if (ap_prc->eos_)
        {
          /* EOS has been received and all the input data has been consumed
       * already, so its time to propagate the EOS flag */
          ap_prc->p_outhdr_->nFlags |= OMX_BUFFERFLAG_EOS;
          /* Reset the flag so we are ready to receive a new stream */
          ap_prc->eos_ = false;
        }

      (void) tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)),
                                     ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX,
                                     ap_hdr);
      ap_prc->p_outhdr_ = NULL;
    }
}

static OMX_ERRORTYPE
obtain_stream_info (vp8d_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * p_inhdr)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_prc);
  assert (p_inhdr);

  if (EXIT_SUCCESS == identify_stream (ap_prc, p_inhdr->pBuffer))
    {
      VP8DPRC_LOG_STATE (ap_prc);

      if (STREAM_IVF == ap_prc->info_.type)
        {
          /* Make sure we skip the IVF header the next time we read from the
           * buffer */
          p_inhdr->nOffset += 32;
          p_inhdr->nFilledLen -= 32;
        }

      /* Update the output port settings */
      tiz_check_omx (update_output_port_params (ap_prc));
    }
  else
    {
      TIZ_ERROR (handleOf (ap_prc), "Unable to identify the stream type");
      rc = OMX_ErrorStreamCorruptFatal;
    }
  return rc;
}

static OMX_ERRORTYPE
read_frame_size (vp8d_prc_t * ap_prc, const size_t a_hdr_size,
                 OMX_BUFFERHEADERTYPE * ap_inhdr, size_t * ap_frame_size)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  char hdr[a_hdr_size];
  size_t bytes_read = 0;
  size_t frame_size = 0;

  assert (ap_prc);
  assert (a_hdr_size > 0);
  assert (ap_inhdr);
  assert (ap_inhdr->nFilledLen > 0);
  assert (ap_frame_size);

  tiz_check_true_ret_val (0 != (bytes_read = read_from_omx_buffer (
                                  ap_prc, hdr, a_hdr_size, ap_inhdr)),
                          OMX_ErrorInsufficientResources);

  frame_size = mem_get_le32 (hdr);

  TIZ_DEBUG (handleOf (ap_prc), "frame size = [%u]", frame_size);

  tiz_check_true_ret_val (frame_size < CORRUPT_FRAME_THRESHOLD,
                          OMX_ErrorInsufficientResources);

  if (ap_prc->info_.type == STREAM_RAW)
    {
      tiz_check_true_ret_val (frame_size > FRAME_TOO_SMALL_THRESHOLD,
                              OMX_ErrorInsufficientResources);
    }

  *ap_frame_size = frame_size;

  return rc;
}

static OMX_ERRORTYPE
realloc_codec_buffer_if_needed (vp8d_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  vp8d_codec_buffer_t * p_buf = NULL;

  assert (ap_prc);

  p_buf = &(ap_prc->codec_buf_);

  if (p_buf->frame_size > p_buf->alloc_len)
    {
      uint8_t * p_new_buf = realloc (p_buf->p_data, 2 * p_buf->frame_size);
      if (p_new_buf)
        {
          p_buf->p_data = p_new_buf;
          p_buf->alloc_len = 2 * p_buf->frame_size;
        }
      else
        {
          TIZ_ERROR (handleOf (ap_prc),
                     "Failed to (re)allocate compressed data buffer");
          p_buf->frame_size = 0;
          rc = OMX_ErrorInsufficientResources;
        }
    }
  return rc;
}

static OMX_ERRORTYPE
read_frame_data_with_hdr (vp8d_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_inhdr,
                          const size_t a_hdr_size)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  vp8d_codec_buffer_t * p_buf = NULL;

  assert (ap_prc);
  assert (ap_inhdr);
  assert (ap_inhdr->nFilledLen > 0);

  p_buf = &(ap_prc->codec_buf_);

  if (p_buf->filled_len == 0)
    {
      tiz_check_omx (
        read_frame_size (ap_prc, a_hdr_size, ap_inhdr, &(p_buf->frame_size)));
      tiz_check_omx (realloc_codec_buffer_if_needed (ap_prc));
    }

  tiz_check_true_ret_val (p_buf->frame_size > 0,
                          OMX_ErrorInsufficientResources);

  if (p_buf->frame_size
      != (p_buf->filled_len += read_from_omx_buffer (
            ap_prc, (p_buf->p_data) + p_buf->filled_len,
            p_buf->frame_size - p_buf->filled_len, ap_prc->p_inhdr_)))
    {
      TIZ_WARN (handleOf (ap_prc), "Failed to read a full frame");
      rc = OMX_ErrorInsufficientResources;
    }

  return rc;
}

static OMX_ERRORTYPE
read_frame_raw (vp8d_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_inhdr)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  vp8d_codec_buffer_t * p_buf = NULL;

  assert (ap_prc);
  assert (ap_inhdr);
  assert (ap_inhdr->nFilledLen > 0);

  p_buf = &(ap_prc->codec_buf_);

  /* Expected a full compressed frame per omx buffer */
  p_buf->frame_size = ap_prc->p_inhdr_->nFilledLen;
  tiz_check_omx (realloc_codec_buffer_if_needed (ap_prc));

  if (p_buf->frame_size
      != (p_buf->filled_len += read_from_omx_buffer (
            ap_prc, (p_buf->p_data) + p_buf->filled_len,
            p_buf->frame_size - p_buf->filled_len, ap_prc->p_inhdr_)))
    {
      TIZ_WARN (handleOf (ap_prc),
                "Failed to read a full frame (frame size = %u)",
                p_buf->frame_size);
      rc = OMX_ErrorInsufficientResources;
    }
  return rc;
}

static OMX_ERRORTYPE
read_frame_raw_with_length_hdr (vp8d_prc_t * ap_prc,
                                OMX_BUFFERHEADERTYPE * ap_inhdr)
{
  return read_frame_data_with_hdr (ap_prc, ap_inhdr, RAW_FRAME_HDR_SZ);
}

static OMX_ERRORTYPE
read_frame_ivf (vp8d_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_inhdr)
{
  return read_frame_data_with_hdr (ap_prc, ap_inhdr, IVF_FRAME_HDR_SZ);
}

static OMX_ERRORTYPE
read_frame (vp8d_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_inhdr)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);

  switch (ap_prc->info_.type)
    {
      case STREAM_RAW:
        {
          rc = read_frame_raw (ap_prc, ap_inhdr);
        }
        break;
      case STREAM_RAW_WITH_LENGTH_HDR:
        {
          rc = read_frame_raw_with_length_hdr (ap_prc, ap_inhdr);
        }
        break;
      case STREAM_IVF:
        {
          rc = read_frame_ivf (ap_prc, ap_inhdr);
        }
        break;
      case STREAM_WEBM:
        {
          /* Not supported for now */
          assert (0);
          rc = OMX_ErrorStreamCorruptFatal;
        }
        break;
      default:
        {
          rc = OMX_ErrorStreamCorruptFatal;
        }
        break;
    }
  return rc;
}

static void
out_put (OMX_BUFFERHEADERTYPE * p_hdr, const uint8_t * buf, unsigned int len)
{
  memcpy (p_hdr->pBuffer + p_hdr->nOffset, buf, len);
  p_hdr->nOffset += len;
  p_hdr->nFilledLen = p_hdr->nOffset;

  if (p_hdr->nFilledLen > p_hdr->nAllocLen)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "len [%d] nFilledLen [%d] nAllocLen [%d]",
               len, p_hdr->nFilledLen, p_hdr->nAllocLen);
      assert (p_hdr->nFilledLen <= p_hdr->nAllocLen);
    }
}

static OMX_ERRORTYPE
decode_frame (vp8d_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  vpx_codec_iter_t iter = NULL;
  vpx_image_t * img = NULL;
  vp8d_codec_buffer_t * p_buf = NULL;

  assert (ap_prc);

  p_buf = &(ap_prc->codec_buf_);

  bail_on_vpx_err_with_omx_err (
    vpx_codec_decode (&(ap_prc->vp8ctx_), p_buf->p_data,
                      (unsigned int) (p_buf->filled_len), NULL, 0),
    OMX_ErrorStreamCorrupt);

  if ((img = vpx_codec_get_frame (&(ap_prc->vp8ctx_), &iter)))
    {
      unsigned int y;
      uint8_t * buf = img->planes[VPX_PLANE_Y];

#if 0
      {
        TIZ_DEBUG (handleOf (ap_prc),
                   "d_w = %u "
                   "d_h = %u "
                   "stride[VPX_PLANE_Y] = %d "
                   "stride[VPX_PLANE_U] = %d "
                   "stride[VPX_PLANE_V] = %d "
                   "img fmt = %0x "
                   "color = %0x "
                   "range = %0x",
                   img->d_w, img->d_h, img->stride[VPX_PLANE_Y],
                   img->stride[VPX_PLANE_U], img->stride[VPX_PLANE_V],
                   img->fmt, img->cs, img->range);
        }
#endif

      for (y = 0; y < img->d_h; y++)
        {
          out_put (ap_prc->p_outhdr_, buf, img->d_w);
          buf += img->stride[VPX_PLANE_Y];
        }

      buf = img->planes[VPX_PLANE_U];

      for (y = 0; y < (1 + img->d_h) / 2; y++)
        {
          out_put (ap_prc->p_outhdr_, buf, (1 + img->d_w) / 2);
          buf += img->stride[VPX_PLANE_U];
        }

      buf = img->planes[VPX_PLANE_V];

      for (y = 0; y < (1 + img->d_h) / 2; y++)
        {
          out_put (ap_prc->p_outhdr_, buf, (1 + img->d_w) / 2);
          buf += img->stride[VPX_PLANE_V];
        }
    }

end:

  p_buf->filled_len = 0;

  return rc;
}

static OMX_ERRORTYPE
decode_stream (vp8d_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE * p_inhdr = NULL;
  OMX_BUFFERHEADERTYPE * p_outhdr = NULL;
  assert (ap_prc);

  /* Step 1: peak at the stream and find the stream parameters */
  if (ap_prc->first_buf_ && !ap_prc->eos_
      && (p_inhdr = get_input_buffer (ap_prc)))
    {
      tiz_check_omx (obtain_stream_info (ap_prc, p_inhdr));
      ap_prc->first_buf_ = false;
    }

  while (!ap_prc->eos_ && (p_inhdr = get_input_buffer (ap_prc))
         && (p_outhdr = get_output_buffer (ap_prc)))
    {
      /* Step 2: Read a frame into our internal storage */
      if (p_inhdr->nFilledLen > 0 && !ap_prc->out_port_disabled_)
        {
          if (OMX_ErrorNone == read_frame (ap_prc, p_inhdr))
            {
              /* Step 3: Decode the frame just read */
              tiz_check_omx (decode_frame (ap_prc));
            }
        }

      /* Step 4: Get rid of input and output buffers, if we can */
      if (p_inhdr->nFilledLen == 0)
        {
          buffer_emptied (ap_prc, p_inhdr);
        }

      if (p_outhdr->nFilledLen > 0 || ap_prc->eos_)
        {
          buffer_filled (ap_prc, p_outhdr);
        }
    }

  return rc;
}

static inline void
free_codec_buffer (vp8d_prc_t * p_prc)
{
  assert (p_prc);
  tiz_mem_free (p_prc->codec_buf_.p_data);
  tiz_mem_set (&(p_prc->codec_buf_), 0, sizeof (p_prc->codec_buf_));
}

static OMX_ERRORTYPE
reset_stream_parameters (vp8d_prc_t * ap_prc)
{
  assert (ap_prc);
  tiz_mem_set (&(ap_prc->info_), 0, sizeof (ap_prc->info_));
  ap_prc->info_.type = STREAM_UNKNOWN;
  free_codec_buffer (ap_prc);
  TIZ_INIT_OMX_PORT_STRUCT (ap_prc->port_def_,
                            ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &(ap_prc->port_def_)));
  ap_prc->p_inhdr_ = 0;
  ap_prc->p_outhdr_ = 0;
  ap_prc->first_buf_ = true;
  ap_prc->eos_ = false;
  return OMX_ErrorNone;
}

static void
release_input_header (vp8d_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->p_inhdr_)
    {
      assert (!ap_prc->in_port_disabled_);
      (void) tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)),
                                     ARATELIA_VP8_DECODER_INPUT_PORT_INDEX,
                                     ap_prc->p_inhdr_);
      ap_prc->p_inhdr_ = NULL;
    }
}

static void
release_output_header (vp8d_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->p_outhdr_)
    {
      assert (!ap_prc->out_port_disabled_);
      (void) tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)),
                                     ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX,
                                     ap_prc->p_outhdr_);
      ap_prc->p_outhdr_ = NULL;
    }
}

static void
release_all_headers (vp8d_prc_t * ap_prc)
{
  release_input_header (ap_prc);
  release_output_header (ap_prc);
}

/*
 * vp8dprc
 */

static void *
vp8d_prc_ctor (void * ap_obj, va_list * app)
{
  vp8d_prc_t * p_prc = super_ctor (typeOf (ap_obj, "vp8dprc"), ap_obj, app);
  assert (p_prc);
  p_prc->in_port_disabled_ = false;
  p_prc->out_port_disabled_ = false;
  (void) reset_stream_parameters (p_prc);
  return p_prc;
}

static void *
vp8d_prc_dtor (void * ap_obj)
{
  vp8d_prc_t * p_obj = ap_obj;
  free_codec_buffer (p_obj);
  return super_dtor (typeOf (ap_obj, "vp8dprc"), ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
vp8d_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  vp8d_prc_t * ap_prc = ap_obj;
  int flags = 0;

  assert (ap_prc);

  /* TODO : vp8 decoder flags */
  /*   flags = (postprc ? VPX_CODEC_USE_POSTPRC : 0) | */
  /*     (ec_enabled ? VPX_CODEC_USE_ERROR_CONCEALMENT : 0); */

  /* Initialize codec */
  bail_on_vpx_err_with_omx_err (
    vpx_codec_dec_init (&(ap_prc->vp8ctx_), ifaces[0].iface, NULL, flags),
    OMX_ErrorInsufficientResources);

end:

  return rc;
}

static OMX_ERRORTYPE
vp8d_prc_deallocate_resources (void * ap_obj)
{
  vp8d_prc_t * p_prc = ap_obj;
  assert (p_prc);
  free_codec_buffer (p_prc);
  (void) vpx_codec_destroy (&(p_prc->vp8ctx_));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_prc_prepare_to_transfer (void * ap_obj, OMX_U32 a_pid)
{
  vp8d_prc_t * p_prc = ap_obj;

  assert (p_prc);

  TIZ_INIT_OMX_PORT_STRUCT (p_prc->port_def_,
                            ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX);

  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (p_prc)), handleOf (p_prc),
                          OMX_IndexParamPortDefinition, &(p_prc->port_def_)));

  p_prc->first_buf_ = true;
  p_prc->eos_ = false;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_prc_transfer_and_process (void * ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_prc_stop_and_return (void * ap_obj)
{
  vp8d_prc_t * p_prc = (vp8d_prc_t *) ap_obj;
  /* Release all buffers */
  release_all_headers (p_prc);
  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
vp8d_prc_buffers_ready (const void * ap_obj)
{
  vp8d_prc_t * p_prc = (vp8d_prc_t *) ap_obj;
  return decode_stream (p_prc);
}

static OMX_ERRORTYPE
vp8d_prc_port_flush (const void * ap_obj, OMX_U32 a_pid)
{
  vp8d_prc_t * p_prc = (vp8d_prc_t *) ap_obj;
  if (OMX_ALL == a_pid || ARATELIA_VP8_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      release_input_header (p_prc);
      free_codec_buffer (p_prc);
    }
  if (OMX_ALL == a_pid || ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX == a_pid)
    {
      release_output_header (p_prc);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_prc_port_disable (const void * ap_obj, OMX_U32 a_pid)
{
  vp8d_prc_t * p_prc = (vp8d_prc_t *) ap_obj;
  assert (p_prc);
  if (OMX_ALL == a_pid || ARATELIA_VP8_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      /* Release all buffers */
      release_all_headers (p_prc);
      tiz_check_omx (reset_stream_parameters (p_prc));
      p_prc->in_port_disabled_ = true;
    }
  if (OMX_ALL == a_pid || ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX == a_pid)
    {
      release_output_header (p_prc);
      p_prc->out_port_disabled_ = true;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_prc_port_enable (const void * ap_obj, OMX_U32 a_pid)
{
  vp8d_prc_t * p_prc = (vp8d_prc_t *) ap_obj;
  assert (p_prc);
  if (OMX_ALL == a_pid || ARATELIA_VP8_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      if (p_prc->in_port_disabled_)
        {
          tiz_check_omx (reset_stream_parameters (p_prc));
          p_prc->in_port_disabled_ = false;
        }
    }
  if (OMX_ALL == a_pid || ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX == a_pid)
    {
      p_prc->out_port_disabled_ = false;
    }
  return OMX_ErrorNone;
}

/*
 * vp8d_prc_class
 */

static void *
vp8d_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "vp8dprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
vp8d_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * vp8dprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "vp8dprc_class", classOf (tizprc),
     sizeof (vp8d_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, vp8d_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return vp8dprc_class;
}

void *
vp8d_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * vp8dprc_class = tiz_get_type (ap_hdl, "vp8dprc_class");
  TIZ_LOG_CLASS (vp8dprc_class);
  void * vp8dprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (vp8dprc_class, "vp8dprc", tizprc, sizeof (vp8d_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, vp8d_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, vp8d_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, vp8d_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, vp8d_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, vp8d_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, vp8d_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, vp8d_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, vp8d_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, vp8d_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, vp8d_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, vp8d_prc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return vp8dprc;
}

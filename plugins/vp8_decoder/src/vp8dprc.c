/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @brief  Tizonia OpenMAX IL - VP8 Decoder processor class
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "vp8dprc.h"
#include "vp8dprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.vp8_decoder.prc"
#endif

#define IVF_FRAME_HDR_SZ (sizeof(uint32_t) + sizeof(uint64_t))
#define RAW_FRAME_HDR_SZ (sizeof(uint32_t))
#define VP8_FOURCC       (0x00385056)

static const struct
{
  char const *name;
  vpx_codec_iface_t *iface;
  unsigned int fourcc;
  unsigned int fourcc_mask;
} ifaces[] =
{
  {
    "vp8", &vpx_codec_vp8_dx_algo, VP8_FOURCC, 0x00FFFFFF
  },
};

static unsigned int
mem_get_le16 (const void *vmem)
{
  unsigned int val;
  const unsigned char *mem = (const unsigned char *) vmem;

  val = mem[1] << 8;
  val |= mem[0];
  return val;
}

static unsigned int
mem_get_le32 (const void *vmem)
{
  unsigned int val;
  const unsigned char *mem = (const unsigned char *) vmem;

  val = mem[3] << 24;
  val |= mem[2] << 16;
  val |= mem[1] << 8;
  val |= mem[0];
  return val;
}

static int
is_raw (OMX_U8 * p_buf, unsigned int *fourcc, unsigned int *width,
        unsigned int *height, unsigned int *fps_den, unsigned int *fps_num)
{
  unsigned char buf[32];
  vpx_codec_stream_info_t si;
  int i = 0;
  int is_raw = 0;

  si.sz = sizeof (si);

  if (mem_get_le32 (buf) < 256 * 1024 * 1024)
    for (i = 0; i < sizeof (ifaces) / sizeof (ifaces[0]); i++)
      if (!vpx_codec_peek_stream_info (ifaces[i].iface, buf + 4, 32 - 4, &si))
        {
          is_raw = 1;
          *fourcc = ifaces[i].fourcc;
          *width = si.w;
          *height = si.h;
          *fps_num = 30;
          *fps_den = 1;
          break;
        }

  return is_raw;
}

static int
is_ivf (OMX_U8 * p_buf, unsigned int *fourcc, unsigned int *width,
        unsigned int *height, unsigned int *fps_den, unsigned int *fps_num)
{
  int is_ivf = 0;

  if (p_buf[0] == 'D' && p_buf[1] == 'K'
      && p_buf[2] == 'I' && p_buf[3] == 'F')
    {
      is_ivf = 1;

      if (mem_get_le16 (p_buf + 4) != 0)
        fprintf (stderr, "Error: Unrecognized IVF version! This file may not"
                 " decode properly.");

      *fourcc = mem_get_le32 (p_buf + 8);
      *width = mem_get_le16 (p_buf + 12);
      *height = mem_get_le16 (p_buf + 14);
      *fps_num = mem_get_le32 (p_buf + 16);
      *fps_den = mem_get_le32 (p_buf + 20);

      /* Some versions of vpxenc used 1/(2*fps) for the timebase, so
       * we can guess the framerate using only the timebase in this
       * case. Other files would require reading ahead to guess the
       * timebase, like we do for webm.
       */
      if (*fps_num < 1000)
        {
          /* Correct for the factor of 2 applied to the timebase in the
           * encoder.
           */
          if (*fps_num & 1)
            *fps_den <<= 1;
          else
            *fps_num >>= 1;
        }
      else
        {
          /* Don't know FPS for sure, and don't have readahead code
           * (yet?), so just default to 30fps.
           */
          *fps_num = 30;
          *fps_den = 1;
        }
    }

  return is_ivf;
}

static int
find_stream_info (OMX_U8 * p_buf,
                 vp8dprc_stream_type_t * stream,
                 unsigned int *fourcc,
                 unsigned int *width,
                 unsigned int *height,
                 unsigned int *fps_den, unsigned int *fps_num)
{
  int rc = EXIT_SUCCESS;

  if (is_ivf (p_buf, fourcc, width, height, fps_den, fps_num))
    {
      *stream = STREAM_IVF;
    }
  else if (is_raw (p_buf, fourcc, width, height, fps_den, fps_num))
    {
      *stream = STREAM_RAW;
    }
  else
    {
      rc = EXIT_FAILURE;
    }

  return rc;
}

static size_t
read_from_omx_buffer (void *ap_dst, size_t a_bytes, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  size_t to_read = a_bytes;

  assert (NULL != ap_dst);
  assert (NULL != ap_hdr);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "a_bytes [%d], nFilledLen [%d] nOffset [%d]",
           a_bytes, ap_hdr->nFilledLen, ap_hdr->nOffset);

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

  TIZ_LOG (TIZ_PRIORITY_TRACE, "a_bytes [%d], nFilledLen [%d] nOffset [%d] to_read [%d]",
           a_bytes, ap_hdr->nFilledLen, ap_hdr->nOffset, to_read);

  return to_read;
}

static OMX_BUFFERHEADERTYPE *
get_input_buffer (vp8d_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl, void *ap_krn)
{
  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_krn);

  if (NULL != ap_obj->p_inhdr_)
    {
      return ap_obj->p_inhdr_;
    }
  else
    {
      tiz_pd_set_t ports;

      TIZ_PD_ZERO (&ports);
      if (OMX_ErrorNone == tiz_krn_select (ap_krn, 1, &ports))
        {
          if (TIZ_PD_ISSET (0, &ports))
            {
              if (OMX_ErrorNone == tiz_krn_claim_buffer
                  (ap_krn, 0, 0, &ap_obj->p_inhdr_))
                {
                  TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "Claimed input HEADER [%p]..."
                            "nFilledLen [%d]", ap_obj->p_inhdr_,
                            ap_obj->p_inhdr_->nFilledLen);
                  return ap_obj->p_inhdr_;
                }
            }
        }
    }

  return NULL;
}

static OMX_BUFFERHEADERTYPE *
get_output_buffer (vp8d_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl, void *ap_krn)
{
  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_krn);

  TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "eos_ = %s", ap_obj->eos_ ? "TRUE" : "FALSE");

  if (NULL != ap_obj->p_outhdr_)
    {
      return ap_obj->p_outhdr_;
    }
  else
    {
      tiz_pd_set_t ports;

      TIZ_PD_ZERO (&ports);
      if (OMX_ErrorNone == tiz_krn_select (ap_krn, 2, &ports))
        {
          if (TIZ_PD_ISSET (1, &ports))
            {
              if (OMX_ErrorNone == tiz_krn_claim_buffer
                  (ap_krn, 1, 0, &ap_obj->p_outhdr_))
                {
                  TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "Claimed output HEADER [%p]..."
                           "nFilledLen [%d]",
                           ap_obj->p_outhdr_, ap_obj->p_outhdr_->nFilledLen);
                  return ap_obj->p_outhdr_;
                }
            }
        }
    }

  return NULL;
}

static void
buffer_emptied (vp8d_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl,
                OMX_BUFFERHEADERTYPE *ap_hdr, void *ap_krn)
{
  assert (NULL != ap_obj);
  assert (NULL != ap_hdr);
  assert (NULL != ap_krn);
  assert (ap_obj->p_inhdr_ == ap_hdr);

  TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "HEADER [%p] emptied ", ap_hdr);

  assert (ap_hdr->nFilledLen == 0);
  ap_hdr->nOffset = 0;

  if ((ap_hdr->nFlags & OMX_BUFFERFLAG_EOS) != 0)
    {
      ap_obj->eos_ = true;
    }

  tiz_krn_release_buffer (ap_krn, 0, ap_hdr);
  ap_obj->p_inhdr_ = NULL;
}

static void
buffer_filled (vp8d_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl,
               OMX_BUFFERHEADERTYPE *ap_hdr, void *ap_krn)
{
  assert (NULL != ap_obj);
  assert (NULL != ap_hdr);
  assert (NULL != ap_krn);
  assert (ap_obj->p_outhdr_ == ap_hdr);

  TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "HEADER [%p] nFilledLen [%d] ", ap_hdr,
           ap_hdr->nFilledLen);

  ap_hdr->nOffset = 0;

  if (ap_obj->eos_)
    {
      /* EOS has been received and all the input data has been consumed
       * already, so its time to propagate the EOS flag */
      ap_obj->p_outhdr_->nFlags |= OMX_BUFFERFLAG_EOS;
      /* Reset the flag so we are ready to receive a new stream */
      ap_obj->eos_ = false;
    }

  tiz_krn_release_buffer (ap_krn, 1, ap_hdr);
  ap_obj->p_outhdr_ = NULL;
}

static void
obtain_stream_info (vp8d_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl,
                    OMX_BUFFERHEADERTYPE *p_inhdr)
{
  unsigned int fourcc, width, height, fps_den, fps_num;

  find_stream_info (p_inhdr->pBuffer, &(ap_obj->stream_type_),
                    &fourcc, &width, &height, &fps_den, &fps_num);

  TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "Stream [%s] fourcc = [%d] width [%d] height [%d] "
            "fps_den [%d] fps_num [%d]",
            ap_obj->stream_type_ == STREAM_RAW ? "RAW" : "IVF",
            fourcc, width, height, fps_den, fps_num);

  if (STREAM_IVF == ap_obj->stream_type_)
    {
      /* Make sure we skip the IVF header the next time we read from the
       * buffer */
      p_inhdr->nOffset    += 32;
      p_inhdr->nFilledLen -= 32;
    }
}

static size_t
read_frame_size (vp8d_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl,
                 OMX_BUFFERHEADERTYPE *ap_inhdr)
{
  char raw_hdr[IVF_FRAME_HDR_SZ];
  size_t bytes_read = 0;
  size_t frame_size = 0;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_inhdr);
  assert (ap_inhdr->nFilledLen > 0);

  /* For both the raw and ivf formats, the frame size is the first 4 bytes
   * of the frame header.
   */
  if (0 == (bytes_read
            = read_from_omx_buffer (raw_hdr,
                                    (ap_obj->stream_type_ == STREAM_IVF
                                     ? IVF_FRAME_HDR_SZ : RAW_FRAME_HDR_SZ),
                                    ap_inhdr) != 1))
    {
      TIZ_LOGN (TIZ_PRIORITY_ERROR, ap_hdl, "Failed to read frame size");
      return 0;
    }
  else
    {
      frame_size = mem_get_le32 (raw_hdr);

      if (frame_size > 256 * 1024 * 1024)
        {
          TIZ_LOGN (TIZ_PRIORITY_ERROR, ap_hdl, "Error: Read invalid frame size [%u]",
                    (unsigned int) frame_size);
          return 0;
        }

      if (ap_obj->stream_type_ == STREAM_RAW && frame_size > 256 * 1024)
        {
          TIZ_LOGN (TIZ_PRIORITY_ERROR, ap_hdl, "Warning: Read invalid frame size [%u]",
                    (unsigned int) frame_size);
          return 0;
        }
    }

  return frame_size;
}

static bool
read_frame (vp8d_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl,
            OMX_BUFFERHEADERTYPE *p_inhdr)
{
  vp8d_codec_buffer_t *p_buf = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != p_inhdr);
  assert (p_inhdr->nFilledLen > 0);

  p_buf = &(ap_obj->codec_buf_);

  TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "p_buf->frame_size [%d] p_buf->alloc_len [%d] "
            "p_buf->filled_len [%d]", p_buf->frame_size,
            p_buf->alloc_len, p_buf->filled_len);

  if (p_buf->filled_len == 0)
    {
      if (0 != (p_buf->frame_size = read_frame_size (ap_obj, ap_hdl, p_inhdr)))
        {
          if (p_buf->frame_size > p_buf->alloc_len)
            {
              uint8_t *new_buf
                = realloc (p_buf->p_data, 2 * p_buf->frame_size);

              if (new_buf)
                {
                  p_buf->p_data = new_buf;
                  p_buf->alloc_len = 2 * p_buf->frame_size;
                }
              else
                {
                  TIZ_LOGN (TIZ_PRIORITY_ERROR, ap_hdl,
                            "Failed to (re)allocate compressed data buffer");
                  p_buf->frame_size = 0;
                }
            }
        }
    }

  if (p_buf->frame_size == 0)
    {
      TIZ_LOGN (TIZ_PRIORITY_ERROR, ap_hdl, "frame size = 0");
      return false;
    }

  if (p_buf->frame_size !=
      (p_buf->filled_len +=
       read_from_omx_buffer ((p_buf->p_data) + p_buf->filled_len,
                             p_buf->frame_size - p_buf->filled_len,
                             ap_obj->p_inhdr_)))
    {
      TIZ_LOGN (TIZ_PRIORITY_ERROR, ap_hdl, "Failed to read a full frame");
      return false;
    }

  TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "p_buf->frame_size [%d] p_buf->alloc_len [%d] "
            "p_buf->filled_len [%d]", p_buf->frame_size, p_buf->alloc_len,
            p_buf->filled_len);

  return true;
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

static void
decode_frame (vp8d_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl, void *ap_krn)
{
  vpx_codec_iter_t iter = NULL;
  vpx_image_t *img = NULL;
  vp8d_codec_buffer_t *p_buf = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_krn);

  p_buf = &(ap_obj->codec_buf_);

  if (vpx_codec_decode (&(ap_obj->vp8ctx_), p_buf->p_data,
                        (unsigned int) (p_buf->filled_len), NULL, 0))
    {
      const char *detail = vpx_codec_error_detail (&(ap_obj->vp8ctx_));
      TIZ_LOGN (TIZ_PRIORITY_ERROR, ap_hdl, "Failed to decode frame: %s - %s",
                vpx_codec_error (&(ap_obj->vp8ctx_)),
                detail ? detail : "");
    }
  else
    {
      if ((img = vpx_codec_get_frame (&(ap_obj->vp8ctx_), &iter)))
        {
          unsigned int y;
          uint8_t *buf = img->planes[VPX_PLANE_Y];

          for (y = 0; y < img->d_h; y++)
            {
              out_put (ap_obj->p_outhdr_, buf, img->d_w);
              buf += img->stride[VPX_PLANE_Y];
            }

          buf = img->planes[VPX_PLANE_U];

          for (y = 0; y < (1 + img->d_h) / 2; y++)
            {
              out_put (ap_obj->p_outhdr_, buf, (1 + img->d_w) / 2);
              buf += img->stride[VPX_PLANE_U];
            }

          buf = img->planes[VPX_PLANE_V];

          for (y = 0; y < (1 + img->d_h) / 2; y++)
            {
              out_put (ap_obj->p_outhdr_, buf, (1 + img->d_w) / 2);
              buf += img->stride[VPX_PLANE_V];
            }
        }
    }

  p_buf->filled_len = 0;
}

static OMX_ERRORTYPE
decode_stream (vp8d_prc_t *ap_obj, OMX_HANDLETYPE ap_hdl, void *ap_krn)
{
  OMX_BUFFERHEADERTYPE *p_inhdr  = NULL;
  OMX_BUFFERHEADERTYPE *p_outhdr = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_krn);

  while (1)
    {
      /* Step 1: Obtain input and output buffers */
      p_inhdr  = get_input_buffer (ap_obj, ap_hdl, ap_krn);
      p_outhdr = get_output_buffer (ap_obj, ap_hdl, ap_krn);
      if (!p_inhdr || !p_outhdr)
        {
          TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "p_inhdr [%p] p_outhdr [%p]",
                    p_inhdr, p_outhdr);
          return OMX_ErrorNone;
        }

      /* This needs to be done only once */
      if (ap_obj->first_buf_)
        {
          obtain_stream_info (ap_obj, ap_hdl, p_inhdr);
          ap_obj->first_buf_ = false;
        }

      /* Step 2: Read a frame into our internal storage */
      if (p_inhdr->nFilledLen > 0)
        {
          if (read_frame (ap_obj, ap_hdl, p_inhdr))
            {
              /* Step 3: Decode the frame just read */
              decode_frame (ap_obj, ap_hdl, p_outhdr);
            }
        }

      /* Step 4: Get rid of input and output buffers, if we can */
      TIZ_LOGN (TIZ_PRIORITY_TRACE, ap_hdl, "p_inhdr->nFilledLen [%d]",
                p_inhdr->nFilledLen);

      if (p_inhdr->nFilledLen == 0)
        {
          buffer_emptied (ap_obj, ap_hdl, p_inhdr, ap_krn);
        }

      if (p_outhdr->nFilledLen > 0 || ap_obj->eos_)
        {
          buffer_filled (ap_obj, ap_hdl, p_outhdr, ap_krn);
        }
    }
}

static inline void
free_codec_buffer (vp8d_prc_t *p_obj)
{
  assert (NULL != p_obj);
  tiz_mem_free (p_obj->codec_buf_.p_data);
  tiz_mem_set (&(p_obj->codec_buf_), 0, sizeof(p_obj->codec_buf_));
}

static void
release_buffers (const void *ap_obj)
{
  vp8d_prc_t *p_obj = (vp8d_prc_t *) ap_obj;
  void *p_krn = tiz_get_krn (tiz_api_get_hdl (p_obj));

  assert (NULL != ap_obj);

  if (NULL != p_obj->p_inhdr_)
    {
      tiz_krn_release_buffer (p_krn, 0, p_obj->p_inhdr_);
      p_obj->p_inhdr_ = NULL;
    }

  if (NULL != p_obj->p_outhdr_)
    {
      tiz_krn_release_buffer (p_krn, 1, p_obj->p_outhdr_);
      p_obj->p_outhdr_ = NULL;
    }

  if (NULL != p_obj->codec_buf_.p_data)
    {
      free_codec_buffer (p_obj);
    }
}

/*
 * vp8dprc
 */

static void *
vp8d_proc_ctor (void *ap_obj, va_list * app)
{
  vp8d_prc_t *p_obj = super_ctor (vp8dprc, ap_obj, app);

  p_obj->p_inhdr_     = 0;
  p_obj->p_outhdr_    = 0;
  p_obj->first_buf_   = true;
  p_obj->eos_         = false;
  p_obj->stream_type_ = STREAM_IVF;

  tiz_mem_set (&(p_obj->codec_buf_), 0, sizeof(p_obj->codec_buf_));
  return p_obj;
}

static void *
vp8d_proc_dtor (void *ap_obj)
{
  vp8d_prc_t *p_obj = ap_obj;
  free_codec_buffer (p_obj);
  return super_dtor (vp8dprc, ap_obj);
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
vp8d_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  vp8d_prc_t *p_obj = ap_obj;
  vpx_codec_err_t err = VPX_CODEC_OK;
  int flags = 0;

  assert (NULL != ap_obj);

  /* TODO : vp8 decoder flags */
  /*   flags = (postproc ? VPX_CODEC_USE_POSTPROC : 0) | */
  /*     (ec_enabled ? VPX_CODEC_USE_ERROR_CONCEALMENT : 0); */

  /* Initialize codec */
  if (VPX_CODEC_OK
      != (err = vpx_codec_dec_init (&(p_obj->vp8ctx_),
                                    ifaces[0].iface, NULL, flags)))
    {
      TIZ_LOGN (TIZ_PRIORITY_ERROR, tiz_api_get_hdl (ap_obj),
                "[OMX_ErrorInsufficientResources] : "
                "Unable to init the vp8 decoder [%s]...",
                vpx_codec_err_to_string (err));
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_deallocate_resources (void *ap_obj)
{
  vp8d_prc_t *p_obj = ap_obj;

  assert (NULL != ap_obj);

  free_codec_buffer (p_obj);
  vpx_codec_destroy (&(p_obj->vp8ctx_));
  
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  vp8d_prc_t *p_obj = ap_obj;
  assert (NULL != ap_obj);
  p_obj->first_buf_ = true;
  p_obj->eos_      = false;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
vp8d_proc_buffers_ready (const void *ap_obj)
{
  vp8d_prc_t *p_obj = (vp8d_prc_t *) ap_obj;

  return decode_stream (p_obj, tiz_api_get_hdl (ap_obj),
                        tiz_get_krn (tiz_api_get_hdl (ap_obj)));
}

static OMX_ERRORTYPE
vp8d_proc_port_flush (const void *ap_obj, OMX_U32 a_pid)
{
  vp8d_prc_t *p_obj = (vp8d_prc_t *) ap_obj;
  /* Release all buffers, regardless of the port this is received on */
  release_buffers (p_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  vp8d_prc_t *p_obj = (vp8d_prc_t *) ap_obj;
  /* Release all buffers, regardless of the port this is received on */
  release_buffers (p_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_port_enable (const void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *vp8dprc;

OMX_ERRORTYPE
vp8d_prc_init (void)
{
  if (!vp8dprc)
    {
      tiz_check_omx_err_ret_oom (tiz_prc_init ());
      tiz_check_null_ret_oom
        (vp8dprc =
         factory_new
         (tizprc_class,
          "vp8dprc",
          tizprc,
          sizeof (vp8d_prc_t),
          ctor, vp8d_proc_ctor,
          dtor, vp8d_proc_dtor,
          tiz_srv_allocate_resources, vp8d_proc_allocate_resources,
          tiz_srv_deallocate_resources, vp8d_proc_deallocate_resources,
          tiz_srv_prepare_to_transfer, vp8d_proc_prepare_to_transfer,
          tiz_srv_transfer_and_process, vp8d_proc_transfer_and_process,
          tiz_srv_stop_and_return, vp8d_proc_stop_and_return,
          tiz_prc_buffers_ready, vp8d_proc_buffers_ready,
          tiz_prc_port_flush, vp8d_proc_port_flush,
          tiz_prc_port_disable, vp8d_proc_port_disable,
          tiz_prc_port_enable, vp8d_proc_port_enable, 0));
    }
  return OMX_ErrorNone;
}

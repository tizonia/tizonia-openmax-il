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

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include "tizkernel.h"
#include "tizscheduler.h"

#include "vp8dprc.h"
#include "vp8dprc_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.vp8_decoder.prc"
#endif

#define VP8_FOURCC (0x00385056)
static const struct
{
  char const *name;
  vpx_codec_iface_t *iface;
  unsigned int fourcc;
  unsigned int fourcc_mask;
} ifaces[] =
{
  {
"vp8", &vpx_codec_vp8_dx_algo, VP8_FOURCC, 0x00FFFFFF},};


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
is_raw (OMX_U8 * p_buf,
        unsigned int *fourcc,
        unsigned int *width,
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
is_ivf (OMX_U8 * p_buf,
        unsigned int *fourcc,
        unsigned int *width,
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
get_stream_info (OMX_U8 * p_buf,
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
read_from_omx_buffer (void *p_dst, size_t bytes, OMX_BUFFERHEADERTYPE * p_hdr)
{
  size_t to_read = bytes;

  assert (p_dst);
  assert (p_hdr);

  TIZ_LOG (TIZ_TRACE,
           "bytes [%d], nFilledLen [%d] nOffset [%d]",
           bytes, p_hdr->nFilledLen, p_hdr->nOffset);

  if (bytes)
    {
      if (p_hdr->nFilledLen < bytes)
        {
          to_read = p_hdr->nFilledLen;
        }

      if (to_read)
        {
          memcpy (p_dst, p_hdr->pBuffer + p_hdr->nOffset, to_read);
        }

      p_hdr->nFilledLen -= to_read;
      p_hdr->nOffset += to_read;
    }

  TIZ_LOG (TIZ_TRACE,
           "bytes [%d], nFilledLen [%d] nOffset [%d] to_read [%d]",
           bytes, p_hdr->nFilledLen, p_hdr->nOffset, to_read);

  return to_read;
}

#define IVF_FRAME_HDR_SZ (sizeof(uint32_t) + sizeof(uint64_t))
#define RAW_FRAME_HDR_SZ (sizeof(uint32_t))
static bool
read_frame (void *ap_obj, uint8_t ** buf, size_t * buf_sz,
            size_t * buf_alloc_sz, size_t * buf_read_sz)
{
  struct vp8dprc *p_vp8dprc = ap_obj;
  const struct tizservant *p_parent = ap_obj;
  char raw_hdr[IVF_FRAME_HDR_SZ];
  size_t new_buf_sz;
  vp8dprc_stream_type_t st = p_vp8dprc->stream_type_;
  size_t bytes_read = 0;

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "*buf_sz [%d] *buf_alloc_sz [%d] *buf_read_sz [%d]",
                 *buf_sz, *buf_alloc_sz, *buf_read_sz);

  if (*buf_read_sz == 0)
    {
      /* For both the raw and ivf formats, the frame size is the first 4 bytes
       * of the frame header.
       */
      if (0 == (bytes_read
                = read_from_omx_buffer (raw_hdr,
                                        (st == STREAM_IVF ? IVF_FRAME_HDR_SZ
                                         : RAW_FRAME_HDR_SZ),
                                        p_vp8dprc->p_inhdr_) != 1))
        {
          TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                         TIZ_CBUF (p_parent->p_hdl_),
                         "Failed to read frame size");
          new_buf_sz = 0;
        }
      else
        {
          new_buf_sz = mem_get_le32 (raw_hdr);

          if (new_buf_sz > 256 * 1024 * 1024)
            {
              TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                             TIZ_CBUF (p_parent->p_hdl_),
                             "Error: Read invalid frame size [%u]",
                             (unsigned int) new_buf_sz);
              new_buf_sz = 0;
            }

          if (st == STREAM_RAW && new_buf_sz > 256 * 1024)
            {
              TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                             TIZ_CBUF (p_parent->p_hdl_),
                             "Warning: Read invalid frame size [%u]",
                             (unsigned int) new_buf_sz);
            }

          if (new_buf_sz > *buf_alloc_sz)
            {
              uint8_t *new_buf = realloc (*buf, 2 * new_buf_sz);

              if (new_buf)
                {
                  *buf = new_buf;
                  *buf_alloc_sz = 2 * new_buf_sz;
                }
              else
                {
                  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                                 TIZ_CBUF (p_parent->p_hdl_),
                                 "Failed to allocate compressed data buffer");
                  new_buf_sz = 0;
                }
            }
        }

      *buf_sz = new_buf_sz;
    }

  if (*buf_sz == 0
      || (*buf_sz !=
          (*buf_read_sz +=
           read_from_omx_buffer ((*buf) + *buf_read_sz,
                                 *buf_sz - *buf_read_sz,
                                 p_vp8dprc->p_inhdr_))))
    {
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                     TIZ_CBUF (p_parent->p_hdl_),
                     "Failed to read full frame");
      return false;
    }
  else
    {
      *buf_read_sz = 0;
    }

  TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "*buf_sz [%d] *buf_alloc_sz [%d] *buf_read_sz [%d]",
                 *buf_sz, *buf_alloc_sz, *buf_read_sz);

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
      TIZ_LOG (TIZ_TRACE,
               "len [%d] nFilledLen [%d] nAllocLen [%d]",
               len, p_hdr->nFilledLen, p_hdr->nAllocLen);
      assert (p_hdr->nFilledLen <= p_hdr->nAllocLen);
    }

}

static void
relinquish_any_buffers_held (const void *ap_obj)
{
  struct vp8dprc *p_obj = (struct vp8dprc *) ap_obj;
  const struct tizservant *p_parent = ap_obj;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);

  if (NULL != p_obj->p_inhdr_)
    {
      tiz_kernel_relinquish_buffer (p_krn, 0, p_obj->p_inhdr_);
      p_obj->p_inhdr_ = NULL;
    }

  if (NULL != p_obj->p_outhdr_)
    {
      tiz_kernel_relinquish_buffer (p_krn, 1, p_obj->p_outhdr_);
      p_obj->p_outhdr_ = NULL;
    }

  if (NULL != p_obj->p_cbuf_)
    {
      free (p_obj->p_cbuf_);
      p_obj->p_cbuf_ = NULL;
      p_obj->cbuf_sz_ = 0;
    }
}


/*
 * vp8dprc
 */

static void *
vp8d_proc_ctor (void *ap_obj, va_list * app)
{
  struct vp8dprc *p_obj = super_ctor (vp8dprc, ap_obj, app);
  TIZ_LOG (TIZ_TRACE, "Constructing vp8dprc...[%p]", p_obj);

  p_obj->p_inhdr_ = 0;
  p_obj->p_outhdr_ = 0;
  p_obj->first_buf_ = true;
  p_obj->eos_ = false;
  p_obj->stream_type_ = STREAM_IVF;
  p_obj->p_cbuf_ = NULL;
  p_obj->cbuf_sz_ = 0;
  p_obj->cbuf_alloc_sz_ = 0;
  p_obj->cbuf_read_sz_ = 0;

  return p_obj;
}

static void *
vp8d_proc_dtor (void *ap_obj)
{
  struct vp8dprc *p_obj = ap_obj;
  TIZ_LOG (TIZ_TRACE, "Destructing vp8dprc...[%p]", p_obj);
  return super_dtor (vp8dprc, ap_obj);
}

static OMX_ERRORTYPE
transform_buffer (const void *ap_obj)
{
  struct vp8dprc *p_obj = (struct vp8dprc *) ap_obj;
  const struct tizservant *p_parent = ap_obj;

  assert (p_obj->p_outhdr_);

  if (p_obj->p_inhdr_)
    {
      if ((p_obj->p_inhdr_->nFlags & OMX_BUFFERFLAG_EOS) != 0)
        {
          p_obj->eos_ = true;
        }
    }

  if (p_obj->first_buf_)
    {
      unsigned int fourcc;
      unsigned int width;
      unsigned int height;
      unsigned int fps_den;
      unsigned int fps_num;

      get_stream_info (p_obj->p_inhdr_->pBuffer,
                       &(p_obj->stream_type_),
                       &fourcc, &width, &height, &fps_den, &fps_num);

      TIZ_LOG_CNAME (TIZ_TRACE,
                     TIZ_CNAME (p_parent->p_hdl_),
                     TIZ_CBUF (p_parent->p_hdl_),
                     "Stream [%s] fourcc = [%d] width [%d] height [%d] "
                     "fps_den [%d] fps_num [%d]",
                     p_obj->stream_type_ == STREAM_RAW ? "RAW" : "IVF",
                     fourcc, width, height, fps_den, fps_num);

      if (STREAM_IVF == p_obj->stream_type_)
        {
          /* Make sure we skip the IVF header the next time we read from the
           * buffer */
          p_obj->p_inhdr_->nOffset += 32;
          p_obj->p_inhdr_->nFilledLen -= 32;
        }
      p_obj->first_buf_ = false;


    }

  /* Decode file */
  while (read_frame (p_obj, &(p_obj->p_cbuf_), &(p_obj->cbuf_sz_),
                     &(p_obj->cbuf_alloc_sz_), &(p_obj->cbuf_read_sz_)))
    {
      vpx_codec_iter_t iter = NULL;
      vpx_image_t *img;

      if (vpx_codec_decode (&(p_obj->vp8ctx_), p_obj->p_cbuf_,
                            (unsigned int) (p_obj->cbuf_sz_), NULL, 0))
        {
          const char *detail = vpx_codec_error_detail (&(p_obj->vp8ctx_));
          TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                         TIZ_CBUF (p_parent->p_hdl_),
                         "Failed to decode frame: %s",
                         vpx_codec_error (&(p_obj->vp8ctx_)));
          if (detail)
            {
              TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                             TIZ_CBUF (p_parent->p_hdl_), "%s", detail);
            }

          return OMX_ErrorInsufficientResources;;
        }

      img = vpx_codec_get_frame (&(p_obj->vp8ctx_), &iter);

      if (img)
        {
          unsigned int y;
          uint8_t *buf = img->planes[VPX_PLANE_Y];

          for (y = 0; y < img->d_h; y++)
            {
              out_put (p_obj->p_outhdr_, buf, img->d_w);
              buf += img->stride[VPX_PLANE_Y];
            }

          buf = img->planes[VPX_PLANE_U];

          for (y = 0; y < (1 + img->d_h) / 2; y++)
            {
              out_put (p_obj->p_outhdr_, buf, (1 + img->d_w) / 2);
              buf += img->stride[VPX_PLANE_U];
            }

          buf = img->planes[VPX_PLANE_V];

          for (y = 0; y < (1 + img->d_h) / 2; y++)
            {
              out_put (p_obj->p_outhdr_, buf, (1 + img->d_w) / 2);
              buf += img->stride[VPX_PLANE_V];
            }

          {
            TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                           TIZ_CBUF (p_parent->p_hdl_),
                           "relinquishing output buffer : nFilledLen %d "
                           "nAllocLen [%d]",
                           p_obj->p_outhdr_->nFilledLen,
                           p_obj->p_outhdr_->nAllocLen);
            void *p_krn = tiz_get_krn (p_parent->p_hdl_);
            tiz_kernel_relinquish_buffer (p_krn, 1, p_obj->p_outhdr_);
            p_obj->p_outhdr_->nOffset = 0;
            p_obj->p_outhdr_ = NULL;
            break;
          }
        }
    }

  return OMX_ErrorNone;
}

/*
 * from tizservant class
 */

static OMX_ERRORTYPE
vp8d_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  struct vp8dprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;
  vpx_codec_err_t err = VPX_CODEC_OK;
  int flags = 0;
  assert (ap_obj);


  /* TODO : vp8 decoder flags */
  /*   flags = (postproc ? VPX_CODEC_USE_POSTPROC : 0) | */
  /*     (ec_enabled ? VPX_CODEC_USE_ERROR_CONCEALMENT : 0); */

  /* Initialize codec */
  if (VPX_CODEC_OK
      != (err = vpx_codec_dec_init (&(p_obj->vp8ctx_),
                                    ifaces[0].iface, NULL, flags)))
    {
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                     TIZ_CBUF (p_parent->p_hdl_),
                     "Unable to init the vp8 decoder [%s]...",
                     vpx_codec_err_to_string (err));
      return OMX_ErrorInsufficientResources;
    }

  TIZ_LOG_CNAME (TIZ_TRACE,
                 TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "Resource allocation complete..." "pid = [%d]", a_pid);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_deallocate_resources (void *ap_obj)
{
  struct vp8dprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;
  assert (ap_obj);

  (void) p_parent;
  (void) p_obj;

  vpx_codec_destroy (&(p_obj->vp8ctx_));

  TIZ_LOG_CNAME (TIZ_TRACE,
                 TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "Resource deallocation complete...");

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  struct vp8dprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;
  assert (ap_obj);

  p_obj->first_buf_ = true;

  TIZ_LOG_CNAME (TIZ_TRACE,
                 TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "Transfering buffers...pid [%d]", a_pid);

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
vp8d_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  assert (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_stop_and_return (void *ap_obj)
{
  struct vp8dprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;

  assert (ap_obj);

  (void) p_obj;
  (void) p_parent;

  return OMX_ErrorNone;
}

/*
 * from tizproc class
 */

static bool
claim_input (const void *ap_obj)
{
  const struct tizservant *p_parent = ap_obj;
  struct vp8dprc *p_obj = (struct vp8dprc *) ap_obj;
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);

  TIZ_PD_ZERO (&ports);
  TIZ_UTIL_TEST_ERR (tiz_kernel_select (p_krn, 2, &ports));

  /* We need one input buffers */
  if (TIZ_PD_ISSET (0, &ports))
    {
      TIZ_UTIL_TEST_ERR (tiz_kernel_claim_buffer
                         (p_krn, 0, 0, &p_obj->p_inhdr_));
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                     TIZ_CBUF (p_parent->p_hdl_),
                     "Claimed INPUT HEADER [%p]...", p_obj->p_inhdr_);
      return true;
    }

  TIZ_LOG_CNAME (TIZ_TRACE,
                 TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "COULD NOT CLAIM AN INPUT HEADER...");

  return false;
}

static bool
claim_output (const void *ap_obj)
{
  const struct tizservant *p_parent = ap_obj;
  struct vp8dprc *p_obj = (struct vp8dprc *) ap_obj;
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);

  TIZ_PD_ZERO (&ports);
  TIZ_UTIL_TEST_ERR (tiz_kernel_select (p_krn, 2, &ports));

  /* We need one output buffers */
  if (TIZ_PD_ISSET (1, &ports))
    {
      TIZ_UTIL_TEST_ERR (tiz_kernel_claim_buffer
                         (p_krn, 1, 0, &p_obj->p_outhdr_));
      TIZ_LOG_CNAME (TIZ_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                     TIZ_CBUF (p_parent->p_hdl_),
                     "Claimed OUTPUT HEADER [%p] BUFFER [%p] "
                     "nFilledLen [%d]...", p_obj->p_outhdr_,
                     p_obj->p_outhdr_->pBuffer, p_obj->p_outhdr_->nFilledLen);
      return true;
    }

  return false;
}

static OMX_ERRORTYPE
vp8d_proc_buffers_ready (const void *ap_obj)
{
  struct vp8dprc *p_obj = (struct vp8dprc *) ap_obj;
  const struct tizservant *p_parent = ap_obj;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);

  TIZ_LOG_CNAME (TIZ_TRACE,
                 TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_), "Buffers ready...");

  while (1)
    {

      if (!p_obj->p_inhdr_)
        {
          if (!claim_input (ap_obj) || !p_obj->p_inhdr_)
            {
              break;
            }
        }

      if (!p_obj->p_outhdr_)
        {
          if (!claim_output (ap_obj))
            {
              break;
            }
        }

      TIZ_UTIL_TEST_ERR (transform_buffer (ap_obj));
      if (p_obj->p_inhdr_ && (0 == p_obj->p_inhdr_->nFilledLen))
        {
          p_obj->p_inhdr_->nOffset = 0;
          tiz_kernel_relinquish_buffer (p_krn, 0, p_obj->p_inhdr_);
          p_obj->p_inhdr_ = NULL;
        }

    }

  if (p_obj->eos_ && p_obj->p_outhdr_)
    {
      /* EOS has been received and all the input data has been consumed
       * already, so its time to propagate the EOS flag */
      TIZ_LOG_CNAME (TIZ_TRACE,
                     TIZ_CNAME (p_parent->p_hdl_),
                     TIZ_CBUF (p_parent->p_hdl_),
                     "p_obj->eos OUTPUT HEADER [%p]...", p_obj->p_outhdr_);
      p_obj->p_outhdr_->nFlags |= OMX_BUFFERFLAG_EOS;
      tiz_kernel_relinquish_buffer (p_krn, 1, p_obj->p_outhdr_);
      p_obj->p_outhdr_ = NULL;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_port_flush (const void *ap_obj, OMX_U32 a_pid)
{
  struct vp8dprc *p_obj = (struct vp8dprc *) ap_obj;
  /* Always relinquish all held buffers, regardless of the port this is
   * received on */
  relinquish_any_buffers_held (p_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vp8d_proc_port_disable (const void *ap_obj, OMX_U32 a_pid)
{
  struct vp8dprc *p_obj = (struct vp8dprc *) ap_obj;
  /* Always relinquish all held buffers, regardless of the port this is
   * received on */
  relinquish_any_buffers_held (p_obj);
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

void
init_vp8dprc (void)
{

  if (!vp8dprc)
    {
      init_tizproc ();
      vp8dprc =
        factory_new
        (tiz_proc_class,
         "vp8dprc",
         tizproc,
         sizeof (struct vp8dprc),
         ctor, vp8d_proc_ctor,
         dtor, vp8d_proc_dtor,
         tiz_servant_allocate_resources, vp8d_proc_allocate_resources,
         tiz_servant_deallocate_resources, vp8d_proc_deallocate_resources,
         tiz_servant_prepare_to_transfer, vp8d_proc_prepare_to_transfer,
         tiz_servant_transfer_and_process, vp8d_proc_transfer_and_process,
         tiz_servant_stop_and_return, vp8d_proc_stop_and_return,
         tiz_proc_buffers_ready, vp8d_proc_buffers_ready,
         tiz_proc_port_flush, vp8d_proc_port_flush,
         tiz_proc_port_disable, vp8d_proc_port_disable,
         tiz_proc_port_enable, vp8d_proc_port_enable, 0);
    }

}

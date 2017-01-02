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
 * @file   oggmuxfltprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Ogg muxer filter processor
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <alloca.h>

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "oggmux.h"
#include "oggmuxfltprc.h"
#include "oggmuxfltprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ogg_muxer.filter.prc"
#endif

#define on_oggz_error_ret_omx_oom(expr)                    \
  do                                                       \
    {                                                      \
      int oggz_error = OGGZ_ERR_OK;                        \
      if (OGGZ_ERR_OK != (oggz_error = (expr)))            \
        {                                                  \
          TIZ_ERROR (handleOf (ap_prc),                    \
                     "[OMX_ErrorInsufficientResources] : " \
                     "oggz error (%d)",                    \
                     oggz_error);                          \
          return OMX_ErrorInsufficientResources;           \
        }                                                  \
    }                                                      \
  while (0)

#define OGGMUXFLT_LOG_STATE(ap_prc)                                          \
  do                                                                         \
    {                                                                        \
      OMX_BUFFERHEADERTYPE * p_a_hdr = get_aud_hdr (ap_prc);                 \
      OMX_BUFFERHEADERTYPE * p_v_hdr = get_vid_hdr (ap_prc);                 \
      OMX_BUFFERHEADERTYPE * p_o_hdr = get_out_hdr (ap_prc);                 \
      TIZ_DEBUG (handleOf (ap_prc),                                          \
                 "eos [%s] aud flen [%d] vid flen [%d] out fLen [%d]",       \
                 (tiz_filter_prc_is_eos (ap_prc) ? "YES" : "NO"),            \
                 ((p_a_hdr != NULL) ? TIZ_OMX_BUF_FILL_LEN (p_a_hdr) : -1),  \
                 ((p_v_hdr != NULL) ? TIZ_OMX_BUF_FILL_LEN (p_v_hdr) : -1),  \
                 ((p_o_hdr != NULL) ? TIZ_OMX_BUF_FILL_LEN (p_o_hdr) : -1)); \
    }                                                                        \
  while (0)

/* Forward declarations */
static OMX_ERRORTYPE
oggmuxflt_prc_deallocate_resources (void *);
static OMX_ERRORTYPE
release_output_header (oggmuxflt_prc_t *, OMX_BUFFERHEADERTYPE *);
static OMX_ERRORTYPE
release_input_header (oggmuxflt_prc_t *, const OMX_U32, OMX_BUFFERHEADERTYPE *);

static inline OMX_BUFFERHEADERTYPE *
get_aud_hdr (oggmuxflt_prc_t * ap_prc)
{
  assert (ap_prc);
  return tiz_filter_prc_get_header (ap_prc,
                                    ARATELIA_OGG_MUXER_FILTER_PORT_0_INDEX);
}

static inline OMX_BUFFERHEADERTYPE *
get_vid_hdr (oggmuxflt_prc_t * ap_prc)
{
  assert (ap_prc);
  return tiz_filter_prc_get_header (ap_prc,
                                    ARATELIA_OGG_MUXER_FILTER_PORT_1_INDEX);
}

static inline OMX_BUFFERHEADERTYPE *
get_out_hdr (oggmuxflt_prc_t * ap_prc)
{
  assert (ap_prc);
  return tiz_filter_prc_get_header (ap_prc,
                                    ARATELIA_OGG_MUXER_FILTER_PORT_2_INDEX);
}

static inline void
le32 (unsigned char * p, int v)
{
  p[0] = v & 0xff;
  p[1] = (v >> 8) & 0xff;
  p[2] = (v >> 16) & 0xff;
  p[3] = (v >> 24) & 0xff;
}

/* write a little-endian 16 bit int */
static inline void
le16 (unsigned char * p, int v)
{
  p[0] = v & 0xff;
  p[1] = (v >> 8) & 0xff;
}

/* OpusHead packet */
static OMX_ERRORTYPE
enqueue_opus_head (oggmuxflt_prc_t * ap_prc)
{
  int size = 19;
  unsigned char * data = tiz_mem_calloc (size, sizeof (unsigned char));
  ogg_packet op;

  OGGMUXFLT_LOG_STATE (ap_prc);

  tiz_check_null_ret_oom (data);

  memcpy (data, "OpusHead", 8); /* identifier */
  data[8] = 1;                  /* version */
  data[9] = 2;                  /* channels */
  le16 (data + 10, 0);          /* pre-skip */
  le32 (data + 12, 48000);      /* original sample rate */
  le16 (data + 16, 0);          /* gain */
  data[18] = 0;                 /* channel mapping family */

  op.packet = data;
  op.bytes = size;
  op.b_o_s = 1;
  op.e_o_s = 0;
  op.granulepos = 0;
  op.packetno = 0;

  assert (ap_prc);
  on_oggz_error_ret_omx_oom (oggz_write_feed (ap_prc->p_oggz_, &op,
                                              ap_prc->oggz_audio_serialno_,
                                              OGGZ_FLUSH_AFTER, NULL));
  ap_prc->oggz_audio_packetno_++;

  tiz_mem_free (data);

  return OMX_ErrorNone;
}

/* A generic OpusTags packet */
static OMX_ERRORTYPE
enqueue_opus_tags (oggmuxflt_prc_t * ap_prc)
{
  ogg_packet op;
  char * identifier = "OpusTags";
  char * vendor = "Tizonia";
  int size = strlen (identifier) + 4 + strlen (vendor) + 4;
  unsigned char * data = tiz_mem_calloc (size, sizeof (unsigned char));

  OGGMUXFLT_LOG_STATE (ap_prc);

  tiz_check_null_ret_oom (data);

  memcpy (data, identifier, 8);
  le32 (data + 8, strlen (vendor));
  memcpy (data + 12, vendor, strlen (vendor));
  le32 (data + 12 + strlen (vendor), 0);

  op.packet = data;
  op.bytes = size;
  op.b_o_s = 0;
  op.e_o_s = 0;
  op.granulepos = 0;
  op.packetno = 1;

  assert (ap_prc);
  on_oggz_error_ret_omx_oom (oggz_write_feed (ap_prc->p_oggz_, &op,
                                              ap_prc->oggz_audio_serialno_,
                                              OGGZ_FLUSH_AFTER, NULL));
  ap_prc->oggz_audio_packetno_++;

  tiz_mem_free (data);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
enqueue_opus_packet (oggmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNotReady;
  ogg_packet op;
  OMX_BUFFERHEADERTYPE * p_hdr = get_aud_hdr (ap_prc);

  OGGMUXFLT_LOG_STATE (ap_prc);
  if (p_hdr)
    {
      op.packet = p_hdr->pBuffer + p_hdr->nOffset;
      op.bytes = p_hdr->nFilledLen;
      op.granulepos = ap_prc->oggz_audio_granulepos_;
      op.packetno = ap_prc->oggz_audio_packetno_;
      op.b_o_s = 0;
      op.e_o_s = (((p_hdr->nFlags & OMX_BUFFERFLAG_EOS) > 0) ? 1 : 0);
      TIZ_DEBUG (handleOf (ap_prc), "written [%d]", op.bytes);
      on_oggz_error_ret_omx_oom (oggz_write_feed (ap_prc->p_oggz_, &op,
                                                  ap_prc->oggz_audio_serialno_,
                                                  OGGZ_FLUSH_AFTER, NULL));
      p_hdr->nFilledLen = 0;
      rc = release_input_header (ap_prc, ARATELIA_OGG_MUXER_FILTER_PORT_0_INDEX,
                                 p_hdr);
      ap_prc->oggz_audio_granulepos_ += 100;
      ap_prc->oggz_audio_packetno_++;
    }

  return rc;
}

static OMX_ERRORTYPE
audio_hungry (oggmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);

  if (0 == ap_prc->oggz_audio_packetno_)
    {
      rc = enqueue_opus_head (ap_prc);
    }
  else if (1 == ap_prc->oggz_audio_packetno_)
    {
      rc = enqueue_opus_tags (ap_prc);
    }
  else
    {
      rc = enqueue_opus_packet (ap_prc);
    }
  return rc;
}

static OMX_ERRORTYPE
video_hungry (oggmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNotReady;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  ogg_packet op;
  assert (ap_prc);
  if ((p_hdr = get_vid_hdr (ap_prc)))
    {
      op.packet = p_hdr->pBuffer + p_hdr->nOffset;
      op.bytes = p_hdr->nFilledLen;
      op.granulepos = ap_prc->oggz_video_granulepos_;
      op.packetno = ap_prc->oggz_video_packetno_;
      op.b_o_s = (0 == ap_prc->oggz_video_packetno_ ? 1 : 0);
      op.e_o_s = (((p_hdr->nFlags & OMX_BUFFERFLAG_EOS) > 0) ? 1 : 0);
      oggz_write_feed (ap_prc->p_oggz_, &op, ap_prc->oggz_video_serialno_,
                       OGGZ_FLUSH_AFTER, NULL);
      p_hdr->nFilledLen = 0;
      rc = release_input_header (ap_prc, ARATELIA_OGG_MUXER_FILTER_PORT_1_INDEX,
                                 p_hdr);
      ap_prc->oggz_video_granulepos_ += 1;
      ap_prc->oggz_video_packetno_++;
    }
  return rc;
}

/**
 * This is callback which Oggz will call when oggz is hungry.
 *
 * \param oggz The OGGZ handle
 * \param empty A value of 1 indicates that the packet queue is currently
 *        empty. A value of 0 indicates that the packet queue is not empty.
 * \param user_data A generic pointer provided to oggz
 * \retval 0 Continue
 * \retval non-zero Instruct Oggz to stop.
 */
static int
og_hungry (OGGZ * oggz, int empty, void * user_data)
{
  OMX_ERRORTYPE audio_rc = OMX_ErrorNone;
  OMX_ERRORTYPE video_rc = OMX_ErrorNone;
  int oggz_rc = OGGZ_ERR_STOP_ERR;
  oggmuxflt_prc_t * p_prc = user_data;
  assert (p_prc);
  TIZ_DEBUG (handleOf (p_prc), "ogg queue is [%s]",
             (empty == 0 ? "NOT EMPTY" : "EMPTY"));
  audio_rc = audio_hungry (p_prc);
  video_rc = video_hungry (p_prc);
  if (OMX_ErrorNone == audio_rc || OMX_ErrorNone == video_rc)
    {
      oggz_rc = OGGZ_ERR_OK; /* this tells oggz to continue writing */
    }
  else if (OMX_ErrorNotReady == audio_rc && OMX_ErrorNotReady == video_rc)
    {
      if (tiz_filter_prc_is_eos (p_prc))
        {
          oggz_rc = OGGZ_ERR_EOS; /* this tells oggz to stop for now */
        }
      else
        {
          oggz_rc = OGGZ_ERR_STOP_OK; /* this tells oggz to stop for now */
        }
    }
  else
    {
      oggz_rc = OGGZ_ERR_STOP_ERR; /* this tells oggz to stop permenently */
    }
  return oggz_rc;
}

static size_t
og_io_write (void * ap_user_handle, void * ap_buf, size_t n)
{
  oggmuxflt_prc_t * p_prc = ap_user_handle;
  ssize_t bytes_written = 0;

  OGGMUXFLT_LOG_STATE (p_prc);

  if (n > 0 && ap_buf)
    {
      OMX_BUFFERHEADERTYPE * p_hdr = p_hdr = get_out_hdr (p_prc);

      assert (p_prc);

      if (p_hdr)
        {
          bytes_written = MIN (n, TIZ_OMX_BUF_AVAIL (p_hdr));
          (void) memcpy (TIZ_OMX_BUF_PTR (p_hdr) + p_hdr->nFilledLen, ap_buf,
                         bytes_written);
          p_hdr->nFilledLen += bytes_written;
          TIZ_DEBUG (handleOf (p_prc), "bytes - n [%d] nFilledLen [%d]", n,
                     p_hdr->nFilledLen);
          if (0 == TIZ_OMX_BUF_AVAIL (p_hdr) || tiz_filter_prc_is_eos(p_prc))
            {
              release_output_header (p_prc, p_hdr);
            }
        }
      else
        {
          TIZ_DEBUG (handleOf (p_prc), "No output headers available");
        }
    }
  return bytes_written;
}

static int
og_io_flush (void * ap_user_handle)
{
  return 0;
}

static OMX_ERRORTYPE
alloc_oggz (oggmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);

  /* Allocate the oggz object */
  tiz_check_null_ret_oom ((ap_prc->p_oggz_ = oggz_new (OGGZ_WRITE)));

  /* Obtain the serial numbers */
  ap_prc->oggz_audio_serialno_ = oggz_serialno_new (ap_prc->p_oggz_);
  ap_prc->oggz_video_serialno_ = oggz_serialno_new (ap_prc->p_oggz_);

  /* Set the 'hungry' callback */
  on_oggz_error_ret_omx_oom (
    oggz_write_set_hungry_callback (ap_prc->p_oggz_, og_hungry, 1, ap_prc));

  /* Set the io callbacks */
  on_oggz_error_ret_omx_oom (
    oggz_io_set_write (ap_prc->p_oggz_, og_io_write, ap_prc));
  on_oggz_error_ret_omx_oom (
    oggz_io_set_flush (ap_prc->p_oggz_, og_io_flush, ap_prc));

  return rc;
}

static bool
able_to_mux (oggmuxflt_prc_t * ap_prc)
{
  bool rc = true;
  bool headers_avail = tiz_filter_prc_headers_available (ap_prc);
  if (!headers_avail)
    {
      TIZ_DEBUG (handleOf (ap_prc), "Input or Output headers not available");
      rc = false;
    }
  return rc;
}

static void
reset_stream_parameters (oggmuxflt_prc_t * ap_prc)
{
  assert (ap_prc);
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

static inline void
dealloc_oggz (
  /*@special@ */ oggmuxflt_prc_t * ap_prc)
/*@releases ap_prc->p_oggz_@ */
/*@ensures isnull ap_prc->p_oggz_@ */
{
  assert (ap_prc);
  if (ap_prc->p_oggz_)
    {
      /* TODO: delete oggz */
      /*   oggz_close (ap_prc->p_oggz_); */
      ap_prc->p_oggz_ = NULL;
    }
}

static inline OMX_ERRORTYPE
do_flush (oggmuxflt_prc_t * ap_prc, OMX_U32 a_pid)
{
  assert (ap_prc);
  TIZ_TRACE (handleOf (ap_prc), "do_flush");
  if (OMX_ALL == a_pid)
    {
      reset_stream_parameters (ap_prc);
    }
  /* Release any buffers held  */
  return tiz_filter_prc_release_header (ap_prc, a_pid);
}

static void
propagate_eos_if_required (oggmuxflt_prc_t * ap_prc,
                           OMX_BUFFERHEADERTYPE * ap_out_hdr)
{
  assert (ap_prc);
  assert (ap_out_hdr);

  /* If EOS, propagate the flag to the next component */
  if (tiz_filter_prc_is_eos (ap_prc))
    {
      ap_out_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
      tiz_filter_prc_update_eos_flag (ap_prc, false);
    }
}

/* TODO: move this functionality to tiz_filter_prc_t */
static OMX_ERRORTYPE
release_input_header (oggmuxflt_prc_t * ap_prc, const OMX_U32 a_pid,
                      OMX_BUFFERHEADERTYPE * ap_hdr)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_prc);
  assert (ARATELIA_OGG_MUXER_FILTER_PORT_0_INDEX == a_pid
          || ARATELIA_OGG_MUXER_FILTER_PORT_1_INDEX == a_pid);
  assert (ap_hdr);
  if (ap_hdr)
    {
      if ((ap_hdr->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          TIZ_DEBUG (handleOf (ap_prc), "[%p] nFlags [%d]", ap_hdr,
                     ap_hdr->nFlags);
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          ap_hdr->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
        }
      rc = tiz_filter_prc_release_header (ap_prc, a_pid);
    }
  return rc;
}

static OMX_ERRORTYPE
release_output_header (oggmuxflt_prc_t * ap_prc, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE * p_hdr = !ap_hdr ? get_out_hdr (ap_prc) : ap_hdr;
  OGGMUXFLT_LOG_STATE (ap_prc);
  if (p_hdr)
    {
      propagate_eos_if_required (ap_prc, p_hdr);
      rc = tiz_filter_prc_release_header (
        ap_prc, ARATELIA_OGG_MUXER_FILTER_PORT_2_INDEX);
    }
  return rc;
}

static OMX_ERRORTYPE
mux_streams (oggmuxflt_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  long oggz_rc = OGGZ_ERR_OK;
  while ((OGGZ_ERR_OK == oggz_rc) && (p_hdr = get_out_hdr (ap_prc)))
    {
      long n = MIN (32, TIZ_OMX_BUF_AVAIL (p_hdr));
      oggz_rc = oggz_write (ap_prc->p_oggz_, n);
      if (oggz_rc > 0)
        {
          oggz_rc = OGGZ_ERR_OK;
          TIZ_DEBUG (handleOf (ap_prc), "OGGZ_ERR_OK written [%d]", oggz_rc);
        }
      else if (0 == oggz_rc)
        {
          /* eos */
          rc = OMX_ErrorNotReady;
          TIZ_DEBUG (handleOf (ap_prc), "eos, OMX_ErrorNotReady");
        }
      else if (OGGZ_ERR_STOP_OK == oggz_rc)
        {
          rc = OMX_ErrorNotReady;
          TIZ_DEBUG (handleOf (ap_prc), "OGGZ_ERR_STOP_OK, OMX_ErrorNotReady");
        }
      else if (OGGZ_ERR_STOP_ERR == oggz_rc)
        {
          rc = OMX_ErrorInsufficientResources;
          TIZ_DEBUG (handleOf (ap_prc),
                     "OGGZ_ERR_STOP_ERR, OMX_ErrorInsufficientResources");
        }
      else
        {
          rc = OMX_ErrorInsufficientResources;
          TIZ_DEBUG (handleOf (ap_prc),
                     "OGGZ_ERR_?, OMX_ErrorInsufficientResources");
        }
      OGGMUXFLT_LOG_STATE (ap_prc);
    }
  return rc;
}

/*
 * oggmuxfltprc
 */

static void *
oggmuxflt_prc_ctor (void * ap_prc, va_list * app)
{
  oggmuxflt_prc_t * p_prc
    = super_ctor (typeOf (ap_prc, "oggmuxfltprc"), ap_prc, app);
  assert (p_prc);
  p_prc->p_oggz_ = NULL;
  p_prc->oggz_audio_serialno_ = 0;
  p_prc->oggz_video_serialno_ = 0;
  p_prc->oggz_audio_granulepos_ = 0;
  p_prc->oggz_video_granulepos_ = 0;
  p_prc->oggz_audio_packetno_ = 0;
  p_prc->oggz_video_packetno_ = 0;
  reset_stream_parameters (p_prc);
  return p_prc;
}

static void *
oggmuxflt_prc_dtor (void * ap_obj)
{
  (void) oggmuxflt_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "oggmuxfltprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
oggmuxflt_prc_allocate_resources (void * ap_prc, OMX_U32 a_pid)
{
  oggmuxflt_prc_t * p_prc = ap_prc;
  assert (p_prc);
  tiz_check_omx (alloc_oggz (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggmuxflt_prc_deallocate_resources (void * ap_prc)
{
  oggmuxflt_prc_t * p_prc = ap_prc;
  assert (p_prc);
  dealloc_oggz (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggmuxflt_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggmuxflt_prc_transfer_and_process (void * ap_prc, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggmuxflt_prc_stop_and_return (void * ap_prc)
{
  /* Do flush on all ports; this will reset the stream parameters and release
     any buffers held */
  return do_flush (ap_prc, OMX_ALL);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
oggmuxflt_prc_buffers_ready (const void * ap_prc)
{
  oggmuxflt_prc_t * p_prc = (oggmuxflt_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_prc);

  OGGMUXFLT_LOG_STATE (p_prc);

  while (able_to_mux (p_prc))
    {
      rc = mux_streams (p_prc);
      /* Return with ErrorNone if NotReady was received */
      tiz_check_true_ret_val (!(OMX_ErrorNotReady == rc), OMX_ErrorNone);
      /* Return if any other error */
      tiz_check_omx_ret_val (rc, rc);
    }
  return rc;
}

static OMX_ERRORTYPE
oggmuxflt_prc_pause (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggmuxflt_prc_resume (const void * ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
oggmuxflt_prc_port_flush (const void * ap_prc, OMX_U32 a_pid)
{
  oggmuxflt_prc_t * p_prc = (oggmuxflt_prc_t *) ap_prc;
  return do_flush (p_prc, a_pid);
}

static OMX_ERRORTYPE
oggmuxflt_prc_port_disable (const void * ap_prc, OMX_U32 a_pid)
{
  oggmuxflt_prc_t * p_prc = (oggmuxflt_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = tiz_filter_prc_release_header (p_prc, a_pid);
  reset_stream_parameters (p_prc);
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

static OMX_ERRORTYPE
oggmuxflt_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  oggmuxflt_prc_t * p_prc = (oggmuxflt_prc_t *) ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

/*
 * oggmuxflt_prc_class
 */

static void *
oggmuxflt_prc_class_ctor (void * ap_prc, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_prc, "oggmuxfltprc_class"), ap_prc, app);
}

/*
 * initialization
 */

void *
oggmuxflt_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * oggmuxfltprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizfilterprc), "oggmuxfltprc_class", classOf (tizfilterprc),
     sizeof (oggmuxflt_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, oggmuxflt_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return oggmuxfltprc_class;
}

void *
oggmuxflt_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * oggmuxfltprc_class = tiz_get_type (ap_hdl, "oggmuxfltprc_class");
  TIZ_LOG_CLASS (oggmuxfltprc_class);
  void * oggmuxfltprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (oggmuxfltprc_class, "oggmuxfltprc", tizfilterprc, sizeof (oggmuxflt_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, oggmuxflt_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, oggmuxflt_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, oggmuxflt_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, oggmuxflt_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, oggmuxflt_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, oggmuxflt_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, oggmuxflt_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, oggmuxflt_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_pause, oggmuxflt_prc_pause,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_resume, oggmuxflt_prc_resume,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, oggmuxflt_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, oggmuxflt_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, oggmuxflt_prc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return oggmuxfltprc;
}

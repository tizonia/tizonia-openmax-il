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
 * @file   opusdprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Opus decoder processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "opusd.h"
#include "opusutils.h"
#include "opusdprc.h"
#include "opusdprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"

#include "tizosal.h"

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_LRINTF
#define float2int(x) lrintf(x)
#else
#define float2int(flt) ((int)(floor(.5+flt)))
#endif

#ifndef HAVE_FMINF
#define fminf(_x,_y) ((_x)<(_y)?(_x):(_y))
#endif

#ifndef HAVE_FMAXF
#define fmaxf(_x,_y) ((_x)>(_y)?(_x):(_y))
#endif

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.opus_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE opusd_prc_deallocate_resources (void *);

static inline OMX_BUFFERHEADERTYPE **
get_header_ptr (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
  pp_hdr = (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX
            ? &(ap_prc->p_in_hdr_) : &(ap_prc->p_out_hdr_));
  assert (NULL != pp_hdr);
  return pp_hdr;
}

static inline bool *
get_port_disabled_ptr (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  bool *p_port_disabled = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
  p_port_disabled = (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX
                     ? &(ap_prc->in_port_disabled_)
                     : &(ap_prc->out_port_disabled_));
  assert (NULL != p_port_disabled);
  return p_port_disabled;
}

static OMX_BUFFERHEADERTYPE *
get_buffer (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = get_header_ptr (ap_prc, a_pid);
  bool *p_port_disabled = get_port_disabled_ptr (ap_prc, a_pid);
  assert (NULL != ap_prc);

  if (false == *p_port_disabled)
    {
      if (NULL != *pp_hdr)
        {
          TIZ_TRACE (handleOf (ap_prc),
                     "HEADER [%p] pid [%d] nFilledLen [%d] ", *pp_hdr, a_pid,
                     (*pp_hdr)->nFilledLen);
          return *pp_hdr;
        }
      else
        {
          tiz_pd_set_t ports;
          void *p_krn = NULL;

          p_krn = tiz_get_krn (handleOf (ap_prc));

          TIZ_PD_ZERO (&ports);
          if (OMX_ErrorNone == tiz_krn_select (p_krn, 2, &ports))
            {
              if (TIZ_PD_ISSET (a_pid, &ports))
                {
                  if (OMX_ErrorNone == tiz_krn_claim_buffer
                      (p_krn, a_pid, 0, pp_hdr))
                    {
                      TIZ_TRACE (handleOf (ap_prc),
                                 "Claimed HEADER [%p] pid [%d] nFilledLen [%d]",
                                 *pp_hdr, a_pid, (*pp_hdr)->nFilledLen);
                      return *pp_hdr;
                    }
                }
            }
        }
      ap_prc->awaiting_buffers_ = true;
    }

  return NULL;
}

/* TODO: Change void to a int for OOM errors */
static void
release_buffer (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = get_header_ptr (ap_prc, a_pid);
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  bool *p_eos = NULL;

  assert (NULL != ap_prc);

  p_eos = &(ap_prc->eos_);
  assert (NULL != p_eos);

  p_hdr = *pp_hdr;
  assert (NULL != p_hdr);

  p_hdr->nOffset = 0;

  TIZ_TRACE (handleOf (ap_prc), "Releasing HEADER [%p] pid [%d] "
             "nFilledLen [%d] nFlags [%d]", p_hdr, a_pid, p_hdr->nFilledLen,
             p_hdr->nFlags);

  /* TODO: Check for OOM error and issue Error Event */
  (void) tiz_krn_release_buffer
    (tiz_get_krn (handleOf (ap_prc)), a_pid, p_hdr);
  *pp_hdr = NULL;
}

static OMX_ERRORTYPE
init_opus_decoder (opusd_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in
    = get_buffer (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);

  if (NULL == p_in)
    {
      return OMX_ErrorNoMore;
    }

  {
    OMX_U8 *p_ogg_data = p_in->pBuffer + p_in->nOffset;
    const OMX_U32 nbytes = p_in->nFilledLen;
    /*If playing to audio out, default the rate to 48000
     * instead of the original rate. The original rate is
     * only important for minimizing surprise about the rate
     * of output files and preserving length, which aren't
     * relevant for playback. Many audio devices sound
     * better at 48kHz and not resampling also saves CPU. */
    ap_prc->rate_ = 48000;
    ap_prc->mapping_family_ = 0;
    ap_prc->channels_ = -1;
    ap_prc->preskip_ = 0;
    float gain = 1;
    float manual_gain = 0;
    int streams = 0;
    int quiet = 0;
    if (NULL == (ap_prc->p_opus_dec_
                 = process_opus_header (handleOf (ap_prc), p_ogg_data,
                                        nbytes, &(ap_prc->rate_),
                                        &(ap_prc->mapping_family_),
                                        &(ap_prc->channels_),
                                        &(ap_prc->preskip_), &gain,
                                        manual_gain, &streams, quiet)))
      {
        rc = OMX_ErrorInsufficientResources;
      }
    TIZ_TRACE (handleOf (ap_prc),
               "rate [%d] mapping_family [%d] channels [%d] "
               "preskip [%d] gain [%d] streams [%d]", ap_prc->rate_,
               ap_prc->mapping_family_, ap_prc->channels_, ap_prc->preskip_,
               gain, streams);
  }
  p_in->nFilledLen = 0;
  release_buffer (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
  return rc;
}

static OMX_ERRORTYPE
print_opus_comments (opusd_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_in
    = get_buffer (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);

  if (NULL == p_in)
    {
      return OMX_ErrorNoMore;
    }

  process_opus_comments (handleOf (ap_prc),
                         (char *) (p_in->pBuffer + p_in->nOffset),
                         p_in->nFilledLen);
  p_in->nFilledLen = 0;
  release_buffer (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
release_all_buffers (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  assert (NULL != ap_prc);

  if ((a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX
       || a_pid == OMX_ALL) && (NULL != ap_prc->p_in_hdr_))
    {
      void *p_krn = tiz_get_krn (handleOf (ap_prc));
      tiz_check_omx_err
        (tiz_krn_release_buffer (p_krn,
                                 ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX,
                                 ap_prc->p_in_hdr_));
      ap_prc->p_in_hdr_ = NULL;
    }

  if ((a_pid == ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX
       || a_pid == OMX_ALL) && (NULL != ap_prc->p_out_hdr_))
    {
      void *p_krn = tiz_get_krn (handleOf (ap_prc));
      tiz_check_omx_err
        (tiz_krn_release_buffer (p_krn,
                                 ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX,
                                 ap_prc->p_out_hdr_));
      ap_prc->p_out_hdr_ = NULL;
    }

  ap_prc->awaiting_buffers_ = true;

  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
do_flush (opusd_prc_t * ap_prc)
{
  assert (NULL != ap_prc);
  TIZ_TRACE (handleOf (ap_prc), "do_flush");
  /* Release any buffers held  */
  return release_all_buffers (ap_prc, OMX_ALL);
}

static OMX_ERRORTYPE
transform_buffer (const opusd_prc_t * ap_prc)
{
  opusd_prc_t *p_prc = (opusd_prc_t *) ap_prc;
  OMX_BUFFERHEADERTYPE *p_in
    = get_buffer (p_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE *p_out
    = get_buffer (p_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);

  if (NULL == p_in || NULL == p_out)
    {
      return OMX_ErrorNone;
    }

  if (0 == p_in->nFilledLen)
    {
      TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFlags [%d] is empty",
                 p_in, p_in->nFlags);
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Propagate EOS flag to output */
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          p_in->nFlags = 0;
          release_buffer (p_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
        }
      release_buffer (p_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
      return OMX_ErrorNone;
    }

  {
    assert (NULL != p_prc);

    const unsigned char *p_data = p_in->pBuffer + p_in->nOffset;
    opus_int32 len = p_in->nFilledLen;
    int fec = 0;
    float *output = NULL;
    short *out = NULL;
    unsigned out_len = 0;
    int i = 0;
    int tmp_skip = 0;
    int frame_size = opus_multistream_decode_float (p_prc->p_opus_dec_, p_data,
                                                    len, p_prc->p_out_buf_,
                                                    OPUS_MAX_FRAME_SIZE, fec);

    assert (frame_size >= 0);

    tmp_skip = (p_prc->preskip_ > frame_size) ? frame_size : p_prc->preskip_;
    p_prc->preskip_ -= tmp_skip;
    output = p_prc->p_out_buf_ + p_prc->channels_ * tmp_skip;
    out_len = frame_size - tmp_skip;

    /* Convert to short and save to output file */
    out = (short *) (p_out->pBuffer + p_out->nOffset);
    for (i = 0; i < (int) out_len * p_prc->channels_; ++i)
      {
        out[i] =
          (short)
          float2int (fmaxf (-32768, fminf (output[i] * 32768.f, 32767)));
      }

    if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
      {
        /* Propagate EOS flag to output */
        p_out->nFlags |= OMX_BUFFERFLAG_EOS;
        p_in->nFlags = 0;
      }

    p_out->nFilledLen = out_len * p_prc->channels_ * 2;
    TIZ_TRACE (handleOf (ap_prc),
               "frame_size [%d] len [%d] - error [%s] nFilledLen [%d]",
               frame_size, len, opus_strerror (frame_size), p_out->nFilledLen);
    p_in->nFilledLen = 0;
    release_buffer (p_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
    release_buffer (p_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
  }
  return OMX_ErrorNone;
}

/*
 * opusdprc
 */

static void *
opusd_prc_ctor (void *ap_obj, va_list * app)
{
  opusd_prc_t *p_prc = super_ctor (typeOf (ap_obj, "opusdprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_opus_dec_ = NULL;
  p_prc->p_in_hdr_ = NULL;
  p_prc->p_out_hdr_ = NULL;
  p_prc->p_out_buf_ = NULL;
  p_prc->packet_count_ = 0;
  p_prc->rate_ = 0;
  p_prc->mapping_family_ = 0;
  p_prc->channels_ = 0;
  p_prc->preskip_ = 0;
  p_prc->eos_ = false;
  p_prc->in_port_disabled_ = false;
  p_prc->out_port_disabled_ = false;
  p_prc->awaiting_buffers_ = true;
  TIZ_TRACE (handleOf (p_prc), "Opus library vesion [%%]",
             opus_get_version_string ());
  return p_prc;
}

static void *
opusd_prc_dtor (void *ap_obj)
{
  (void) opusd_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "opusdprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
opusd_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  opusd_prc_t *p_prc = ap_obj;
/*   int error = 0; */
  assert (NULL != ap_obj);
  if (NULL == (p_prc->p_out_buf_
               =
               tiz_mem_alloc (sizeof (float) *
                              ARATELIA_OPUS_DECODER_PORT_MIN_OUTPUT_BUF_SIZE)))
    {
      return OMX_ErrorInsufficientResources;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusd_prc_deallocate_resources (void *ap_obj)
{
  opusd_prc_t *p_prc = ap_obj;
  assert (NULL != ap_obj);
  if (NULL != p_prc->p_opus_dec_)
    {
      opus_multistream_decoder_destroy (p_prc->p_opus_dec_);
      p_prc->p_opus_dec_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusd_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusd_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusd_prc_stop_and_return (void *ap_obj)
{
  opusd_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "stop_and_return");
  return do_flush (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
opusd_prc_buffers_ready (const void *ap_obj)
{
  opusd_prc_t *p_prc = (opusd_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_obj);
  TIZ_TRACE (handleOf (p_prc),
             "awaiting_buffers [%s] eos [%s] packet_count [%d]",
             p_prc->awaiting_buffers_ ? "YES" : "NO",
             p_prc->eos_ ? "YES" : "NO", p_prc->packet_count_);
  if (p_prc->awaiting_buffers_ && !p_prc->eos_)
    {
      p_prc->awaiting_buffers_ = false;

      if (0 == p_prc->packet_count_)
        {
          /* If first packet in the logical stream, process the Opus header and
           * instantiate an opus decoder with the right settings */
          rc = init_opus_decoder (p_prc);
          if (OMX_ErrorNoMore == rc)
            {
              rc = OMX_ErrorNone;
            }
          else
            {
              p_prc->packet_count_++;
            }
          p_prc->awaiting_buffers_ = true;
        }
      else if (1 == p_prc->packet_count_)
        {
          rc = print_opus_comments (p_prc);
          if (OMX_ErrorNoMore == rc)
            {
              rc = OMX_ErrorNone;
            }
          else
            {
              p_prc->packet_count_++;
            }
          p_prc->awaiting_buffers_ = true;
        }
      else
        {
          rc = transform_buffer (ap_obj);
          p_prc->packet_count_++;
          p_prc->awaiting_buffers_ = true;
        }
    }

  return rc;
}

/*
 * opusd_prc_class
 */

static void *
opusd_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "opusdprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
opusd_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *opusdprc_class = factory_new (classOf (tizprc),
                                      "opusdprc_class",
                                      classOf (tizprc),
                                      sizeof (opusd_prc_class_t),
                                      ap_tos, ap_hdl,
                                      ctor, opusd_prc_class_ctor, 0);
  return opusdprc_class;
}

void *
opusd_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizprc = tiz_get_type (ap_hdl, "tizprc");
  void *opusdprc_class = tiz_get_type (ap_hdl, "opusdprc_class");
  void *opusdprc = factory_new (opusdprc_class,
                                "opusdprc",
                                tizprc,
                                sizeof (opusd_prc_t),
                                ap_tos, ap_hdl,
                                ctor, opusd_prc_ctor,
                                dtor, opusd_prc_dtor,
                                tiz_prc_buffers_ready, opusd_prc_buffers_ready,
                                tiz_srv_allocate_resources, opusd_prc_allocate_resources,
                                tiz_srv_deallocate_resources, opusd_prc_deallocate_resources,
                                tiz_srv_prepare_to_transfer, opusd_prc_prepare_to_transfer,
                                tiz_srv_transfer_and_process, opusd_prc_transfer_and_process,
                                tiz_srv_stop_and_return, opusd_prc_stop_and_return, 0);

  return opusdprc;
}

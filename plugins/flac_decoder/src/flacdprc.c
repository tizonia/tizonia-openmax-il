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
 * @file   flacdprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - FLAC Decoder processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "flacd.h"
#include "flacdprc.h"
#include "flacdprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.flac_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE flacd_prc_deallocate_resources (void *);

static inline OMX_BUFFERHEADERTYPE **
get_header_ptr (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX);
  pp_hdr = (a_pid == ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX
            ? &(ap_prc->p_in_hdr_) : &(ap_prc->p_out_hdr_));
  assert (NULL != pp_hdr);
  return pp_hdr;
}

static inline bool *
get_port_disabled_ptr (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  bool *p_port_disabled = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX);
  p_port_disabled = (a_pid == ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX
                     ? &(ap_prc->in_port_disabled_)
                     : &(ap_prc->out_port_disabled_));
  assert (NULL != p_port_disabled);
  return p_port_disabled;
}

static OMX_BUFFERHEADERTYPE *
get_buffer (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
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
    }

  return NULL;
}

/* TODO: Change void to a int for OOM errors */
static void
release_buffer (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
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

static inline bool
buffers_available (flacd_prc_t * ap_prc)
{
  bool rc = true;
  rc &=
    (NULL != get_buffer (ap_prc, ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX));
  rc &=
    (NULL != get_buffer (ap_prc, ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX));
  return rc;
}

static OMX_ERRORTYPE
release_all_buffers (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  assert (NULL != ap_prc);

  if ((a_pid == ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX
       || a_pid == OMX_ALL) && (NULL != ap_prc->p_in_hdr_))
    {
      void *p_krn = tiz_get_krn (handleOf (ap_prc));
      tiz_check_omx_err
        (tiz_krn_release_buffer (p_krn,
                                 ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX,
                                 ap_prc->p_in_hdr_));
      ap_prc->p_in_hdr_ = NULL;
    }

  if ((a_pid == ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX
       || a_pid == OMX_ALL) && (NULL != ap_prc->p_out_hdr_))
    {
      void *p_krn = tiz_get_krn (handleOf (ap_prc));
      tiz_check_omx_err
        (tiz_krn_release_buffer (p_krn,
                                 ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX,
                                 ap_prc->p_out_hdr_));
      ap_prc->p_out_hdr_ = NULL;
    }

  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
do_flush (flacd_prc_t * ap_prc)
{
  assert (NULL != ap_prc);
  TIZ_TRACE (handleOf (ap_prc), "do_flush");
  /* Release any buffers held  */
  return release_all_buffers (ap_prc, OMX_ALL);
}

static OMX_ERRORTYPE
transform_buffers (const flacd_prc_t * ap_prc)
{
  flacd_prc_t *p_prc = (flacd_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  FLAC__bool decode_outcome = 1;

  assert (NULL != p_prc);
  assert (NULL != p_prc->p_flac_dec_);

  while (buffers_available (p_prc) && decode_outcome > 0)
  {
    TIZ_TRACE (handleOf (ap_prc), "Buffers available");
    decode_outcome = FLAC__stream_decoder_process_single (p_prc->p_flac_dec_);
    TIZ_TRACE (handleOf (ap_prc), "Buffers available : decode_outcome [%d]", decode_outcome);
  }

  return rc;
}

/*
 * flacdprc
 */

static void *
flacd_prc_ctor (void *ap_obj, va_list * app)
{
  flacd_prc_t *p_prc        = super_ctor (typeOf (ap_obj, "flacdprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_flac_dec_        = NULL;
  p_prc->p_in_hdr_          = NULL;
  p_prc->p_out_hdr_         = NULL;
  p_prc->eos_               = false;
  p_prc->in_port_disabled_  = false;
  p_prc->out_port_disabled_ = false;
  p_prc->total_samples_     = 0;
  p_prc->sample_rate_       = 0;
  p_prc->channels_          = 0;
  p_prc->bps_               = 0;
  return p_prc;
}

static void *
flacd_prc_dtor (void *ap_obj)
{
  (void) flacd_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "flacdprc"), ap_obj);
}

static FLAC__StreamDecoderReadStatus
read_cb (const FLAC__StreamDecoder *ap_decoder,
         FLAC__byte buffer[], size_t *ap_bytes,
         void *ap_client_data)
{
  flacd_prc_t *p_prc = (flacd_prc_t *) ap_client_data;
  OMX_BUFFERHEADERTYPE *p_in = NULL;
  FLAC__StreamDecoderReadStatus rc = FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;

  (void) ap_decoder;
  assert (NULL != p_prc);
  assert (NULL != ap_bytes);

  TIZ_TRACE (handleOf (p_prc), "bytes : [%d]", *ap_bytes);

  p_in = get_buffer (p_prc, ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX);

  if (NULL == p_in)
    {
      rc = FLAC__STREAM_DECODER_READ_STATUS_ABORT;
      *ap_bytes = 0;
    }
  else if (p_in->nFilledLen == 0)
    {
      rc        = FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
      *ap_bytes = 0;
    }
  else
    {
      OMX_U32 nbytes_to_copy = MIN (*ap_bytes, p_in->nFilledLen);

      memcpy (buffer, p_in->pBuffer + p_in->nOffset, nbytes_to_copy);

      p_in->nFilledLen -= nbytes_to_copy;
      p_in->nOffset    += nbytes_to_copy;
      *ap_bytes           = nbytes_to_copy;
    }

  if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
    {
      /* Propagate EOS flag to output */
      OMX_BUFFERHEADERTYPE *p_out
        = get_buffer (p_prc, ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX);
      assert (NULL != p_out);
      p_out->nFlags |= OMX_BUFFERFLAG_EOS;
      p_in->nFlags = 0;
      rc = FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }

  release_buffer (p_prc, ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX);

  return rc;
}

static FLAC__StreamDecoderWriteStatus
write_cb (const FLAC__StreamDecoder *ap_decoder, const FLAC__Frame *ap_frame,
          const FLAC__int32 *const ap_buffer[], void *ap_client_data)
{
  flacd_prc_t *p_prc = (flacd_prc_t *) ap_client_data;
  OMX_BUFFERHEADERTYPE *p_out = NULL;
  FLAC__StreamDecoderWriteStatus rc = FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;

  (void) ap_decoder;
  assert (NULL != p_prc);
  assert (NULL != ap_frame);
  assert (NULL != ap_buffer);

  TIZ_TRACE (handleOf (p_prc), "blocksize : [%d]",
             ap_frame->header.blocksize);

  p_out = get_buffer (p_prc, ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX);
  assert (NULL != p_out);

  if(p_prc->channels_ != 2 || p_prc->bps_ != 16)
    {
      TIZ_ERROR (handleOf (p_prc), "Only support for 16bit stereo streams");
      rc = FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
  else
    {
      /* write decoded PCM samples */
      int i = 0;
      for (i = 0; i < ap_frame->header.blocksize; ++i)
        {
          short *out = (short *) (p_out->pBuffer + p_out->nOffset) + i;
          out[0] = (FLAC__int16) ap_buffer[0][i]; /* left channel */
          out[1] = (FLAC__int16) ap_buffer[1][i]; /* right channel */
        }
      p_out->nFilledLen = ap_frame->header.blocksize
        * ap_frame->header.channels
        * ap_frame->header.bits_per_sample;
      release_buffer (p_prc, ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX);
    }

  return rc;
}

static void
metadata_cb(const FLAC__StreamDecoder *ap_decoder,
            const FLAC__StreamMetadata *ap_metadata, void *ap_client_data)
{
  flacd_prc_t *p_prc = (flacd_prc_t *) ap_client_data;

  (void) ap_decoder;
  assert (NULL != p_prc);
  assert (NULL != ap_metadata);

  if(ap_metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
      /* save for later */
      p_prc->total_samples_ = ap_metadata->data.stream_info.total_samples;
      p_prc->sample_rate_   = ap_metadata->data.stream_info.sample_rate;
      p_prc->channels_      = ap_metadata->data.stream_info.channels;
      p_prc->bps_           = ap_metadata->data.stream_info.bits_per_sample;

      TIZ_TRACE (handleOf (p_prc), "sample rate     : [%u] Hz", p_prc->sample_rate_);
      TIZ_TRACE (handleOf (p_prc), "channels        : [%u]", p_prc->channels_);
      TIZ_TRACE (handleOf (p_prc), "bits per sample : [%u]", p_prc->bps_);
      TIZ_TRACE (handleOf (p_prc), "total samples   : [%llu]", p_prc->total_samples_);
    }
}

static void
error_cb (const FLAC__StreamDecoder *ap_decoder,
          FLAC__StreamDecoderErrorStatus status, void *ap_client_data)
{
  flacd_prc_t *p_prc = (flacd_prc_t *) ap_client_data;
  (void) ap_decoder;
  assert (NULL != p_prc);
  TIZ_ERROR (handleOf (p_prc), "Got error callback: %s",
             FLAC__StreamDecoderErrorStatusString[status]);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
flacd_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  flacd_prc_t *p_prc = ap_obj;
  assert (NULL != ap_obj);
  if(NULL == (p_prc->p_flac_dec_ = FLAC__stream_decoder_new ()))
    {
      TIZ_ERROR (handleOf (p_prc), "[OMX_ErrorInsufficientResources] : "
                 "Unable to create a FLAC stream decoder instance.");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_deallocate_resources (void *ap_obj)
{
  flacd_prc_t *p_prc = ap_obj;
  assert (NULL != ap_obj);
  if (NULL != p_prc->p_flac_dec_)
    {
      FLAC__stream_decoder_delete (p_prc->p_flac_dec_);
      p_prc->p_flac_dec_ = NULL;
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  flacd_prc_t *p_prc = ap_obj;
  FLAC__StreamDecoderInitStatus result = FLAC__STREAM_DECODER_INIT_STATUS_OK;
  assert (NULL != ap_obj);

  if (NULL != p_prc->p_flac_dec_)
  {
    result = FLAC__stream_decoder_init_stream (p_prc->p_flac_dec_,
                                               read_cb,
                                               NULL, /* seek_callback */
                                               NULL, /* tell_callback */
                                               NULL, /* length_callback */
                                               NULL, /* eof_callback */
                                               write_cb,
                                               metadata_cb,
                                               error_cb,
                                               p_prc);
  }

  if (FLAC__STREAM_DECODER_INIT_STATUS_OK !=  result)
    {
      TIZ_ERROR (handleOf (p_prc), "[OMX_ErrorInsufficientResources] : "
                 "While initializing the FLAC stream decoder instance.");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_stop_and_return (void *ap_obj)
{
  flacd_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  TIZ_TRACE (handleOf (p_prc), "stop_and_return");

  if (NULL != p_prc->p_flac_dec_)
  {
    (void) FLAC__stream_decoder_finish(p_prc->p_flac_dec_);
  }
  return do_flush (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
flacd_prc_buffers_ready (const void *ap_obj)
{
  return transform_buffers (ap_obj);
}

/*
 * flacd_prc_class
 */

static void *
flacd_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "flacdprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
flacd_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * flacdprc_class = factory_new (classOf (tizprc),
                                    "flacdprc_class",
                                    classOf (tizprc),
                                    sizeof (flacd_prc_class_t),
                                    ap_tos, ap_hdl,
                                    ctor, flacd_prc_class_ctor, 0);
  return flacdprc_class;
}

void *
flacd_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * flacdprc_class = tiz_get_type (ap_hdl, "flacdprc_class");
  TIZ_LOG_CLASS (flacdprc_class);
  void * flacdprc =
    factory_new
    (flacdprc_class,
     "flacdprc",
     tizprc,
     sizeof (flacd_prc_t),
     ap_tos, ap_hdl,
     ctor, flacd_prc_ctor,
     dtor, flacd_prc_dtor,
     tiz_prc_buffers_ready, flacd_prc_buffers_ready,
     tiz_srv_allocate_resources, flacd_prc_allocate_resources,
     tiz_srv_deallocate_resources, flacd_prc_deallocate_resources,
     tiz_srv_prepare_to_transfer, flacd_prc_prepare_to_transfer,
     tiz_srv_transfer_and_process, flacd_prc_transfer_and_process,
     tiz_srv_stop_and_return, flacd_prc_stop_and_return, 0);

  return flacdprc;
}

/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @brief  Tizonia - FLAC Decoder processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizplatform.h>

#include <tizkernel.h>

#include "flacd.h"
#include "flacdprc.h"
#include "flacdprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.flac_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE
flacd_prc_deallocate_resources (void *);

static OMX_ERRORTYPE
alloc_temp_data_store (flacd_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX);

  assert (ap_prc);

  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_store_ == NULL);
  ap_prc->store_size_ = port_def.nBufferSize;
  tiz_check_null_ret_oom (
    (ap_prc->p_store_ = tiz_mem_alloc (ap_prc->store_size_)));

  return OMX_ErrorNone;
}

static inline void
dealloc_temp_data_store (/*@special@ */ flacd_prc_t * ap_prc)
/*@releases ap_prc->p_store_ @ */
/*@ensures isnull ap_prc->p_store_ @ */
{
  assert (ap_prc);
  tiz_mem_free (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
  ap_prc->store_size_ = 0;
  ap_prc->store_offset_ = 0;
}

static inline OMX_U8 **
get_store_ptr (flacd_prc_t * ap_prc)
{
  OMX_U8 ** pp_store = NULL;
  assert (ap_prc);
  pp_store = &(ap_prc->p_store_);
  return pp_store;
}

static inline OMX_U32 *
get_store_size_ptr (flacd_prc_t * ap_prc)
{
  OMX_U32 * p_size = NULL;
  assert (ap_prc);
  p_size = &(ap_prc->store_size_);
  return p_size;
}

static inline OMX_U32 *
get_store_offset_ptr (flacd_prc_t * ap_prc)
{
  OMX_U32 * p_offset = NULL;
  assert (ap_prc);
  p_offset = &(ap_prc->store_offset_);
  assert (p_offset);
  return p_offset;
}

static inline OMX_BUFFERHEADERTYPE **
get_header_ptr (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE ** pp_hdr = NULL;
  assert (ap_prc);
  assert (a_pid <= ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX);
  pp_hdr
    = (a_pid == ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX ? &(ap_prc->p_in_hdr_)
                                                       : &(ap_prc->p_out_hdr_));
  assert (pp_hdr);
  return pp_hdr;
}

static inline bool *
get_port_disabled_ptr (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  bool * p_port_disabled = NULL;
  assert (ap_prc);
  assert (a_pid <= ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX);
  p_port_disabled = (a_pid == ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX
                       ? &(ap_prc->in_port_disabled_)
                       : &(ap_prc->out_port_disabled_));
  assert (p_port_disabled);
  return p_port_disabled;
}

static OMX_BUFFERHEADERTYPE *
get_header (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  bool port_disabled = *(get_port_disabled_ptr (ap_prc, a_pid));

  if (!port_disabled)
    {
      OMX_BUFFERHEADERTYPE ** pp_hdr = get_header_ptr (ap_prc, a_pid);
      p_hdr = *pp_hdr;
      if (NULL == p_hdr)
        {
          if (OMX_ErrorNone
              == tiz_krn_claim_buffer (tiz_get_krn (handleOf (ap_prc)), a_pid,
                                       0, pp_hdr))
            {
              p_hdr = *pp_hdr;
              if (p_hdr)
                {
                  TIZ_TRACE (handleOf (ap_prc),
                             "Claimed HEADER [%p] pid [%d] nFilledLen [%d]",
                             p_hdr, a_pid, p_hdr->nFilledLen);
                }
            }
        }
    }

  return p_hdr;
}

static void
release_header (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE ** pp_hdr = get_header_ptr (ap_prc, a_pid);
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;

  p_hdr = *pp_hdr;
  assert (p_hdr);

  TIZ_TRACE (handleOf (ap_prc),
             "Releasing HEADER [%p] pid [%d] "
             "nFilledLen [%d] nFlags [%d]",
             p_hdr, a_pid, p_hdr->nFilledLen, p_hdr->nFlags);

  {
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    p_hdr->nOffset = 0;
    if (OMX_ErrorNone != (rc = tiz_krn_release_buffer (
                            tiz_get_krn (handleOf (ap_prc)), a_pid, p_hdr)))
      {
        TIZ_ERROR (handleOf (ap_prc),
                   "[%s] : Releasing HEADER [%p] pid [%d] "
                   "nFilledLen [%d] nFlags [%d]",
                   tiz_err_to_str (rc), p_hdr, a_pid, p_hdr->nFilledLen,
                   p_hdr->nFlags);
        assert (0);
      }
  }
  *pp_hdr = NULL;
}

static int
store_data (flacd_prc_t * ap_prc, const OMX_U8 * ap_data, OMX_U32 a_nbytes)
{
  OMX_U8 ** pp_store = NULL;
  OMX_U32 * p_offset = NULL;
  OMX_U32 * p_size = NULL;
  OMX_U32 nbytes_to_copy = 0;
  OMX_U32 nbytes_avail = 0;

  assert (ap_prc);
  assert (ap_data);

  pp_store = get_store_ptr (ap_prc);
  p_size = get_store_size_ptr (ap_prc);
  p_offset = get_store_offset_ptr (ap_prc);

  assert (pp_store && *pp_store);
  assert (p_size);
  assert (p_offset);

  nbytes_avail = *p_size - *p_offset;

  if (a_nbytes > nbytes_avail)
    {
      /* need to re-alloc */
      OMX_U8 * p_new_store = NULL;
      p_new_store = tiz_mem_realloc (*pp_store, *p_offset + a_nbytes);
      if (p_new_store)
        {
          *pp_store = p_new_store;
          *p_size = *p_offset + a_nbytes;
          nbytes_avail = *p_size - *p_offset;
          TIZ_TRACE (handleOf (ap_prc),
                     "Realloc'd data store "
                     "to new size [%d]",
                     *p_size);
        }
    }
  nbytes_to_copy = MIN (nbytes_avail, a_nbytes);
  memcpy (*pp_store + *p_offset, ap_data, nbytes_to_copy);
  *p_offset += nbytes_to_copy;

  TIZ_TRACE (handleOf (ap_prc), "bytes currently stored [%d]", *p_offset);

  return nbytes_to_copy;
}

static inline bool
input_data_available (flacd_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  bool done = false;

  assert (ap_prc);

  if (ap_prc->store_offset_ < ARATELIA_FLAC_DECODER_BUFFER_THRESHOLD)
    {
      while (!done && ((p_hdr = get_header (
                          ap_prc, ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX))))
        {
          int bytes_stored = store_data (
            ap_prc, p_hdr->pBuffer + p_hdr->nOffset, p_hdr->nFilledLen);
          p_hdr->nFilledLen -= bytes_stored;

          if ((p_hdr->nFlags & OMX_BUFFERFLAG_EOS) > 0)
            {
              ap_prc->eos_ = true;
              done = true;
              /* Clear the EOS flag */
              p_hdr->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
            }

          /* TODO: Replace assert with a check for when nFilledLen != 0 */
          assert (0 == p_hdr->nFilledLen);
          release_header (ap_prc, ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX);
        }
    }

  TIZ_TRACE (handleOf (ap_prc), "bytes available [%d]", ap_prc->store_offset_);

  return (ap_prc->store_offset_ >= ARATELIA_FLAC_DECODER_BUFFER_THRESHOLD
          || (ap_prc->eos_ && ap_prc->store_offset_ > 0));
}

static inline bool
output_buffers_available (flacd_prc_t * ap_prc)
{
  return (get_header (ap_prc, ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX));
}

static OMX_ERRORTYPE
release_all_headers (flacd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  assert (ap_prc);

  if ((a_pid == ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX || a_pid == OMX_ALL)
      && (ap_prc->p_in_hdr_))
    {
      void * p_krn = tiz_get_krn (handleOf (ap_prc));
      tiz_check_omx (tiz_krn_release_buffer (
        p_krn, ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX, ap_prc->p_in_hdr_));
      ap_prc->p_in_hdr_ = NULL;
    }

  if ((a_pid == ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX || a_pid == OMX_ALL)
      && (ap_prc->p_out_hdr_))
    {
      void * p_krn = tiz_get_krn (handleOf (ap_prc));
      tiz_check_omx (tiz_krn_release_buffer (
        p_krn, ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX, ap_prc->p_out_hdr_));
      ap_prc->p_out_hdr_ = NULL;
    }

  return OMX_ErrorNone;
}

static inline OMX_ERRORTYPE
do_flush (flacd_prc_t * ap_prc)
{
  TIZ_TRACE (handleOf (ap_prc), "do_flush");
  /* Release any buffers held  */
  return release_all_headers (ap_prc, OMX_ALL);
}

static OMX_ERRORTYPE
transform_stream (const flacd_prc_t * ap_prc)
{
  flacd_prc_t * p_prc = (flacd_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  FLAC__bool decode_ok = 1;

  assert (p_prc);
  assert (p_prc->p_flac_dec_);

  TIZ_TRACE (handleOf (ap_prc), "output buffers avail [%s]",
             output_buffers_available (p_prc) ? "YES" : "NO");
  while (decode_ok > 0 && input_data_available (p_prc)
         && output_buffers_available (p_prc))
    {
      TIZ_TRACE (handleOf (ap_prc), "decoding");
      decode_ok = FLAC__stream_decoder_process_single (p_prc->p_flac_dec_);
      TIZ_TRACE (handleOf (ap_prc), "decode_ok [%d]", decode_ok);
      if (!decode_ok)
        {
          TIZ_ERROR (handleOf (ap_prc), "error [%s]",
                     FLAC__stream_decoder_get_resolved_state_string (
                       p_prc->p_flac_dec_));
          rc = OMX_ErrorStreamCorrupt;
          break;
        }
    }

  return rc;
}

static int
dump_temp_store (flacd_prc_t * ap_prc, OMX_U8 * ap_buffer, size_t nbytes_avail)
{
  OMX_U8 * p_store = NULL;
  OMX_U32 * p_offset = NULL;
  OMX_U32 nbytes_to_copy = 0;

  assert (ap_prc);
  assert (ap_buffer);

  p_store = *(get_store_ptr (ap_prc));
  p_offset = get_store_offset_ptr (ap_prc);

  assert (p_store);
  assert (p_offset);

  nbytes_to_copy = MIN (*p_offset, nbytes_avail);

  if (nbytes_to_copy > 0)
    {
      memcpy (ap_buffer, p_store, nbytes_to_copy);
      *p_offset -= nbytes_to_copy;
      if (*p_offset > 0)
        {
          memmove (p_store, p_store + nbytes_to_copy, *p_offset);
        }
      TIZ_TRACE (handleOf (ap_prc),
                 "nbytes_to_copy [%d]"
                 "offset [%d]",
                 nbytes_to_copy, *p_offset);
    }

  return nbytes_to_copy;
}

static FLAC__StreamDecoderReadStatus
read_cb (const FLAC__StreamDecoder * ap_decoder, FLAC__byte buffer[],
         size_t * ap_bytes, void * ap_client_data)
{
  flacd_prc_t * p_prc = (flacd_prc_t *) ap_client_data;
  FLAC__StreamDecoderReadStatus rc = FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;

  (void) ap_decoder;
  assert (p_prc);
  assert (ap_bytes);

  TIZ_TRACE (handleOf (p_prc), "bytes requested : [%d]", *ap_bytes);

  if (*ap_bytes <= 0)
    {
      rc = FLAC__STREAM_DECODER_READ_STATUS_ABORT;
      *ap_bytes = 0;
    }
  else if (p_prc->store_offset_ == 0)
    {
      rc = FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
      *ap_bytes = 0;
    }
  else
    {
      rc = FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
      *ap_bytes = dump_temp_store (p_prc, buffer, *ap_bytes);
    }

  TIZ_TRACE (handleOf (p_prc), "bytes delivered [%d] rc [%d]", *ap_bytes, rc);

  return rc;
}

static void
write_pcm_block_8 (uint8_t * ap_to, const FLAC__int32 * const ap_buffer[],
                   const unsigned int a_nsamples,
                   const unsigned int a_nchannels)
{
  size_t i = 0;
  size_t j = 0;
  for (i = 0; i < a_nsamples; i += 2, ++j)
    {
      FLAC__int8 * out = (FLAC__int8 *) (ap_to) + i;
      int k;
      for (k = 0; k < a_nchannels; ++k)
        {
          out[k] = (FLAC__int8) ap_buffer[k][j];
        }
    }
}

static void
write_pcm_block_16 (uint8_t * ap_to, const FLAC__int32 * const ap_buffer[],
                    const unsigned int a_nsamples,
                    const unsigned int a_nchannels)
{
  size_t i = 0;
  size_t j = 0;
  for (i = 0; i < a_nsamples; i += 2, ++j)
    {
      FLAC__int16 * out = (FLAC__int16 *) (ap_to) + i;
      int k;
      for (k = 0; k < a_nchannels; ++k)
        {
          out[k] = (FLAC__int16) ap_buffer[k][j];
        }
    }
}

static void
write_pcm_block_24 (uint8_t * ap_to, const FLAC__int32 * const ap_buffer[],
                    const unsigned int a_nsamples,
                    const unsigned int a_nchannels)
{
  size_t i = 0;
  size_t j = 0;
  for (i = 0; i < a_nsamples; i += 2, ++j)
    {
      unsigned char * out = (unsigned char *) (ap_to) + (i * 3);
      int k;
      for (k = 0; k < a_nchannels; ++k)
        {
          unsigned long word32 = (unsigned long) ap_buffer[k][j];
          *out++ = (unsigned char) (word32 >> 0);
          *out++ = (unsigned char) (word32 >> 8);
          *out++ = (unsigned char) (word32 >> 16);
        }
    }
}

static FLAC__StreamDecoderWriteStatus
write_cb (const FLAC__StreamDecoder * ap_decoder, const FLAC__Frame * ap_frame,
          const FLAC__int32 * const ap_buffer[], void * ap_client_data)
{
  flacd_prc_t * p_prc = (flacd_prc_t *) ap_client_data;
  FLAC__StreamDecoderWriteStatus rc
    = FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;

  (void) ap_decoder;
  assert (p_prc);
  assert (ap_frame);
  assert (ap_buffer);

  TIZ_TRACE (handleOf (p_prc), "blocksize : [%d] channels [%d] bps [%d]",
             ap_frame->header.blocksize, ap_frame->header.channels,
             ap_frame->header.bits_per_sample);

  if (p_prc->channels_ > 2 || (ap_frame->header.bits_per_sample != 8
                               && ap_frame->header.bits_per_sample != 16
                               && ap_frame->header.bits_per_sample != 24))
    {
      TIZ_ERROR (handleOf (p_prc),
                 "Only stereo streams are supported"
                 "at 8, 16, or 24 bits per sample.");
      /* TODO: Signal client */
      rc = FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
  else
    {
      /* write decoded PCM samples */
      size_t nsamples = ap_frame->header.blocksize * p_prc->channels_;
      OMX_BUFFERHEADERTYPE * p_out
        = get_header (p_prc, ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX);
      assert (p_out);

      if (nsamples * (p_prc->bps_ / 8) > p_out->nAllocLen)
        {
          nsamples = p_out->nAllocLen / (p_prc->bps_ / 8);
        }
      assert (nsamples <= p_out->nAllocLen);

      {
        uint8_t * p_to = p_out->pBuffer + p_out->nOffset;

        switch (ap_frame->header.bits_per_sample)
          {
            case 8:
              {
                write_pcm_block_8 (p_to, ap_buffer, nsamples,
                                   ap_frame->header.channels);
              }
              break;
            case 16:
              {
                write_pcm_block_16 (p_to, ap_buffer, nsamples,
                                    ap_frame->header.channels);
              }
              break;
            case 24:
              {
                write_pcm_block_24 (p_to, ap_buffer, nsamples,
                                    ap_frame->header.channels);
              }
              break;
            default:
              {
                assert (0);
              }
              break;
          };

        p_out->nFilledLen = nsamples * (p_prc->bps_ / 8);
        if ((p_prc->eos_ && p_prc->store_offset_ == 0))
          {
            /* Propagate EOS flag to output */
            p_out->nFlags |= OMX_BUFFERFLAG_EOS;
            p_prc->eos_ = false;
          }
        release_header (p_prc, ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX);
      }
    }

  return rc;
}

static void
metadata_cb (const FLAC__StreamDecoder * ap_decoder,
             const FLAC__StreamMetadata * ap_metadata, void * ap_client_data)
{
  flacd_prc_t * p_prc = (flacd_prc_t *) ap_client_data;

  (void) ap_decoder;
  assert (p_prc);
  assert (ap_metadata);

  if (ap_metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
      /* save for later */
      p_prc->total_samples_ = ap_metadata->data.stream_info.total_samples;
      p_prc->sample_rate_ = ap_metadata->data.stream_info.sample_rate;
      p_prc->channels_ = ap_metadata->data.stream_info.channels;
      p_prc->bps_ = ap_metadata->data.stream_info.bits_per_sample;

      TIZ_TRACE (handleOf (p_prc), "sample rate     : [%u] Hz",
                 p_prc->sample_rate_);
      TIZ_TRACE (handleOf (p_prc), "channels        : [%u]", p_prc->channels_);
      TIZ_TRACE (handleOf (p_prc), "bits per sample : [%u]", p_prc->bps_);
      TIZ_TRACE (handleOf (p_prc), "total samples   : [%llu]",
                 p_prc->total_samples_);
    }
}

static void
error_cb (const FLAC__StreamDecoder * ap_decoder,
          FLAC__StreamDecoderErrorStatus status, void * ap_client_data)
{
  flacd_prc_t * p_prc = (flacd_prc_t *) ap_client_data;
  (void) ap_decoder;
  assert (p_prc);
  TIZ_ERROR (handleOf (p_prc), "Got error callback: %s",
             FLAC__StreamDecoderErrorStatusString[status]);
}

static void
reset_stream_parameters (flacd_prc_t * ap_prc)
{
  assert (ap_prc);
  ap_prc->total_samples_ = 0;
  ap_prc->sample_rate_ = 0;
  ap_prc->channels_ = 0;
  ap_prc->bps_ = 0;
}

/*
 * flacdprc
 */

static void *
flacd_prc_ctor (void * ap_obj, va_list * app)
{
  flacd_prc_t * p_prc = super_ctor (typeOf (ap_obj, "flacdprc"), ap_obj, app);
  assert (p_prc);
  p_prc->p_flac_dec_ = NULL;
  p_prc->p_in_hdr_ = NULL;
  p_prc->p_out_hdr_ = NULL;
  p_prc->eos_ = false;
  p_prc->in_port_disabled_ = false;
  p_prc->out_port_disabled_ = false;
  p_prc->p_store_ = NULL;
  p_prc->store_offset_ = 0;
  p_prc->store_size_ = 0;
  reset_stream_parameters (p_prc);
  return p_prc;
}

static void *
flacd_prc_dtor (void * ap_obj)
{
  (void) flacd_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "flacdprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
flacd_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  flacd_prc_t * p_prc = ap_obj;
  assert (p_prc);

  tiz_check_omx (alloc_temp_data_store (p_prc));

  if (NULL == (p_prc->p_flac_dec_ = FLAC__stream_decoder_new ()))
    {
      TIZ_ERROR (handleOf (p_prc),
                 "[OMX_ErrorInsufficientResources] : "
                 "Unable to create a FLAC stream decoder instance.");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_deallocate_resources (void * ap_obj)
{
  flacd_prc_t * p_prc = ap_obj;
  assert (p_prc);
  if (p_prc->p_flac_dec_)
    {
      FLAC__stream_decoder_delete (p_prc->p_flac_dec_);
      p_prc->p_flac_dec_ = NULL;
    }
  dealloc_temp_data_store (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_prepare_to_transfer (void * ap_obj, OMX_U32 a_pid)
{
  flacd_prc_t * p_prc = ap_obj;
  FLAC__StreamDecoderInitStatus result = FLAC__STREAM_DECODER_INIT_STATUS_OK;
  assert (p_prc);

  if (p_prc->p_flac_dec_)
    {
      result = FLAC__stream_decoder_init_stream (
        p_prc->p_flac_dec_, read_cb, NULL, /* seek_callback */
        NULL,                              /* tell_callback */
        NULL,                              /* length_callback */
        NULL,                              /* eof_callback */
        write_cb, metadata_cb, error_cb, p_prc);
    }

  if (FLAC__STREAM_DECODER_INIT_STATUS_OK != result)
    {
      TIZ_ERROR (handleOf (p_prc),
                 "[OMX_ErrorInsufficientResources] : "
                 "While initializing the FLAC stream decoder instance.");
      return OMX_ErrorInsufficientResources;
    }

  reset_stream_parameters (p_prc);
  p_prc->store_offset_ = 0;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_transfer_and_process (void * ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_stop_and_return (void * ap_obj)
{
  flacd_prc_t * p_prc = ap_obj;
  assert (p_prc);
  TIZ_TRACE (handleOf (p_prc), "stop_and_return");

  if (p_prc->p_flac_dec_)
    {
      (void) FLAC__stream_decoder_finish (p_prc->p_flac_dec_);
    }
  return do_flush (p_prc);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
flacd_prc_buffers_ready (const void * ap_obj)
{
  return transform_stream (ap_obj);
}

/*
 * flacd_prc_class
 */

static void *
flacd_prc_class_ctor (void * ap_obj, va_list * app)
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
  void * flacdprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "flacdprc_class", classOf (tizprc),
     sizeof (flacd_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, flacd_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value */
     0);
  return flacdprc_class;
}

void *
flacd_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * flacdprc_class = tiz_get_type (ap_hdl, "flacdprc_class");
  TIZ_LOG_CLASS (flacdprc_class);
  void * flacdprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (flacdprc_class, "flacdprc", tizprc, sizeof (flacd_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, flacd_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, flacd_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, flacd_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, flacd_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, flacd_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, flacd_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, flacd_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, flacd_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return flacdprc;
}

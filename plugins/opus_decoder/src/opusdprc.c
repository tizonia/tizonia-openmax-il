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
 * @file   opusdprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Opus decoder processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#include <tizplatform.h>

#include <tizkernel.h>

#include "opusd.h"
#include "opusutils.h"
#include "opusdprc.h"
#include "opusdprc_decls.h"

#ifdef HAVE_LRINTF
#define float2int(x) lrintf (x)
#else
#define float2int(flt) ((int) (floor (.5 + flt)))
#endif

#ifndef HAVE_FMINF
#define fminf(_x, _y) ((_x) < (_y) ? (_x) : (_y))
#endif

#ifndef HAVE_FMAXF
#define fmaxf(_x, _y) ((_x) > (_y) ? (_x) : (_y))
#endif

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.opus_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE
opusd_prc_deallocate_resources (void *);
static OMX_ERRORTYPE
opusd_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid);

static inline OMX_BUFFERHEADERTYPE **
get_header_ptr (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE ** pp_hdr = NULL;
  assert (ap_prc);
  assert (a_pid <= ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
  pp_hdr
    = (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX ? &(ap_prc->p_in_hdr_)
                                                       : &(ap_prc->p_out_hdr_));
  assert (pp_hdr);
  return pp_hdr;
}

static inline bool *
get_port_disabled_ptr (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  bool * p_port_disabled = NULL;
  assert (ap_prc);
  assert (a_pid <= ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
  p_port_disabled = (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX
                       ? &(ap_prc->in_port_disabled_)
                       : &(ap_prc->out_port_disabled_));
  assert (p_port_disabled);
  return p_port_disabled;
}

static OMX_BUFFERHEADERTYPE *
get_header (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  bool port_disabled = *(get_port_disabled_ptr (ap_prc, a_pid));

  if (!port_disabled)
    {
      OMX_BUFFERHEADERTYPE ** pp_hdr = get_header_ptr (ap_prc, a_pid);
      p_hdr = *pp_hdr;
      if (!p_hdr)
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

static OMX_ERRORTYPE
release_header (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE ** pp_hdr = get_header_ptr (ap_prc, a_pid);
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;

  p_hdr = *pp_hdr;
  assert (p_hdr);

  TIZ_TRACE (handleOf (ap_prc),
             "Releasing HEADER [%p] pid [%d] "
             "nFilledLen [%d] nFlags [%d]",
             p_hdr, a_pid, p_hdr->nFilledLen, p_hdr->nFlags);

  if (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX)
    {
      ap_prc->packet_count_++;
    }

  p_hdr->nOffset = 0;
  tiz_check_omx (
    tiz_krn_release_buffer (tiz_get_krn (handleOf (ap_prc)), a_pid, p_hdr));
  *pp_hdr = NULL;

  return OMX_ErrorNone;
}

static inline bool
headers_available (opusd_prc_t * ap_prc)
{
  bool rc = true;
  rc &= (NULL != get_header (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX));
  rc &= (NULL != get_header (ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX));
  return rc;
}

static OMX_ERRORTYPE
store_metadata (opusd_prc_t * ap_prc, const char * ap_header_name,
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

static void
store_stream_metadata (opusd_prc_t * ap_prc)
{
  assert (ap_prc);

  {
    char info[100];

    (void) tiz_krn_clear_metadata (tiz_get_krn (handleOf (ap_prc)));

    snprintf (info, 99, "%d Ch, %d Hz", ap_prc->channels_, ap_prc->rate_);
    info[99] = '\000';
    (void) store_metadata (ap_prc, "Opus Stream", info);
  }

  /* Signal that a new set of metadata items is available */
  (void) tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventIndexSettingChanged,
                              OMX_ALL, /* no particular port associated */
                              OMX_IndexConfigMetadataItem, /* index of the
                                                             struct that has
                                                             been modififed */
                              NULL);
}

static OMX_ERRORTYPE
update_pcm_mode (opusd_prc_t * ap_prc, const OMX_U32 a_samplerate,
                 const OMX_U32 a_channels)
{
  assert (ap_prc);
  if (a_samplerate != ap_prc->pcmmode_.nSamplingRate
      || a_channels != ap_prc->pcmmode_.nChannels)
    {
      TIZ_DEBUG (handleOf (ap_prc),
                 "Updating pcm mode : old samplerate [%d] new samplerate [%d]",
                 ap_prc->pcmmode_.nSamplingRate, a_samplerate);
      TIZ_DEBUG (handleOf (ap_prc),
                 "Updating pcm mode : old channels [%d] new channels [%d]",
                 ap_prc->pcmmode_.nChannels, a_channels);
      ap_prc->pcmmode_.nSamplingRate = a_samplerate;
      ap_prc->pcmmode_.nChannels = a_channels;
      tiz_check_omx (tiz_krn_SetParameter_internal (
        tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
        OMX_IndexParamAudioPcm, &(ap_prc->pcmmode_)));
      tiz_srv_issue_event ((OMX_PTR) ap_prc, OMX_EventPortSettingsChanged,
                           ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX,
                           OMX_IndexParamAudioPcm, /* the index of the
                                                      struct that has
                                                      been modififed */
                           NULL);
    }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
init_opus_decoder (opusd_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE * p_in
    = get_header (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);

  if (!p_in)
    {
      return OMX_ErrorNoMore;
    }

  {
    OMX_U8 * p_data = p_in->pBuffer + p_in->nOffset;
    const OMX_U32 nbytes = p_in->nFilledLen;
    /*If playing to audio out, default the rate to 48000
     * instead of the original rate. The original rate is
     * only important for minimizing surprise about the rate
     * of output files and preserving length, which aren't
     * relevant for playback. Many audio devices sound
     * better at 48kHz and not resampling also saves CPU. */
    float gain = 1;
    float manual_gain = 0;
    int streams = 0;
    int quiet = 0;
    int header_offset = 0;

    ap_prc->rate_ = 48000;
    ap_prc->mapping_family_ = 0;
    ap_prc->channels_ = -1;
    ap_prc->preskip_ = 0;

    TIZ_DEBUG (handleOf (ap_prc), "nbytes [%d] : ", nbytes);

    assert (!ap_prc->p_opus_dec_);

    header_offset = process_opus_header (
      handleOf (ap_prc), p_data, nbytes, &(ap_prc->rate_),
      &(ap_prc->mapping_family_), &(ap_prc->channels_), &(ap_prc->preskip_),
      &gain, manual_gain, &streams, &(ap_prc->p_opus_dec_), quiet);

    if (!(ap_prc->p_opus_dec_))
      {
        TIZ_ERROR (handleOf (ap_prc),
                   "[OMX_ErrorInsufficientResources] : "
                   "NULL returned by process_opus_header");
        return OMX_ErrorInsufficientResources;
      }

    TIZ_TRACE (handleOf (ap_prc),
               "rate [%d] mapping_family [%d] channels [%d] "
               "preskip [%d] gain [%f] streams [%d]",
               ap_prc->rate_, ap_prc->mapping_family_, ap_prc->channels_,
               ap_prc->preskip_, gain, streams);

    store_stream_metadata (ap_prc);
    (void) update_pcm_mode (ap_prc, ap_prc->rate_, ap_prc->channels_);

    p_in->nOffset += header_offset;
    p_in->nFilledLen -= header_offset;
    if (0 == p_in->nFilledLen)
      {
        return release_header (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
      }
  }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
parse_opus_comments (opusd_prc_t * ap_prc)
{
  int comments_offset = 0;
  OMX_BUFFERHEADERTYPE * p_in
    = get_header (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);

  if (!p_in)
    {
      return OMX_ErrorNoMore;
    }

  comments_offset
    = process_opus_comments (handleOf (ap_prc), (char *) TIZ_OMX_BUF_PTR (p_in),
                             TIZ_OMX_BUF_FILL_LEN (p_in));

  p_in->nOffset += comments_offset;
  p_in->nFilledLen -= comments_offset;

  if (0 == p_in->nFilledLen)
    {
      return release_header (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
release_headers (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  assert (ap_prc);

  if ((a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX || a_pid == OMX_ALL)
      && (ap_prc->p_in_hdr_))
    {
      tiz_check_omx (tiz_krn_release_buffer (
        tiz_get_krn (handleOf (ap_prc)), ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX,
        ap_prc->p_in_hdr_));
      ap_prc->p_in_hdr_ = NULL;
    }

  if ((a_pid == ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX || a_pid == OMX_ALL)
      && (ap_prc->p_out_hdr_))
    {
      tiz_check_omx (tiz_krn_release_buffer (
        tiz_get_krn (handleOf (ap_prc)),
        ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX, ap_prc->p_out_hdr_));
      ap_prc->p_out_hdr_ = NULL;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
transform_buffer (opusd_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE * p_in
    = get_header (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE * p_out
    = get_header (ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);

  assert (ap_prc);

  if (!p_in || !p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "IN HEADER [%p] OUT HEADER [%p]", p_in,
                 p_out);
      return OMX_ErrorNone;
    }

  if (0 == p_in->nFilledLen)
    {
      TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFlags [%d] is empty", p_in,
                 p_in->nFlags);
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Propagate EOS flag to output */
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          p_in->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
          tiz_check_omx (
            release_header (ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX));
        }
      tiz_check_omx (
        release_header (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX));
      return OMX_ErrorNone;
    }

  {
    const unsigned char * p_data = p_in->pBuffer + p_in->nOffset;
    opus_int32 len = p_in->nFilledLen;
    int fec = 0;
    float * output = NULL;
    short * out = NULL;
    unsigned out_len = 0;
    int i = 0;
    int tmp_skip = 0;
    int frame_size = opus_multistream_decode_float (ap_prc->p_opus_dec_, p_data,
                                                    len, ap_prc->p_out_buf_,
                                                    OPUS_MAX_FRAME_SIZE, fec);

    if (frame_size < 0)
      {
        TIZ_ERROR (handleOf (ap_prc), "[OMX_ErrorInsufficientResources] : [%s]",
                   opus_strerror (frame_size));
        return OMX_ErrorInsufficientResources;
      }
    else
      {
        tmp_skip
          = (ap_prc->preskip_ > frame_size) ? frame_size : ap_prc->preskip_;
        ap_prc->preskip_ -= tmp_skip;
        output = ap_prc->p_out_buf_ + ap_prc->channels_ * tmp_skip;
        out_len = frame_size - tmp_skip;

        /* Convert to short and save to output file */
        out = (short *) (p_out->pBuffer + p_out->nOffset);
        for (i = 0; i < (int) out_len * ap_prc->channels_; ++i)
          {
            out[i] = (short) float2int (
              fmaxf (-32768, fminf (output[i] * 32768.f, 32767)));
          }

        if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
          {
            /* Propagate EOS flag to output */
            p_out->nFlags |= OMX_BUFFERFLAG_EOS;
            p_in->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
          }

        p_out->nFilledLen = out_len * ap_prc->channels_ * 2;
        TIZ_TRACE (handleOf (ap_prc),
                   "frame_size [%d] len [%d] - error [%s] nFilledLen [%d]",
                   frame_size, len, opus_strerror (frame_size),
                   p_out->nFilledLen);
        p_in->nFilledLen = 0;
        tiz_check_omx (
          release_header (ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX));
        tiz_check_omx (
          release_header (ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX));
      }
  }
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
allocate_output_buffer (opusd_prc_t * ap_prc)
{
  assert (ap_prc);
  if (!ap_prc->p_out_buf_)
    {
      if (!(ap_prc->p_out_buf_ = tiz_mem_alloc (
              sizeof (float) * ARATELIA_OPUS_DECODER_PORT_MIN_OUTPUT_BUF_SIZE)))
        {
          return OMX_ErrorInsufficientResources;
        }
    }
  return OMX_ErrorNone;
}

static void
deallocate_output_buffer (opusd_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->p_out_buf_)
    {
      tiz_mem_free (ap_prc->p_out_buf_);
      ap_prc->p_out_buf_ = NULL;
    }
}

static void
reset_output_buffer (opusd_prc_t * ap_prc)
{
  assert (ap_prc);
  if (ap_prc->p_out_buf_)
    {
      tiz_mem_set (
        ap_prc->p_out_buf_, 0,
        sizeof (float) * ARATELIA_OPUS_DECODER_PORT_MIN_OUTPUT_BUF_SIZE);
    }
}

static void
reset_stream_parameters (opusd_prc_t * ap_prc)
{
  assert (ap_prc);
  TIZ_DEBUG (handleOf (ap_prc), "Resetting stream parameters");
  ap_prc->packet_count_ = 0;
  ap_prc->rate_ = 0;
  ap_prc->mapping_family_ = 0;
  ap_prc->channels_ = 0;
  ap_prc->preskip_ = 0;
  ap_prc->eos_ = false;
  ap_prc->opus_header_parsed_ = false;
  ap_prc->opus_comments_parsed_ = false;
  if (ap_prc->p_opus_dec_)
    {
      opus_multistream_decoder_ctl (ap_prc->p_opus_dec_, OPUS_RESET_STATE);
    }
  if (ap_prc->p_out_buf_)
    {
      reset_output_buffer (ap_prc);
    }
}

/*
 * opusdprc
 */

static void *
opusd_prc_ctor (void * ap_obj, va_list * app)
{
  opusd_prc_t * p_prc = super_ctor (typeOf (ap_obj, "opusdprc"), ap_obj, app);
  assert (p_prc);
  p_prc->p_opus_dec_ = NULL;
  p_prc->p_in_hdr_ = NULL;
  p_prc->p_out_hdr_ = NULL;
  p_prc->p_out_buf_ = NULL;
  reset_stream_parameters (p_prc);
  p_prc->in_port_disabled_ = false;
  p_prc->out_port_disabled_ = false;
  TIZ_TRACE (handleOf (p_prc), "Opus library vesion [%s]",
             opus_get_version_string ());
  return p_prc;
}

static void *
opusd_prc_dtor (void * ap_obj)
{
  (void) opusd_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "opusdprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
opusd_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  return allocate_output_buffer (ap_obj);
}

static OMX_ERRORTYPE
opusd_prc_deallocate_resources (void * ap_obj)
{
  opusd_prc_t * p_prc = ap_obj;
  assert (p_prc);
  if (p_prc->p_opus_dec_)
    {
      opus_multistream_decoder_destroy (p_prc->p_opus_dec_);
      p_prc->p_opus_dec_ = NULL;
    }
  deallocate_output_buffer (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusd_prc_prepare_to_transfer (void * ap_obj, OMX_U32 a_pid)
{
  opusd_prc_t * p_prc = ap_obj;
  TIZ_INIT_OMX_PORT_STRUCT (p_prc->pcmmode_,
                            ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
  tiz_check_omx (tiz_api_GetParameter (tiz_get_krn (handleOf (p_prc)),
                                       handleOf (p_prc), OMX_IndexParamAudioPcm,
                                       &(p_prc->pcmmode_)));

  TIZ_TRACE (handleOf (p_prc),
             "sample rate renderer = [%d] channels renderer = [%d]",
             p_prc->pcmmode_.nSamplingRate, p_prc->pcmmode_.nChannels);

  reset_stream_parameters (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusd_prc_transfer_and_process (void * ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusd_prc_stop_and_return (void * ap_obj)
{
  opusd_prc_t * p_obj = (opusd_prc_t *) ap_obj;
  return release_headers (p_obj, OMX_ALL);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
opusd_prc_buffers_ready (const void * ap_obj)
{
  opusd_prc_t * p_prc = (opusd_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_obj);
  TIZ_TRACE (handleOf (p_prc), "eos [%s] packet_count [%d]",
             p_prc->eos_ ? "YES" : "NO", p_prc->packet_count_);
  if (!p_prc->eos_)
    {
      if (!p_prc->opus_header_parsed_)
        {
          /* If first packet in the logical stream, process the Opus header and
           * instantiate an opus decoder with the right settings */
          rc = init_opus_decoder (p_prc);
          tiz_check_true_ret_val (!(OMX_ErrorNoMore == rc), OMX_ErrorNone);
          tiz_check_omx (rc);
          p_prc->opus_header_parsed_ = true;
        }
      if (!p_prc->opus_comments_parsed_ && p_prc->packet_count_ < 5)
        {
          rc = parse_opus_comments (p_prc);
          tiz_check_true_ret_val (!(OMX_ErrorNoMore == rc), OMX_ErrorNone);
          tiz_check_omx (rc);
          p_prc->opus_header_parsed_ = true;
        }

      while (headers_available (p_prc) && OMX_ErrorNone == rc)
        {
          rc = transform_buffer (p_prc);
        }
    }

  return rc;
}

static OMX_ERRORTYPE
opusd_prc_port_flush (const void * ap_obj, OMX_U32 a_pid)
{
  opusd_prc_t * p_obj = (opusd_prc_t *) ap_obj;
  return release_headers (p_obj, a_pid);
}

static OMX_ERRORTYPE
opusd_prc_port_disable (const void * ap_prc, OMX_U32 a_pid)
{
  opusd_prc_t * p_prc = (opusd_prc_t *) ap_prc;
  assert (p_prc);
  if (OMX_ALL == a_pid || ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      p_prc->in_port_disabled_ = true;
      (void) opusd_prc_deallocate_resources (p_prc);
      reset_stream_parameters (p_prc);
    }
  if (OMX_ALL == a_pid || ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX == a_pid)
    {
      p_prc->out_port_disabled_ = true;
    }
  return release_headers (p_prc, a_pid);
}

static OMX_ERRORTYPE
opusd_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  opusd_prc_t * p_prc = (opusd_prc_t *) ap_prc;
  assert (p_prc);
  if (OMX_ALL == a_pid || ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX == a_pid)
    {
      p_prc->in_port_disabled_ = false;
      (void) opusd_prc_allocate_resources (p_prc, a_pid);
    }
  if (OMX_ALL == a_pid || ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX == a_pid)
    {
      p_prc->out_port_disabled_ = false;
    }
  return OMX_ErrorNone;
}

/*
 * opusd_prc_class
 */

static void *
opusd_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "opusdprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
opusd_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * opusdprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "opusdprc_class", classOf (tizprc),
     sizeof (opusd_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, opusd_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return opusdprc_class;
}

void *
opusd_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * opusdprc_class = tiz_get_type (ap_hdl, "opusdprc_class");
  void * opusdprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (opusdprc_class, "opusdprc", tizprc, sizeof (opusd_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, opusd_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, opusd_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, opusd_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, opusd_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, opusd_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, opusd_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, opusd_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, opusd_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, opusd_prc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, opusd_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, opusd_prc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);

  return opusdprc;
}

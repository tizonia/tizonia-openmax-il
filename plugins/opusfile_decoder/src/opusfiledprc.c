/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   opusfiledprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Opus Decoder (libopusfile-based) processor
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "opusfiled.h"
#include "opusfiledprc.h"
#include "opusfiledprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.opusfile_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE
opusfiled_prc_deallocate_resources (void *);

static OMX_ERRORTYPE
update_pcm_mode (opusfiled_prc_t * ap_prc, const OMX_U32 a_samplerate,
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
allocate_temp_data_store (opusfiled_prc_t * ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;

  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
  tiz_check_omx (
    tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                          OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_store_ == NULL);
  return tiz_buffer_init (&(ap_prc->p_store_), port_def.nBufferSize);
}

static inline void
deallocate_temp_data_store (
  /*@special@ */ opusfiled_prc_t * ap_prc)
/*@releases ap_prc->p_store_@ */
/*@ensures isnull ap_prc->p_store_@ */
{
  assert (ap_prc);
  tiz_buffer_destroy (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
}

static OMX_ERRORTYPE
release_input_header (opusfiled_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE * p_in = tiz_filter_prc_get_header (
    ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);

  assert (ap_prc);

  if (p_in)
    {
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Let's propagate EOS flag to output */
          TIZ_TRACE (handleOf (ap_prc), "Propagating EOS flag to output");
          OMX_BUFFERHEADERTYPE * p_out = tiz_filter_prc_get_header (
            ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
          if (p_out)
            {
              p_out->nFlags |= OMX_BUFFERFLAG_EOS;
            }
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_in->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
        }
      tiz_filter_prc_release_header (ap_prc,
                                     ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
    }
  return OMX_ErrorNone;
}

static bool
store_data (opusfiled_prc_t * ap_prc)
{
  bool rc = true;

  if ((tiz_buffer_available (ap_prc->p_store_) - ap_prc->store_offset_)
      < ARATELIA_OPUS_DECODER_PORT_MIN_INPUT_BUF_SIZE)
    {
      OMX_BUFFERHEADERTYPE * p_in = tiz_filter_prc_get_header (
        ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);

      assert (ap_prc);

      if (p_in)
        {
          TIZ_TRACE (handleOf (ap_prc), "store available [%d]",
                     tiz_buffer_available (ap_prc->p_store_));
          if (tiz_buffer_push (ap_prc->p_store_, p_in->pBuffer + p_in->nOffset,
                               p_in->nFilledLen)
              < p_in->nFilledLen)
            {
              TIZ_ERROR (handleOf (ap_prc),
                         "[%s] : Unable to store all the data.",
                         tiz_err_to_str (rc));
              rc = false;
            }
          release_input_header (ap_prc);
        }
    }
  return rc;
}

static int
read_cback (void * ap_private, unsigned char * ap_ptr, int a_nbytes)
{
  opusfiled_prc_t * p_prc = ap_private;
  int bytes_read = 0;

  (void) store_data (p_prc);

  TIZ_TRACE (handleOf (p_prc),
             "decoder_inited_ [%s] store bytes [%d] offset [%d]",
             (p_prc->decoder_inited_ ? "YES" : "NO"),
             tiz_buffer_available (p_prc->p_store_), p_prc->store_offset_);

  if (tiz_buffer_available (p_prc->p_store_) > 0)
    {
      bytes_read = MIN (a_nbytes, tiz_buffer_available (p_prc->p_store_)
                                    - p_prc->store_offset_);
      memcpy (ap_ptr, tiz_buffer_get (p_prc->p_store_) + p_prc->store_offset_,
              bytes_read);
      if (p_prc->decoder_inited_)
        {
          tiz_buffer_advance (p_prc->p_store_, bytes_read);
        }
      else
        {
          p_prc->store_offset_ += bytes_read;
        }
    }
  else
    {
      TIZ_TRACE (handleOf (p_prc), "Run out of compressed data");
    }
  return bytes_read;
}

static OMX_ERRORTYPE
init_opus_decoder (opusfiled_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  if (store_data (ap_prc))
    {
      int op_error = 0;
      OpusFileCallbacks op_cbacks = {read_cback, NULL, NULL, NULL};

      ap_prc->p_opus_dec_
        = op_open_callbacks (ap_prc, &op_cbacks, NULL, 0, &op_error);

      if (0 != op_error)
        {
          TIZ_ERROR (handleOf (ap_prc),
                     "Unable to open the opus file handle (op_error = %d).",
                     op_error);
        }
      else
        {
          TIZ_TRACE (handleOf (ap_prc),
                     "decoder_inited = TRUE - store_offset [%d]",
                     ap_prc->store_offset_);
          ap_prc->decoder_inited_ = true;
          tiz_buffer_advance (ap_prc->p_store_, ap_prc->store_offset_);
          tiz_check_omx (update_pcm_mode (
            ap_prc, 48000, op_channel_count (ap_prc->p_opus_dec_, -1)));
        }
      ap_prc->store_offset_ = 0;
    }
  else
    {
      rc = OMX_ErrorInsufficientResources;
    }

  return rc;
}

static OMX_ERRORTYPE
transform_buffer (opusfiled_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE * p_out = tiz_filter_prc_get_header (
    ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);

  if (!store_data (ap_prc))
    {
      TIZ_ERROR (handleOf (ap_prc),
                 "[OMX_ErrorInsufficientResources] : "
                 "Could not store all the incoming data");
      return OMX_ErrorInsufficientResources;
    }

  if (tiz_buffer_available (ap_prc->p_store_) == 0 || NULL == p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "store bytes [%d] OUT HEADER [%p]",
                 tiz_buffer_available (ap_prc->p_store_), p_out);

      /* Propagate the EOS flag to the next component */
      if (tiz_buffer_available (ap_prc->p_store_) == 0 && p_out
          && tiz_filter_prc_is_eos (ap_prc))
        {
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          tiz_filter_prc_release_header (
            ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
          tiz_filter_prc_update_eos_flag (ap_prc, false);
        }
      return OMX_ErrorNotReady;
    }

  assert (ap_prc);
  assert (ap_prc->p_opus_dec_);

  {
    unsigned char * p_pcm = p_out->pBuffer + p_out->nOffset;
    const long len = p_out->nAllocLen;
    int samples_read
      = op_read_float_stereo (ap_prc->p_opus_dec_, (float *) p_pcm, len);
    TIZ_TRACE (handleOf (ap_prc), "samples_read [%d] ", samples_read);

    if (samples_read > 0)
      {
        p_out->nFilledLen = 2 * samples_read * sizeof (float);
        (void) tiz_filter_prc_release_header (
          ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
      }
    else
      {
        switch (samples_read)
          {
            case OP_HOLE:
              {
                TIZ_NOTICE (handleOf (ap_prc),
                            "[OP_HOLE] : "
                            "While decoding the input stream.");
              }
              break;
            default:
              {
                TIZ_ERROR (handleOf (ap_prc),
                           "[OMX_ErrorStreamCorruptFatal] : "
                           "While decoding the input stream.");
                rc = OMX_ErrorStreamCorruptFatal;
              }
          };
      }
  }

  return rc;
}

static void
reset_stream_parameters (opusfiled_prc_t * ap_prc)
{
  assert (ap_prc);
  ap_prc->decoder_inited_ = false;
  tiz_buffer_clear (ap_prc->p_store_);
  ap_prc->store_offset_ = 0;
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

/*
 * opusfiledprc
 */

static void *
opusfiled_prc_ctor (void * ap_obj, va_list * app)
{
  opusfiled_prc_t * p_prc
    = super_ctor (typeOf (ap_obj, "opusfiledprc"), ap_obj, app);
  assert (p_prc);
  p_prc->p_opus_dec_ = NULL;
  p_prc->p_store_ = NULL;
  reset_stream_parameters (p_prc);
  return p_prc;
}

static void *
opusfiled_prc_dtor (void * ap_obj)
{
  (void) opusfiled_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "opusfiledprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
opusfiled_prc_allocate_resources (void * ap_obj, OMX_U32 a_pid)
{
  return allocate_temp_data_store (ap_obj);
}

static OMX_ERRORTYPE
opusfiled_prc_deallocate_resources (void * ap_obj)
{
  opusfiled_prc_t * p_prc = ap_obj;
  deallocate_temp_data_store (p_prc);
  assert (p_prc);
  op_free (p_prc->p_opus_dec_);
  p_prc->p_opus_dec_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusfiled_prc_prepare_to_transfer (void * ap_prc, OMX_U32 a_pid)
{
  opusfiled_prc_t * p_prc = ap_prc;
  assert (ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (p_prc->pcmmode_,
                            ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
  tiz_check_omx (tiz_api_GetParameter (tiz_get_krn (handleOf (p_prc)),
                                       handleOf (p_prc), OMX_IndexParamAudioPcm,
                                       &(p_prc->pcmmode_)));

  TIZ_TRACE (handleOf (p_prc),
             "output sample rate = [%d] output channels = [%d]",
             p_prc->pcmmode_.nSamplingRate, p_prc->pcmmode_.nChannels);

  reset_stream_parameters (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusfiled_prc_transfer_and_process (void * ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
opusfiled_prc_stop_and_return (void * ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
opusfiled_prc_buffers_ready (const void * ap_obj)
{
  opusfiled_prc_t * p_prc = (opusfiled_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (ap_obj);

  if (!p_prc->decoder_inited_)
    {
      rc = init_opus_decoder (p_prc);
    }

  if (p_prc->decoder_inited_ && OMX_ErrorNone == rc)
    {
      while (OMX_ErrorNone == rc)
        {
          rc = transform_buffer (p_prc);
        }
      if (OMX_ErrorNotReady == rc)
        {
          rc = OMX_ErrorNone;
        }
    }

  return rc;
}

static OMX_ERRORTYPE
opusfiled_proc_port_flush (const void * ap_prc, OMX_U32 a_pid)
{
  opusfiled_prc_t * p_prc = (opusfiled_prc_t *) ap_prc;
  reset_stream_parameters (p_prc);
  return tiz_filter_prc_release_header (p_prc, a_pid);
}

static OMX_ERRORTYPE
opusfiled_prc_port_disable (const void * ap_prc, OMX_U32 a_pid)
{
  opusfiled_prc_t * p_prc = (opusfiled_prc_t *) ap_prc;
  OMX_ERRORTYPE rc = tiz_filter_prc_release_header (p_prc, a_pid);
  reset_stream_parameters (p_prc);
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

static OMX_ERRORTYPE
opusfiled_prc_port_enable (const void * ap_prc, OMX_U32 a_pid)
{
  opusfiled_prc_t * p_prc = (opusfiled_prc_t *) ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

/*
 * opusfiled_prc_class
 */

static void *
opusfiled_prc_class_ctor (void * ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "opusfiledprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
opusfiled_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * opusfiledprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizfilterprc), "opusfiledprc_class", classOf (tizfilterprc),
     sizeof (opusfiled_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, opusfiled_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return opusfiledprc_class;
}

void *
opusfiled_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * opusfiledprc_class = tiz_get_type (ap_hdl, "opusfiledprc_class");
  TIZ_LOG_CLASS (opusfiledprc_class);
  void * opusfiledprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (opusfiledprc_class, "opusfiledprc", tizfilterprc, sizeof (opusfiled_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, opusfiled_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, opusfiled_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, opusfiled_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, opusfiled_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, opusfiled_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, opusfiled_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, opusfiled_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, opusfiled_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_flush, opusfiled_proc_port_flush,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, opusfiled_prc_port_disable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, opusfiled_prc_port_enable,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return opusfiledprc;
}

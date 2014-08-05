/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
static OMX_ERRORTYPE opusfiled_prc_deallocate_resources (void *);

static OMX_ERRORTYPE allocate_temp_data_store (opusfiled_prc_t *ap_prc)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;

  assert (NULL != ap_prc);

  TIZ_INIT_OMX_PORT_STRUCT (port_def, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
  tiz_check_omx_err (
      tiz_api_GetParameter (tiz_get_krn (handleOf (ap_prc)), handleOf (ap_prc),
                            OMX_IndexParamPortDefinition, &port_def));

  assert (ap_prc->p_store_ == NULL);
  return tiz_buffer_init (&(ap_prc->p_store_), port_def.nBufferSize);
}

static inline void deallocate_temp_data_store (
    /*@special@ */ opusfiled_prc_t *ap_prc)
/*@releases ap_prc->p_store_@ */
/*@ensures isnull ap_prc->p_store_@ */
{
  assert (NULL != ap_prc);
  tiz_buffer_destroy (ap_prc->p_store_);
  ap_prc->p_store_ = NULL;
}

static int read_cback (void *ap_private, unsigned char *ap_ptr, int a_nbytes)
{
  opusfiled_prc_t *p_prc = ap_private;
  int bytes_to_read = 0;
  OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
      p_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);

  if (p_in)
    {
      bytes_to_read = MIN (a_nbytes, p_in->nFilledLen);
      memcpy (ap_ptr, p_in->pBuffer + p_in->nOffset, bytes_to_read);
      p_in->nOffset += bytes_to_read;
      p_in->nFilledLen -= bytes_to_read;
      if (0 == p_in->nFilledLen)
        {
          if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
            {
              /* Let's propagate EOS flag to output */
              TIZ_TRACE (handleOf (p_prc), "Propagating EOS flag to output");
              OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header (
                  p_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
              if (p_out)
                {
                  p_out->nFlags |= OMX_BUFFERFLAG_EOS;
                }
              tiz_filter_prc_update_eos_flag (p_prc, true);
              p_in->nFlags = 0;
            }
          tiz_filter_prc_release_header (
              p_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
        }
    }
  else
    {
      TIZ_TRACE (handleOf (p_prc), "Unable to get an input buffer");
    }
  return bytes_to_read;
}

static OMX_ERRORTYPE init_opus_decoder (opusfiled_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != tiz_filter_prc_get_header (
                      ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX));

  {
    int op_error = 0;
    OpusFileCallbacks op_cbacks = { read_cback, NULL, NULL, NULL };

    ap_prc->p_opus_dec_
        = op_open_callbacks (ap_prc, &op_cbacks, NULL, 0, &op_error);

    if (0 != op_error)
      {
        rc = OMX_ErrorInsufficientResources;
        TIZ_ERROR (
            handleOf (ap_prc),
            "[%s] : Unable to open the opus file handle (op_error = %d).",
            tiz_err_to_str (rc), op_error);
      }
  }

  return rc;
}

static OMX_ERRORTYPE transform_buffer (opusfiled_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE *p_out = tiz_filter_prc_get_header (
      ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);

  if (NULL == p_in || NULL == p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "IN HEADER [%p] OUT HEADER [%p]", p_in,
                 p_out);
      return OMX_ErrorNone;
    }

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_opus_dec_);

  if (0 == p_in->nFilledLen)
    {
      TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFlags [%d] is empty", p_in,
                 p_in->nFlags);
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Propagate EOS flag to output */
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_in->nFlags = 0;
          tiz_check_omx_err (tiz_filter_prc_release_header (
              ap_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX));
        }
    }
  else
    {
      unsigned char *p_pcm = p_out->pBuffer + p_out->nOffset;
      const long len = p_out->nAllocLen;
      int samples_read
          = op_read_float_stereo (ap_prc->p_opus_dec_, (float *)p_pcm, len);
      TIZ_TRACE (handleOf (ap_prc), "samples_read [%d] ", samples_read);

      if (samples_read > 0)
        {
          p_out->nFilledLen = 2 * samples_read * sizeof(float);
          (void)tiz_filter_prc_release_header (
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

static void reset_stream_parameters (opusfiled_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  ap_prc->decoder_inited_ = false;
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

/*
 * opusfiledprc
 */

static void *opusfiled_prc_ctor (void *ap_obj, va_list *app)
{
  opusfiled_prc_t *p_prc
      = super_ctor (typeOf (ap_obj, "opusfiledprc"), ap_obj, app);
  assert (NULL != p_prc);
  reset_stream_parameters (p_prc);
  return p_prc;
}

static void *opusfiled_prc_dtor (void *ap_obj)
{
  (void)opusfiled_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "opusfiledprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE opusfiled_prc_allocate_resources (void *ap_obj,
                                                     OMX_U32 a_pid)
{
  opusfiled_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  tiz_check_omx_err (allocate_temp_data_store (p_prc));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE opusfiled_prc_deallocate_resources (void *ap_obj)
{
  opusfiled_prc_t *p_prc = ap_obj;
  deallocate_temp_data_store (p_prc);
  assert (NULL != p_prc);
  op_free (p_prc->p_opus_dec_);
  p_prc->p_opus_dec_ = NULL;
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE opusfiled_prc_prepare_to_transfer (void *ap_obj,
                                                      OMX_U32 a_pid)
{
  reset_stream_parameters (ap_obj);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE opusfiled_prc_transfer_and_process (void *ap_obj,
                                                       OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE opusfiled_prc_stop_and_return (void *ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE opusfiled_prc_buffers_ready (const void *ap_obj)
{
  opusfiled_prc_t *p_prc = (opusfiled_prc_t *)ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);

  if (!tiz_filter_prc_is_eos (p_prc))
    {
      if (!p_prc->decoder_inited_
          && NULL != tiz_filter_prc_get_header (
                         p_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX))
        {
          rc = init_opus_decoder (p_prc);
          if (OMX_ErrorNone == rc)
            {
              p_prc->decoder_inited_ = true;
            }
        }
      else
        {
          while (OMX_ErrorNone == rc
                 && tiz_filter_prc_headers_available (p_prc))
            {
              rc = transform_buffer (p_prc);
            }
        }
    }

  return rc;
}

static OMX_ERRORTYPE opusfiled_proc_port_flush (const void *ap_prc, OMX_U32 a_pid)
{
  opusfiled_prc_t *p_prc = (opusfiled_prc_t *)ap_prc;
  return tiz_filter_prc_release_header (p_prc, a_pid);
}

static OMX_ERRORTYPE opusfiled_prc_port_disable (const void *ap_prc,
                                               OMX_U32 a_pid)
{
  opusfiled_prc_t *p_prc = (opusfiled_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = tiz_filter_prc_release_header (p_prc, a_pid);
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

static OMX_ERRORTYPE opusfiled_prc_port_enable (const void *ap_prc, OMX_U32 a_pid)
{
  opusfiled_prc_t *p_prc = (opusfiled_prc_t *)ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

/*
 * opusfiled_prc_class
 */

static void *opusfiled_prc_class_ctor (void *ap_obj, va_list *app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "opusfiledprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *opusfiled_prc_class_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *opusfiledprc_class = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (classOf (tizfilterprc), "opusfiledprc_class", classOf (tizfilterprc),
       sizeof(opusfiled_prc_class_t),
       /* TIZ_CLASS_COMMENT: */
       ap_tos, ap_hdl,
       /* TIZ_CLASS_COMMENT: class constructor */
       ctor, opusfiled_prc_class_ctor,
       /* TIZ_CLASS_COMMENT: stop value*/
       0);
  return opusfiledprc_class;
}

void *opusfiled_prc_init (void *ap_tos, void *ap_hdl)
{
  void *tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void *opusfiledprc_class = tiz_get_type (ap_hdl, "opusfiledprc_class");
  TIZ_LOG_CLASS (opusfiledprc_class);
  void *opusfiledprc = factory_new
      /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
      (opusfiledprc_class, "opusfiledprc", tizfilterprc, sizeof(opusfiled_prc_t),
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

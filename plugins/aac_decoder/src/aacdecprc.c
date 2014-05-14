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
 * @file   aacdecprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - AAC Decoder processor class implementation
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

#include "aacdec.h"
#include "aacdecprc_decls.h"
#include "aacdecprc.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.aac_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE aacdec_prc_deallocate_resources (void *);

static OMX_ERRORTYPE init_aac_decoder (aacdec_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  long nbytes = 0;
  OMX_BUFFERHEADERTYPE *p_in
    = tiz_filter_prc_get_header (ap_prc, ARATELIA_AAC_DECODER_INPUT_PORT_INDEX);

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_aac_dec_);
  assert (NULL != p_in);

  TIZ_DEBUG (handleOf (ap_prc), "");

  /* Get the current config */
  /* NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(hAac); */

  /* If needed change some of the values in conf */
  /* Set the new configuration */
  /* NeAACDecSetConfiguration(hAac, conf); */

  /* Initialise the library using one of the initialization functions */
  nbytes = NeAACDecInit(ap_prc->p_aac_dec_, p_in->pBuffer, p_in->nFilledLen,
                        &(ap_prc->samplerate_), &(ap_prc->channels_));

  if (nbytes < 0)
    {
      TIZ_ERROR (handleOf (ap_prc), "[%s] : libfaad decoder initialisation failure",
                 tiz_err_to_str (rc));
    }
  else
    {
      /* We will skip this many bytes the next time we read from this buffer */
      p_in->nOffset = nbytes;
      TIZ_DEBUG (handleOf (ap_prc), "samplerate [%d] channels [%d]",
                 ap_prc->samplerate_, (int) ap_prc->channels_);
      rc = OMX_ErrorNone;
    }
  return rc;
}

static OMX_ERRORTYPE
transform_buffer (aacdec_prc_t *ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in
      = tiz_filter_prc_get_header (ap_prc, ARATELIA_AAC_DECODER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE *p_out
      = tiz_filter_prc_get_header (ap_prc, ARATELIA_AAC_DECODER_OUTPUT_PORT_INDEX);

  if (NULL == p_in || NULL == p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "IN HEADER [%p] OUT HEADER [%p]", p_in,
                 p_out);
      return OMX_ErrorNone;
    }

  assert (NULL != ap_prc);
  assert (NULL != ap_prc->p_aac_dec_);

  if (0 == p_in->nFilledLen)
    {
      TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFlags [%d] is empty", p_in,
                 p_in->nFlags);
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Inmediately propagate EOS flag to output */
          TIZ_TRACE (handleOf (ap_prc), "Propagate EOS flag to output HEADER [%p]",
                     p_out);
          p_out->nFlags |= OMX_BUFFERFLAG_EOS;
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_in->nFlags   = 0;
          tiz_check_omx_err
            (tiz_filter_prc_release_header (ap_prc, ARATELIA_AAC_DECODER_OUTPUT_PORT_INDEX));
        }
    }

  if (p_in->nFilledLen > 0)
    {
      /* Decode the AAC data passed in the buffer. Returns a pointer to a
         sample buffer or NULL. Info about the decoded frame is filled in the
         NeAACDecFrameInfo structure. This structure holds information about
         errors during decoding, number of sample, number of channels and
         samplerate. The returned buffer contains the channel interleaved
         samples of the frame. */
      void * p_buf = p_out->pBuffer + p_out->nOffset;
      (void) NeAACDecDecode2(ap_prc->p_aac_dec_, &(ap_prc->aac_info_),
                             p_in->pBuffer + p_in->nOffset, p_in->nFilledLen,
                             &p_buf, p_out->nAllocLen - p_out->nOffset);
      if ((ap_prc->aac_info_.error == 0) && (ap_prc->aac_info_.samples > 0))
        {
          p_in->nOffset    += ap_prc->aac_info_.bytesconsumed;
          p_in->nFilledLen -= ap_prc->aac_info_.bytesconsumed;
        }
      else if (ap_prc->aac_info_.error != 0)
        {
          /* Some error occurred while decoding this frame */
          TIZ_ERROR (handleOf (ap_prc),
                     "[OMX_ErrorStreamCorruptFatal] : "
                     "While decoding the input stream (%s).",
                     NeAACDecGetErrorMessage (ap_prc->aac_info_.error));
          rc = OMX_ErrorStreamCorruptFatal;
        }
    }

  assert (p_in->nFilledLen >= 0);

  TIZ_TRACE (handleOf (ap_prc), "p_in->nFilledLen = [%d] consumed = [%d]",
             p_in->nFilledLen, ap_prc->aac_info_.bytesconsumed);

  if (OMX_ErrorNone == rc && 0 == p_in->nFilledLen)
    {
      TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] nFlags [%d] is empty", p_in,
                 p_in->nFlags);
      if ((p_in->nFlags & OMX_BUFFERFLAG_EOS) > 0)
        {
          /* Let's propagate EOS flag to output */
          TIZ_TRACE (handleOf (ap_prc), "Let's propagate EOS flag to output");
          tiz_filter_prc_update_eos_flag (ap_prc, true);
          p_in->nFlags = 0;
        }
      rc = tiz_filter_prc_release_header
        (ap_prc, ARATELIA_AAC_DECODER_INPUT_PORT_INDEX);
    }

  if (tiz_filter_prc_is_eos (ap_prc))
    {
      /* Propagate EOS flag to output */
      p_out->nFlags |= OMX_BUFFERFLAG_EOS;
      tiz_filter_prc_update_eos_flag (ap_prc, false);
      TIZ_TRACE (handleOf (ap_prc), "Propagating EOS flag to output");
    }

  if (OMX_ErrorNone == rc)
    {
      rc = tiz_filter_prc_release_header
        (ap_prc, ARATELIA_AAC_DECODER_OUTPUT_PORT_INDEX);
    }

  return rc;
}

static void reset_stream_parameters (aacdec_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  tiz_mem_set (&(ap_prc->aac_info_), 0, sizeof(ap_prc->aac_info_));
  ap_prc->samplerate_  = 0;
  ap_prc->channels_    = 0;
  ap_prc->nbytes_read_ = 0;
  ap_prc->first_buffer_read_ = false;
  tiz_filter_prc_update_eos_flag (ap_prc, false);
}

/*
 * aacdecprc
 */

static void *
aacdec_prc_ctor (void *ap_obj, va_list * app)
{
  aacdec_prc_t *p_prc = super_ctor (typeOf (ap_obj, "aacdecprc"), ap_obj, app);
  assert (NULL != p_prc);
  unsigned long cap = NeAACDecGetCapabilities();
  TIZ_DEBUG (handleOf (ap_obj), "libfaad2 caps: %X", cap);
  /*   Open the faad library */
  p_prc->p_aac_dec_ = NeAACDecOpen();
  reset_stream_parameters (p_prc);
  return p_prc;
}

static void *
aacdec_prc_dtor (void *ap_obj)
{
  aacdec_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  (void) aacdec_prc_deallocate_resources (p_prc);
  if (NULL != p_prc->p_aac_dec_)
    {
      NeAACDecClose(p_prc->p_aac_dec_);
      p_prc->p_aac_dec_ = NULL;
    }
  return super_dtor (typeOf (p_prc, "aacdecprc"), p_prc);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
aacdec_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  aacdec_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  /* Check that the library has been successfully inited */
  tiz_check_null_ret_oom ((p_prc->p_aac_dec_));
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
aacdec_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
aacdec_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  aacdec_prc_t *p_prc = ap_obj;
  assert (NULL != p_prc);
  reset_stream_parameters (p_prc);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
aacdec_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
aacdec_prc_stop_and_return (void *ap_obj)
{
  return tiz_filter_prc_release_all_headers (ap_obj);
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
aacdec_prc_buffers_ready (const void *ap_prc)
{
  aacdec_prc_t *p_prc = (aacdec_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_prc);

  TIZ_TRACE (handleOf (p_prc), "eos [%s] ",
             tiz_filter_prc_is_eos (p_prc) ? "YES" : "NO");
  while (tiz_filter_prc_headers_available (p_prc) && OMX_ErrorNone == rc)
    {
      if (!p_prc->first_buffer_read_)
        {
          rc = init_aac_decoder (p_prc);
          if (OMX_ErrorNone == rc)
            {
              p_prc->first_buffer_read_ = true;
            }
        }
      else
        {
          rc = transform_buffer (p_prc);
        }
    }

  return rc;
}

static OMX_ERRORTYPE
aacdec_prc_port_enable (const void *ap_prc, OMX_U32 a_pid)
{
  aacdec_prc_t *p_prc = (aacdec_prc_t *)ap_prc;
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, false);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
aacdec_prc_port_disable (const void *ap_prc, OMX_U32 a_pid)
{
  aacdec_prc_t *p_prc = (aacdec_prc_t *)ap_prc;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (OMX_ALL == a_pid)
    {
      rc = tiz_filter_prc_release_all_headers (p_prc);
    }
  else
    {
      rc = tiz_filter_prc_release_header (p_prc, a_pid);
    }
  tiz_filter_prc_update_port_disabled_flag (p_prc, a_pid, true);
  return rc;
}

/*
 * aacdec_prc_class
 */

static void *
aacdec_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "aacdecprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
aacdec_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * aacdecprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizfilterprc), "aacdecprc_class", classOf (tizfilterprc), sizeof (aacdec_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, aacdec_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return aacdecprc_class;
}

void *
aacdec_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizfilterprc = tiz_get_type (ap_hdl, "tizfilterprc");
  void * aacdecprc_class = tiz_get_type (ap_hdl, "aacdecprc_class");
  TIZ_LOG_CLASS (aacdecprc_class);
  void * aacdecprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (aacdecprc_class, "aacdecprc", tizfilterprc, sizeof (aacdec_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, aacdec_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, aacdec_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, aacdec_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, aacdec_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, aacdec_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, aacdec_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, aacdec_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, aacdec_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_enable, aacdec_prc_port_enable,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_port_disable, aacdec_prc_port_disable,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return aacdecprc;
}

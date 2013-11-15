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
#include "opusdprc.h"
#include "opusdprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"

#include "tizosal.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.opus_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE opusd_prc_deallocate_resources (void *);

static OMX_BUFFERHEADERTYPE *
get_buffer (opusd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  bool *p_port_disabled = NULL;
  assert (NULL != ap_prc);
  assert (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX
          || a_pid == ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);

  pp_hdr = (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX
            ? &(ap_prc->p_in_hdr_) : &(ap_prc->p_out_hdr_));
  assert (NULL != pp_hdr);

  p_port_disabled = (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX
                     ? &(ap_prc->in_port_disabled_)
                     : &(ap_prc->out_port_disabled_));
  assert (NULL != p_port_disabled);

  if (false == *p_port_disabled)
    {
      if (NULL != *pp_hdr)
        {
          TIZ_TRACE (handleOf (ap_prc), "HEADER [%p] pid [%d] nFilledLen [%d] ",
                     *pp_hdr, a_pid, (*pp_hdr)->nFilledLen);
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
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  bool *p_eos = NULL;

  assert (NULL != ap_prc);
  assert (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX
          || a_pid == ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);

  pp_hdr = (a_pid == ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX
            ? &(ap_prc->p_in_hdr_) : &(ap_prc->p_out_hdr_));
  assert (NULL != pp_hdr && NULL != *pp_hdr);

  p_eos = &(ap_prc->eos_);
  assert (NULL != p_eos);

  p_hdr = *pp_hdr;

  TIZ_TRACE (handleOf (ap_prc), "Releasing HEADER [%p] pid [%d] "
             "nFilledLen [%d] ", p_hdr, a_pid, p_hdr->nFilledLen);

  p_hdr->nOffset = 0;

  if ((p_hdr->nFlags & OMX_BUFFERFLAG_EOS) > 0)
    {
      TIZ_TRACE (handleOf (ap_prc), "EOS: Last buffer "
                "HEADER [%p] pid [%d]", p_hdr, a_pid);
      tiz_srv_issue_event ((OMX_PTR) ap_prc,
                           OMX_EventBufferFlag, a_pid, p_hdr->nFlags, NULL);
    }

  /* TODO: Check for OOM error and issue Error Event */
  (void) tiz_krn_release_buffer
    (tiz_get_krn (handleOf (ap_prc)), a_pid, p_hdr);
  *pp_hdr = NULL;
}

static OMX_ERRORTYPE
transform_buffer (const opusd_prc_t * ap_prc)
{
  opusd_prc_t *p_prc = (opusd_prc_t *) p_prc;
  OMX_BUFFERHEADERTYPE *p_in  = get_buffer (p_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE *p_out = get_buffer (p_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);

  if (NULL == p_in || NULL == p_out)
    {
      return OMX_ErrorNone;
    }

  {
    assert (NULL != p_prc);

    const unsigned char *p_data = p_in->pBuffer + p_in->nOffset;
    opus_int32 len = p_in->nFilledLen;
    opus_int16 *p_pcm = (opus_int16 *) (p_out->pBuffer + p_out->nOffset);
    int frame_size = 0;
    int decode_fec = 0;
    int decoded_samples = 0;
    decoded_samples = opus_decode (p_prc->p_opus_dec_, p_data, len, p_pcm, frame_size, decode_fec);
    (void) decoded_samples;
  }
  release_buffer (p_prc, ARATELIA_OPUS_DECODER_INPUT_PORT_INDEX);
  release_buffer (p_prc, ARATELIA_OPUS_DECODER_OUTPUT_PORT_INDEX);
  
  return OMX_ErrorNone;
}

/*
 * opusdprc
 */

static void *
opusd_prc_ctor (void *ap_obj, va_list * app)
{
  opusd_prc_t *p_prc        = super_ctor (typeOf (ap_obj, "opusdprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_opus_dec_        = NULL;
  p_prc->p_in_hdr_          = NULL;
  p_prc->p_out_hdr_         = NULL;
  p_prc->eos_               = false;
  p_prc->in_port_disabled_  = false;
  p_prc->out_port_disabled_ = false;
  p_prc->awaiting_buffers_  = true;
  TIZ_TRACE (handleOf (p_prc),
            "Opus library vesion [%]", opus_get_version_string());
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
  int error = 0;
  assert (NULL != ap_obj);
  if (NULL == (p_prc->p_opus_dec_
               = opus_decoder_create (48000, 2, &error)))
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
      opus_decoder_destroy (p_prc->p_opus_dec_);
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
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
opusd_prc_buffers_ready (const void *ap_obj)
{
  return transform_buffer (ap_obj);
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
opusd_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * opusdprc_class = factory_new (classOf (tizprc),
                                      "opusdprc_class",
                                      classOf (tizprc),
                                      sizeof (opusd_prc_class_t),
                                      ap_tos, ap_hdl,
                                      ctor, opusd_prc_class_ctor, 0);
  return opusdprc_class;
}

void *
opusd_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * opusdprc_class = tiz_get_type (ap_hdl, "opusdprc_class");
  void * opusdprc =
    factory_new
    (opusdprc_class,
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

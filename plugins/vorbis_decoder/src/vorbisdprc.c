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
 * @file   vorbisdprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Vorbis Decoder processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "vorbisd.h"
#include "vorbisdprc.h"
#include "vorbisdprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.vorbis_decoder.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE vorbisd_prc_deallocate_resources (void *);

static inline OMX_BUFFERHEADERTYPE **
get_header_ptr (vorbisd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  OMX_BUFFERHEADERTYPE **pp_hdr = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);
  pp_hdr = (a_pid == ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX
            ? &(ap_prc->p_in_hdr_) : &(ap_prc->p_out_hdr_));
  assert (NULL != pp_hdr);
  return pp_hdr;
}

static inline bool *
get_port_disabled_ptr (vorbisd_prc_t * ap_prc, const OMX_U32 a_pid)
{
  bool *p_port_disabled = NULL;
  assert (NULL != ap_prc);
  assert (a_pid <= ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);
  p_port_disabled = (a_pid == ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX
                     ? &(ap_prc->in_port_disabled_)
                     : &(ap_prc->out_port_disabled_));
  assert (NULL != p_port_disabled);
  return p_port_disabled;
}

static OMX_BUFFERHEADERTYPE *
get_buffer (vorbisd_prc_t * ap_prc, const OMX_U32 a_pid)
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
release_buffer (vorbisd_prc_t * ap_prc, const OMX_U32 a_pid)
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
buffers_available (vorbisd_prc_t * ap_prc)
{
  bool rc = true;
  rc &=
    (NULL != get_buffer (ap_prc, ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX));
  rc &=
    (NULL != get_buffer (ap_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX));
  return rc;
}

OMX_ERRORTYPE
init_vorbis_decoder (vorbisd_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *p_in
    = get_buffer (ap_prc, ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX);

  if (NULL == p_in)
    {
      return OMX_ErrorNoMore;
    }

/*   { */
/*     OMX_U8 *p_ogg_data = p_in->pBuffer + p_in->nOffset; */
/*     const OMX_U32 nbytes = p_in->nFilledLen; */
/*     /\*If playing to audio out, default the rate to 48000 */
/*      * instead of the original rate. The original rate is */
/*      * only important for minimizing surprise about the rate */
/*      * of output files and preserving length, which aren't */
/*      * relevant for playback. Many audio devices sound */
/*      * better at 48kHz and not resampling also saves CPU. *\/ */
/*     ap_prc->rate_ = 48000; */
/*     ap_prc->channels_ = -1; */
/*     float gain = 1; */
/*     float manual_gain = 0; */
/*     int streams = 0; */
/*     int quiet = 0; */
/*     if (NULL == (ap_prc->p_vorbis_dec_ */
/*                  = process_vorbis_header (handleOf (ap_prc), p_ogg_data, */
/*                                         nbytes, &(ap_prc->rate_), */
/*                                         &(ap_prc->mapping_family_), */
/*                                         &(ap_prc->channels_), */
/*                                         &(ap_prc->preskip_), &gain, */
/*                                         manual_gain, &streams, quiet))) */
/*       { */
/*         rc = OMX_ErrorInsufficientResources; */
/*       } */
/*     TIZ_TRACE (handleOf (ap_prc), */
/*                "rate [%d] mapping_family [%d] channels [%d] " */
/*                "preskip [%d] gain [%d] streams [%d]", ap_prc->rate_, */
/*                ap_prc->mapping_family_, ap_prc->channels_, ap_prc->preskip_, */
/*                gain, streams); */
/*   } */
/*   p_in->nFilledLen = 0; */
/*   release_buffer (ap_prc, ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX); */
  return rc;
}

static OMX_ERRORTYPE
print_vorbis_comments (vorbisd_prc_t * ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_in
    = get_buffer (ap_prc, ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX);

  if (NULL == p_in)
    {
      return OMX_ErrorNoMore;
    }

/*   process_vorbis_comments (handleOf (ap_prc), */
/*                          (char *) (p_in->pBuffer + p_in->nOffset), */
/*                          p_in->nFilledLen); */
  p_in->nFilledLen = 0;
  release_buffer (ap_prc, ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
transform_buffer (const void *ap_obj)
{
  return OMX_ErrorNone;
}

static void
reset_stream_parameters (vorbisd_prc_t *ap_prc)
{
  assert (NULL != ap_prc);
  ap_prc->packet_count_   = 0;
  ap_prc->rate_           = 0;
  ap_prc->channels_       = 0;
}

/*
 * vorbisdprc
 */

static void *
vorbisd_prc_ctor (void *ap_obj, va_list * app)
{
  vorbisd_prc_t *p_prc           = super_ctor (typeOf (ap_obj, "vorbisdprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_in_hdr_          = NULL;
  p_prc->p_out_hdr_         = NULL;
  reset_stream_parameters (p_prc);
  p_prc->eos_               = false;
  p_prc->in_port_disabled_  = false;
  p_prc->out_port_disabled_ = false;
  return p_prc;
}

static void *
vorbisd_prc_dtor (void *ap_obj)
{
  (void) vorbisd_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "vorbisdprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
vorbisd_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vorbisd_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vorbisd_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vorbisd_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
vorbisd_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
vorbisd_prc_buffers_ready (const void *ap_obj)
{
  vorbisd_prc_t *p_prc = (vorbisd_prc_t *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_obj);
  TIZ_TRACE (handleOf (p_prc), "eos [%s] packet_count [%d]",
             p_prc->eos_ ? "YES" : "NO", p_prc->packet_count_);
  if (!p_prc->eos_)
    {
      if (0 == p_prc->packet_count_)
        {
          /* If first packet in the logical stream, process the Vorbis header and
           * instantiate an vorbis decoder with the right settings */
          rc = init_vorbis_decoder (p_prc);
          if (OMX_ErrorNoMore == rc)
            {
              rc = OMX_ErrorNone;
            }
          else
            {
              p_prc->packet_count_++;
            }
        }
      else if (1 == p_prc->packet_count_)
        {
          rc = print_vorbis_comments (p_prc);
          if (OMX_ErrorNoMore == rc)
            {
              rc = OMX_ErrorNone;
            }
          else
            {
              p_prc->packet_count_++;
            }
        }
      else
        {
          while (buffers_available (p_prc) && OMX_ErrorNone == rc)
            {
              rc = transform_buffer (p_prc);
              if (OMX_ErrorNone)
                {
                  p_prc->packet_count_++;
                }
            }
        }
    }

  return rc;
}

/*
 * vorbisd_prc_class
 */

static void *
vorbisd_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "vorbisdprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
vorbisd_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * vorbisdprc_class = factory_new (classOf (tizprc),
                                    "vorbisdprc_class",
                                    classOf (tizprc),
                                    sizeof (vorbisd_prc_class_t),
                                    ap_tos, ap_hdl,
                                    ctor, vorbisd_prc_class_ctor, 0);
  return vorbisdprc_class;
}

void *
vorbisd_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * vorbisdprc_class = tiz_get_type (ap_hdl, "vorbisdprc_class");
  TIZ_LOG_CLASS (vorbisdprc_class);
  void * vorbisdprc =
    factory_new
    (vorbisdprc_class,
     "vorbisdprc",
     tizprc,
     sizeof (vorbisd_prc_t),
     ap_tos, ap_hdl,
     ctor, vorbisd_prc_ctor,
     dtor, vorbisd_prc_dtor,
     tiz_prc_buffers_ready, vorbisd_prc_buffers_ready,
     tiz_srv_allocate_resources, vorbisd_prc_allocate_resources,
     tiz_srv_deallocate_resources, vorbisd_prc_deallocate_resources,
     tiz_srv_prepare_to_transfer, vorbisd_prc_prepare_to_transfer,
     tiz_srv_transfer_and_process, vorbisd_prc_transfer_and_process,
     tiz_srv_stop_and_return, vorbisd_prc_stop_and_return, 0);

  return vorbisdprc;
}

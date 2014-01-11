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

#include <fishsound/constants.h>

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

static int
decoded_cback (FishSound *ap_fsound, float **app_pcm, long frames, void *ap_user_data)
{
  int rc = FISH_SOUND_CONTINUE;
  vorbisd_prc_t * p_prc = ap_user_data;

  (void) ap_fsound;
  assert (NULL != app_pcm);
  assert (NULL != ap_user_data);

  if (0 == p_prc->packet_count_)
    {
      fish_sound_command (p_prc->p_fsnd_, FISH_SOUND_GET_INFO, &(p_prc->fsinfo_),
                          sizeof (FishSoundInfo));
      if (p_prc->fsinfo_.channels > 2 || p_prc->fsinfo_.format != FISH_SOUND_VORBIS)
        {
          TIZ_ERROR (handleOf (p_prc), "Only support for vorbis "
                     "streams with 1 or 2 channels.");
          rc = FISH_SOUND_STOP_ERR;
        }
    }

  if (FISH_SOUND_STOP_ERR == rc)
    {
      /* write decoded PCM samples */
      size_t i = 0;
      size_t j = 0;
      size_t nbytes = frames * sizeof(float) * p_prc->channels_;
      OMX_BUFFERHEADERTYPE *p_out
        = get_buffer (p_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);
      assert (NULL != p_out);

      for (i = 0; i < nbytes;)
        {
          float *out = (float *) (p_out->pBuffer + p_out->nOffset) + i;
          out[0] = (float) app_pcm[0][j];       /* left channel */
          out[1] = (float) app_pcm[1][j];       /* right channel */
          j++;
          i += sizeof(float);
        }
/*       p_out->nFilledLen = */
/*         frames * p_prc->channels_ * (p_prc->bps_ / 8); */
/*       if ((p_prc->eos_ && p_prc->store_offset_ == 0)) */
/*         { */
/*           /\* Propagate EOS flag to output *\/ */
/*           p_out->nFlags |= OMX_BUFFERFLAG_EOS; */
/*           p_prc->eos_ = false; */
/*         } */
      release_buffer (p_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);
    }

/* Parameters: */
/*     	fsound 	The FishSound* handle */
/*     	pcm 	The decoded audio */
/*     	frames 	The count of frames decoded */
/*     	user_data 	Arbitrary user data */

/* Return values: */
/*     	FISH_SOUND_CONTINUE 	Continue decoding */
/*     	FISH_SOUND_STOP_OK 	Stop decoding immediately and return control to the fish_sound_decode() caller */
/*     	FISH_SOUND_STOP_ERR 	Stop decoding immediately, purge buffered data, and return control to the fish_sound_decode() caller  */
  /* Continue decoding */
  return FISH_SOUND_CONTINUE;
}

static OMX_ERRORTYPE
init_vorbis_decoder (vorbisd_prc_t * ap_prc)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;

  assert (NULL != ap_prc);

  tiz_check_null_ret_oom (ap_prc->p_fsnd_
                          = fish_sound_new (FISH_SOUND_DECODE, NULL));

  if (0 != fish_sound_set_interleave (ap_prc->p_fsnd_, 1))
    {
      TIZ_ERROR (handleOf (ap_prc), "[OMX_ErrorInsufficientResources] : "
                 "Could not set interleaved.");
      goto end;
    }


  if (0 != fish_sound_set_decoded_float_ilv (ap_prc->p_fsnd_,
                                             decoded_cback,
                                             ap_prc))
    {
      TIZ_ERROR (handleOf (ap_prc), "[OMX_ErrorInsufficientResources] : "
                 "Could not set 'decoded' callback.");
      goto end;
    }

  rc = OMX_ErrorNone;

 end:
  if (OMX_ErrorInsufficientResources == rc)
    {
      fish_sound_delete (ap_prc->p_fsnd_);
    }

  return rc;
}

static OMX_ERRORTYPE
transform_buffer (vorbisd_prc_t *ap_prc)
{
  OMX_BUFFERHEADERTYPE *p_in
    = get_buffer (ap_prc, ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX);
  OMX_BUFFERHEADERTYPE *p_out
    = get_buffer (ap_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);

  if (NULL == p_in || NULL == p_out)
    {
      TIZ_TRACE (handleOf (ap_prc), "IN HEADER [%p] OUT HEADER [%p]",
                 p_in, p_out);
      return OMX_ErrorNone;
    }

  assert (NULL != ap_prc);

  if (p_in->nFilledLen > 0)
  {
    unsigned char *p_data = p_in->pBuffer + p_in->nOffset;
    const long     len    = p_in->nFilledLen;
    long bytes_decoded = fish_sound_decode(ap_prc->p_fsnd_, p_data, len);

    if (bytes_decoded > 0)
      {
        assert (p_in->nFilledLen >= bytes_decoded);
        p_in->nFilledLen -= bytes_decoded;
        p_in->nOffset += bytes_decoded;
      }
    else
      {
        switch (bytes_decoded)
          {
          case FISH_SOUND_STOP_ERR:
            {
              TIZ_ERROR (handleOf (ap_prc), "[FISH_SOUND_STOP_ERR] : "
                         "While decoding the input stream.");
            }
            break;
          case FISH_SOUND_ERR_OUT_OF_MEMORY:
            {
              TIZ_ERROR (handleOf (ap_prc), "[OMX_ErrorInsufficientResources] : "
                         "While decoding the input stream.");
            }
            break;
          case FISH_SOUND_ERR_BAD:
          default:
            {
              assert (0);
            }
          };
      }
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
          release_buffer (ap_prc, ARATELIA_VORBIS_DECODER_OUTPUT_PORT_INDEX);
        }
      release_buffer (ap_prc, ARATELIA_VORBIS_DECODER_INPUT_PORT_INDEX);
    }
      
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
  vorbisd_prc_t *p_prc      = super_ctor (typeOf (ap_obj, "vorbisdprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_in_hdr_          = NULL;
  p_prc->p_out_hdr_         = NULL;
  p_prc->p_fsnd_            = NULL;
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
  return init_vorbis_decoder (ap_obj);
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
      while (buffers_available (p_prc) && OMX_ErrorNone == rc)
        {
          rc = transform_buffer (p_prc);
          if (OMX_ErrorNone)
            {
              p_prc->packet_count_++;
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

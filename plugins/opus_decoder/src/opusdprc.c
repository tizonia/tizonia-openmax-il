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

static OMX_ERRORTYPE
transform_buffer (const void *ap_obj)
{
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
  p_prc->pinhdr_     = NULL;
  p_prc->pouthdr_    = NULL;
  p_prc->eos_        = false;
  TIZ_TRACE (handleOf (p_prc),
            "Opus library vesion [%]", opus_get_version_string());
  return p_prc;
}

static void *
opusd_prc_dtor (void *ap_obj)
{
  opusd_prc_t *p_prc = ap_obj;
  assert (NULL != ap_obj);
  if (NULL != p_prc->p_opus_dec_)
    {
      opus_decoder_destroy (p_prc->p_opus_dec_);
      p_prc->p_opus_dec_ = NULL;
    }
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

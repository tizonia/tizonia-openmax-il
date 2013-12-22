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

static OMX_ERRORTYPE
transform_buffer (const void *ap_obj)
{
  return OMX_ErrorNone;
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
  return transform_buffer (ap_obj);
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

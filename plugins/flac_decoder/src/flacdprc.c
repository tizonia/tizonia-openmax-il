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
 * @file   flacdprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - FLAC Decoder processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "flacdprc.h"
#include "flacdprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.flac_decoder.prc"
#endif

static OMX_ERRORTYPE
transform_buffer (const void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * flacdprc
 */

static void *
flacd_prc_ctor (void *ap_obj, va_list * app)
{
  flacd_prc_t *p_obj   = super_ctor (typeOf (ap_obj, "flacdprc"), ap_obj, app);
  p_obj->p_in_hdr_  = false;
  p_obj->p_out_hdr_ = false;
  p_obj->eos_       = false;
  return p_obj;
}

static void *
flacd_prc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "flacdprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
flacd_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
flacd_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
flacd_prc_buffers_ready (const void *ap_obj)
{
  return transform_buffer (ap_obj);
}

/*
 * flacd_prc_class
 */

static void *
flacd_prc_class_ctor (void *ap_obj, va_list * app)
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
  void * flacdprc_class = factory_new (classOf (tizprc),
                                    "flacdprc_class",
                                    classOf (tizprc),
                                    sizeof (flacd_prc_class_t),
                                    ap_tos, ap_hdl,
                                    ctor, flacd_prc_class_ctor, 0);
  return flacdprc_class;
}

void *
flacd_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * flacdprc_class = tiz_get_type (ap_hdl, "flacdprc_class");
  TIZ_LOG_CLASS (flacdprc_class);
  void * flacdprc =
    factory_new
    (flacdprc_class,
     "flacdprc",
     tizprc,
     sizeof (flacd_prc_t),
     ap_tos, ap_hdl,
     ctor, flacd_prc_ctor,
     dtor, flacd_prc_dtor,
     tiz_prc_buffers_ready, flacd_prc_buffers_ready,
     tiz_srv_allocate_resources, flacd_prc_allocate_resources,
     tiz_srv_deallocate_resources, flacd_prc_deallocate_resources,
     tiz_srv_prepare_to_transfer, flacd_prc_prepare_to_transfer,
     tiz_srv_transfer_and_process, flacd_prc_transfer_and_process,
     tiz_srv_stop_and_return, flacd_prc_stop_and_return, 0);

  return flacdprc;
}

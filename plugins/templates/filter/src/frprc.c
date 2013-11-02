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
 * @file   frprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - File Reader processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "frprc.h"
#include "frprc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"
#include "tizosal.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.file_reader.prc"
#endif

static OMX_ERRORTYPE
transform_buffer (const void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * frprc
 */

static void *
fr_prc_ctor (void *ap_obj, va_list * app)
{
  fr_prc_t *p_obj = super_ctor (typeOf (ap_obj, "frprc"), ap_obj, app);
  p_obj->pinhdr_  = false;
  p_obj->pouthdr_ = false;
  p_obj->eos_     = false;
  return p_obj;
}

static void *
fr_prc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "frprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
fr_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
fr_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
fr_prc_buffers_ready (const void *ap_obj)
{
  return transform_buffer (ap_obj);
}

/*
 * fr_prc_class
 */

static void *
fr_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "frprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
fr_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * frprc_class = factory_new (classOf (tizprc),
                                    "frprc_class",
                                    classOf (tizprc),
                                    sizeof (fr_prc_class_t),
                                    ap_tos, ap_hdl,
                                    ctor, fr_prc_class_ctor, 0);
  return frprc_class;
}

void *
fr_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * frprc_class = tiz_get_type (ap_hdl, "frprc_class");
  TIZ_LOG_CLASS (frprc_class);
  void * frprc =
    factory_new
    (frprc_class,
     "frprc",
     tizprc,
     sizeof (fr_prc_t),
     ap_tos, ap_hdl,
     ctor, fr_prc_ctor,
     dtor, fr_prc_dtor,
     tiz_prc_buffers_ready, fr_prc_buffers_ready,
     tiz_srv_allocate_resources, fr_prc_allocate_resources,
     tiz_srv_deallocate_resources, fr_prc_deallocate_resources,
     tiz_srv_prepare_to_transfer, fr_prc_prepare_to_transfer,
     tiz_srv_transfer_and_process, fr_prc_transfer_and_process,
     tiz_srv_stop_and_return, fr_prc_stop_and_return, 0);

  return frprc;
}

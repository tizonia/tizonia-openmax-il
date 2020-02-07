/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   inprocsrcprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - ZMQ inproc socket reader
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <tizplatform.h>

#include <tizkernel.h>
#include <tizscheduler.h>

#include "inprocsrc.h"
#include "inprocsrcprc.h"
#include "inprocsrcprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.inproc_reader.prc"
#endif

/*
 * inprocsrcprc
 */

static void *
inprocsrc_prc_ctor (void *ap_obj, va_list * app)
{
  inprocsrc_prc_t *p_obj = super_ctor (typeOf (ap_obj, "inprocsrcprc"), ap_obj, app);
  p_obj->eos_ = false;
  return p_obj;
}

static void *
inprocsrc_prc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "inprocsrcprc"), ap_obj);
}

static OMX_ERRORTYPE
inprocsrc_prc_read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  return OMX_ErrorNone;
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
inprocsrc_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
inprocsrc_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
inprocsrc_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
inprocsrc_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
inprocsrc_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
inprocsrc_prc_buffers_ready (const void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * inprocsrc_prc_class
 */

static void *
inprocsrc_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "inprocsrcprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
inprocsrc_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * inprocsrcprc_class = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (classOf (tizprc), "inprocsrcprc_class", classOf (tizprc), sizeof (inprocsrc_prc_class_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, inprocsrc_prc_class_ctor,
     /* TIZ_CLASS_COMMENT: stop value*/
     0);
  return inprocsrcprc_class;
}

void *
inprocsrc_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * inprocsrcprc_class = tiz_get_type (ap_hdl, "inprocsrcprc_class");
  TIZ_LOG_CLASS (inprocsrcprc_class);
  void * inprocsrcprc = factory_new
    /* TIZ_CLASS_COMMENT: class type, class name, parent, size */
    (inprocsrcprc_class, "inprocsrcprc", tizprc, sizeof (inprocsrc_prc_t),
     /* TIZ_CLASS_COMMENT: */
     ap_tos, ap_hdl,
     /* TIZ_CLASS_COMMENT: class constructor */
     ctor, inprocsrc_prc_ctor,
     /* TIZ_CLASS_COMMENT: class destructor */
     dtor, inprocsrc_prc_dtor,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_allocate_resources, inprocsrc_prc_allocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_deallocate_resources, inprocsrc_prc_deallocate_resources,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_prepare_to_transfer, inprocsrc_prc_prepare_to_transfer,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_transfer_and_process, inprocsrc_prc_transfer_and_process,
     /* TIZ_CLASS_COMMENT: */
     tiz_srv_stop_and_return, inprocsrc_prc_stop_and_return,
     /* TIZ_CLASS_COMMENT: */
     tiz_prc_buffers_ready, inprocsrc_prc_buffers_ready,
     /* TIZ_CLASS_COMMENT: stop value */
     0);

  return inprocsrcprc;
}

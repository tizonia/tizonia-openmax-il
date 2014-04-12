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
 * @file   mp3metaprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Mp3 Metadata Eraser processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <tizkernel.h>
#include <tizscheduler.h>
#include <tizplatform.h>

#include "mp3meta.h"
#include "mp3metaprc.h"
#include "mp3metaprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.mp3_metadata.prc"
#endif

/* Forward declarations */
static OMX_ERRORTYPE mp3meta_prc_deallocate_resources (void *);

static OMX_ERRORTYPE
transform_buffer (const void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * mp3metaprc
 */

static void *
mp3meta_prc_ctor (void *ap_obj, va_list * app)
{
  mp3meta_prc_t *p_prc           = super_ctor (typeOf (ap_obj, "mp3metaprc"), ap_obj, app);
  assert (NULL != p_prc);
  p_prc->p_in_hdr_          = NULL;
  p_prc->p_out_hdr_         = NULL;
  p_prc->eos_               = false;
  p_prc->in_port_disabled_  = false;
  p_prc->out_port_disabled_ = false;
  return p_prc;
}

static void *
mp3meta_prc_dtor (void *ap_obj)
{
  (void) mp3meta_prc_deallocate_resources (ap_obj);
  return super_dtor (typeOf (ap_obj, "mp3metaprc"), ap_obj);
}

/*
 * from tizsrv class
 */

static OMX_ERRORTYPE
mp3meta_prc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3meta_prc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3meta_prc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3meta_prc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
mp3meta_prc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tizprc class
 */

static OMX_ERRORTYPE
mp3meta_prc_buffers_ready (const void *ap_obj)
{
  return transform_buffer (ap_obj);
}

/*
 * mp3meta_prc_class
 */

static void *
mp3meta_prc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "mp3metaprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
mp3meta_prc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * mp3metaprc_class = factory_new (classOf (tizprc),
                                    "mp3metaprc_class",
                                    classOf (tizprc),
                                    sizeof (mp3meta_prc_class_t),
                                    ap_tos, ap_hdl,
                                    ctor, mp3meta_prc_class_ctor, 0);
  return mp3metaprc_class;
}

void *
mp3meta_prc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * mp3metaprc_class = tiz_get_type (ap_hdl, "mp3metaprc_class");
  TIZ_LOG_CLASS (mp3metaprc_class);
  void * mp3metaprc =
    factory_new
    (mp3metaprc_class,
     "mp3metaprc",
     tizprc,
     sizeof (mp3meta_prc_t),
     ap_tos, ap_hdl,
     ctor, mp3meta_prc_ctor,
     dtor, mp3meta_prc_dtor,
     tiz_prc_buffers_ready, mp3meta_prc_buffers_ready,
     tiz_srv_allocate_resources, mp3meta_prc_allocate_resources,
     tiz_srv_deallocate_resources, mp3meta_prc_deallocate_resources,
     tiz_srv_prepare_to_transfer, mp3meta_prc_prepare_to_transfer,
     tiz_srv_transfer_and_process, mp3meta_prc_transfer_and_process,
     tiz_srv_stop_and_return, mp3meta_prc_stop_and_return, 0);

  return mp3metaprc;
}

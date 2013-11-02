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
 * @file   tiztcproc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - test component processor class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tiztcproc.h"
#include "tiztcproc_decls.h"
#include "tizkernel.h"
#include "tizscheduler.h"

#include "tizosal.h"

#include <assert.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.test_comp"
#endif

/*
 * tiztcprc
 */

static void *
tcprc_ctor (void *ap_obj, va_list * app)
{
  tiz_tcprc_t *p_obj = super_ctor (typeOf (ap_obj, "tiztcprc"), ap_obj, app);
  return p_obj;
}

static void *
tcprc_dtor (void *ap_obj)
{
  return super_dtor (typeOf (ap_obj, "tiztcprc"), ap_obj);
}

static OMX_ERRORTYPE
tiztc_proc_render_buffer (OMX_BUFFERHEADERTYPE * p_hdr)
{
  return OMX_ErrorNone;
}

/*
 * from tiz_srv class
 */

static OMX_ERRORTYPE
tcprc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tcprc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tcprc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tcprc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tcprc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tiz_prc class
 */

static OMX_ERRORTYPE
tcprc_buffers_ready (const void *ap_obj)
{
  tiz_pd_set_t ports;
  void *p_ker = tiz_get_krn (tiz_api_get_hdl (ap_obj));
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  TIZ_PD_ZERO (&ports);

  tiz_check_omx_err (tiz_krn_select (p_ker, 1, &ports));

  if (TIZ_PD_ISSET (0, &ports))
    {
      tiz_check_omx_err (tiz_krn_claim_buffer (p_ker, 0, 0, &p_hdr));
      tiz_check_omx_err (tiztc_proc_render_buffer (p_hdr));
      (void) tiz_krn_release_buffer (p_ker, 0, p_hdr);
    }

  return OMX_ErrorNone;
}

/*
 * tiztcprc_class
 */

static void *
tcprc_class_ctor (void *ap_obj, va_list * app)
{
  /* NOTE: Class methods might be added in the future. None for now. */
  return super_ctor (typeOf (ap_obj, "tiztcprc_class"), ap_obj, app);
}

/*
 * initialization
 */

void *
tiz_tcprc_class_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * tiztcprc_class = factory_new (classOf (tizprc),
                                         "tiztcprc_class",
                                         classOf (tizprc),
                                         sizeof (tiz_tcprc_class_t),
                                         ap_tos, ap_hdl,
                                         ctor, tcprc_class_ctor, 0);
  return tiztcprc_class;
}

void *
tiz_tcprc_init (void * ap_tos, void * ap_hdl)
{
  void * tizprc = tiz_get_type (ap_hdl, "tizprc");
  void * tiztcprc_class = tiz_get_type (ap_hdl, "tiztcprc_class");
  TIZ_LOG_CLASS (tiztcprc_class);
  void * tiztcprc =
    factory_new
    (tiztcprc_class,
     "tiztcprc",
     tizprc,
     sizeof (tiz_tcprc_t),
     ap_tos, ap_hdl,
     ctor, tcprc_ctor,
     dtor, tcprc_dtor,
     tiz_prc_buffers_ready, tcprc_buffers_ready,
     tiz_srv_allocate_resources, tcprc_allocate_resources,
     tiz_srv_deallocate_resources, tcprc_deallocate_resources,
     tiz_srv_prepare_to_transfer, tcprc_prepare_to_transfer,
     tiz_srv_transfer_and_process, tcprc_transfer_and_process,
     tiz_srv_stop_and_return, tcprc_stop_and_return, 0);

  return tiztcprc;
}

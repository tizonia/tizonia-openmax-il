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

#include <assert.h>

#include "tizkernel.h"
#include "tizscheduler.h"

#include "tiztcproc.h"
#include "tiztcproc_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.test_comp"
#endif

/*
 * tiztcproc
 */

static void *
tiztc_proc_ctor (void *ap_obj, va_list * app)
{
  struct tiztcproc *p_obj = super_ctor (tiztcproc, ap_obj, app);
  return p_obj;
}

static void *
tiztc_proc_dtor (void *ap_obj)
{
  return super_dtor (tiztcproc, ap_obj);
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
tiztc_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tiztc_proc_deallocate_resources (void *ap_obj)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tiztc_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tiztc_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tiztc_proc_stop_and_return (void *ap_obj)
{
  return OMX_ErrorNone;
}

/*
 * from tiz_proc class
 */

static OMX_ERRORTYPE
tiztc_proc_buffers_ready (const void *ap_obj)
{
  const tiz_srv_t *p_parent = ap_obj;
  tiz_pd_set_t ports;
  void *p_ker = tiz_get_krn (p_parent->p_hdl_);
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  TIZ_LOG (TIZ_TRACE, "Buffers ready...");

  TIZ_PD_ZERO (&ports);

  tiz_check_omx_err (tiz_krn_select (p_ker, 1, &ports));

  if (TIZ_PD_ISSET (0, &ports))
    {
      tiz_check_omx_err (tiz_krn_claim_buffer (p_ker, 0, 0, &p_hdr));
      tiz_check_omx_err (tiztc_proc_render_buffer (p_hdr));
      (void) tiz_krn_relinquish_buffer (p_ker, 0, p_hdr);
    }

  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *tiztcproc;

void
init_tiztcproc (void)
{
  if (!tiztcproc)
    {
      tiz_proc_init ();
      tiztcproc =
        factory_new
        (tizproc_class,
         "tiztcproc",
         tizproc,
         sizeof (struct tiztcproc),
         ctor, tiztc_proc_ctor,
         dtor, tiztc_proc_dtor,
         tiz_proc_buffers_ready, tiztc_proc_buffers_ready,
         tiz_srv_allocate_resources, tiztc_proc_allocate_resources,
         tiz_srv_deallocate_resources, tiztc_proc_deallocate_resources,
         tiz_srv_prepare_to_transfer, tiztc_proc_prepare_to_transfer,
         tiz_srv_transfer_and_process, tiztc_proc_transfer_and_process,
         tiz_srv_stop_and_return, tiztc_proc_stop_and_return, 0);
    }
}

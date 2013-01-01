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
  TIZ_LOG (TIZ_LOG_TRACE, "Constructing tiztcproc...[%p]", p_obj);
  return p_obj;
}

static void *
tiztc_proc_dtor (void *ap_obj)
{
  struct tiztcproc *p_obj = ap_obj;
  TIZ_LOG (TIZ_LOG_TRACE, "Destructing tiztcproc...[%p]", p_obj);
  return super_dtor (tiztcproc, ap_obj);
}

static OMX_ERRORTYPE
tiztc_proc_render_buffer (OMX_BUFFERHEADERTYPE * p_hdr)
{
  TIZ_LOG (TIZ_LOG_TRACE, "Rendering HEADER [%p]...!!!", p_hdr);
  return OMX_ErrorNone;
}

/*
 * from tizservant class
 */

static OMX_ERRORTYPE
tiztc_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  struct tiztcproc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "Resource allocation...p_obj = [%p]!!!",
             p_obj);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tiztc_proc_deallocate_resources (void *ap_obj)
{
  struct tiztcproc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "Resource deallocation...p_obj = [%p]!!!",
             p_obj);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tiztc_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  struct tiztcproc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "Awaiting buffers...p_obj = [%p]!!!", p_obj);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tiztc_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  struct tiztcproc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "Transfering buffers...p_obj = [%p]!!!",
             p_obj);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
tiztc_proc_stop_and_return (void *ap_obj)
{
  struct tiztcproc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "Stopped buffer transfer...p_obj = [%p]!!!",
             p_obj);

  return OMX_ErrorNone;
}

/*
 * from tizproc class
 */

static OMX_ERRORTYPE
tiztc_proc_buffers_ready (const void *ap_obj)
{
  const struct tizservant *p_parent = ap_obj;
  tiz_pd_set_t ports;
  void *p_ker = tiz_get_krn (p_parent->p_hdl_);
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "Buffers ready...");

  TIZ_PD_ZERO (&ports);

  TIZ_UTIL_TEST_ERR (tizkernel_select (p_ker, 1, &ports));

  if (TIZ_PD_ISSET (0, &ports))
    {
      TIZ_UTIL_TEST_ERR (tizkernel_claim_buffer (p_ker, 0, 0, &p_hdr));
      TIZ_LOG (TIZ_LOG_TRACE, "Claimed HEADER [%p]...", p_hdr);
      TIZ_UTIL_TEST_ERR (tiztc_proc_render_buffer (p_hdr));
      tizkernel_relinquish_buffer (p_ker, 0, p_hdr);
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
      TIZ_LOG (TIZ_LOG_TRACE, "Initializing tiztcproc...");
      init_tizproc ();
      tiztcproc =
        factory_new
        (tizproc_class,
         "tiztcproc",
         tizproc,
         sizeof (struct tiztcproc),
         ctor, tiztc_proc_ctor,
         dtor, tiztc_proc_dtor,
         tizproc_buffers_ready, tiztc_proc_buffers_ready,
         tizservant_allocate_resources, tiztc_proc_allocate_resources,
         tizservant_deallocate_resources, tiztc_proc_deallocate_resources,
         tizservant_prepare_to_transfer, tiztc_proc_prepare_to_transfer,
         tizservant_transfer_and_process, tiztc_proc_transfer_and_process,
         tizservant_stop_and_return, tiztc_proc_stop_and_return, 0);
    }

}

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
 * @file   icerprc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - Icecast-like Http Sink processor class
 * implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#include "OMX_Core.h"

#include "tizkernel.h"
#include "tizscheduler.h"

#include "icerprc.h"
#include "icerprc_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.prc"
#endif

/*
 * icerprc
 */

static void *
icer_proc_ctor (void *ap_obj, va_list * app)
{
  struct icerprc *p_obj = super_ctor (icerprc, ap_obj, app);
  p_obj->eos_ = false;
  return p_obj;
}

static void *
icer_proc_dtor (void *ap_obj)
{
  return super_dtor (icerprc, ap_obj);
}

static OMX_ERRORTYPE
icer_proc_read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  struct icerprc *p_obj = (struct icerprc *) ap_obj;
  (void) p_obj;
  return OMX_ErrorNone;
}

/*
 * from tizservant class
 */

static OMX_ERRORTYPE
icer_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  struct icerprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;

  assert (ap_obj);

  (void) p_parent;
  (void) p_obj;

  TIZ_LOG (TIZ_LOG_TRACE, "Resource allocation complete... "
           "icerprc = [%p]!!!", p_obj);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_proc_deallocate_resources (void *ap_obj)
{
  struct icerprc *p_obj = ap_obj;
  assert (ap_obj);

  (void) p_obj;

  TIZ_LOG (TIZ_LOG_TRACE, "Resource deallocation complete..."
           "icerprc = [%p]!!!", p_obj);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  struct icerprc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "pid [%d]", a_pid);

  TIZ_LOG (TIZ_LOG_TRACE,
           "Prepared to transfer buffers...p_obj = [%p]!!!", p_obj);

  return OMX_ErrorNone;

}

static OMX_ERRORTYPE
icer_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  struct icerprc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "pid [%d]", a_pid);

  TIZ_LOG (TIZ_LOG_TRACE, "Awaiting buffers...p_obj = [%p]!!!", p_obj);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_proc_stop_and_return (void *ap_obj)
{
  struct icerprc *p_obj = ap_obj;
  assert (ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "Stopped buffer transfer...p_obj = [%p]!!!", p_obj);

  return OMX_ErrorNone;
}

/*
 * from tizproc class
 */

static OMX_ERRORTYPE
icer_proc_buffers_ready (const void *ap_obj)
{
  const struct icerprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;
  tiz_pd_set_t ports;
  void *p_krn = tiz_get_krn (p_parent->p_hdl_);
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "Buffers ready...");

  if (!(p_obj->eos_))
    {
      TIZ_PD_ZERO (&ports);

      TIZ_UTIL_TEST_ERR (tizkernel_select (p_krn, 1, &ports));

      if (TIZ_PD_ISSET (0, &ports))
        {
          TIZ_UTIL_TEST_ERR (tizkernel_claim_buffer (p_krn, 0, 0, &p_hdr));
          TIZ_LOG (TIZ_LOG_TRACE, "Claimed HEADER [%p]...", p_hdr);
          TIZ_UTIL_TEST_ERR (icer_proc_read_buffer (ap_obj, p_hdr));
          tizkernel_relinquish_buffer (p_krn, 0, p_hdr);
        }
    }

  return OMX_ErrorNone;
}

/*
 * initialization
 */

const void *icerprc;

void
init_icerprc (void)
{
  if (!icerprc)
    {
      init_tizproc ();
      icerprc =
        factory_new
        (tizproc_class,
         "icerprc",
         tizproc,
         sizeof (struct icerprc),
         ctor, icer_proc_ctor,
         dtor, icer_proc_dtor,
         tizproc_buffers_ready, icer_proc_buffers_ready,
         tizservant_allocate_resources, icer_proc_allocate_resources,
         tizservant_deallocate_resources, icer_proc_deallocate_resources,
         tizservant_prepare_to_transfer, icer_proc_prepare_to_transfer,
         tizservant_transfer_and_process, icer_proc_transfer_and_process,
         tizservant_stop_and_return, icer_proc_stop_and_return, 0);
    }
}

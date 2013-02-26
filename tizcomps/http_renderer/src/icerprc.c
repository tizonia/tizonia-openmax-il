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

#include "OMX_Core.h"
#include "OMX_TizoniaExt.h"

#include "tizkernel.h"
#include "tizscheduler.h"

#include "icerprc.h"
#include "icercon.h"
#include "icerprc_decls.h"


#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.prc"
#endif

static OMX_ERRORTYPE
read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
{
  struct icerprc *p_obj = (struct icerprc *) ap_obj;
  (void) p_obj;
  return OMX_ErrorNone;
}

/*
 * icerprc
 */

static void *
icer_proc_ctor (void *ap_obj, va_list * app)
{
  struct icerprc *p_obj = super_ctor (icerprc, ap_obj, app);
  p_obj->bind_address_ = NULL;
  p_obj->listening_port_ = 0;
  p_obj->mount_name_ = NULL;
  p_obj->max_clients_ = 0;
  p_obj->nclients_ = 0;
  p_obj->burst_size_ = 65536;
  p_obj->eos_ = false;
  p_obj->srv_sockfd_ = -1;
  p_obj->p_clnt_socket_lst_ = NULL;
  p_obj->p_srv_ev_io_ = NULL;
  p_obj->p_clnt_ev_io_lst_ = NULL;
  p_obj->p_listener_lst_ = NULL;
  return p_obj;
}

static void *
icer_proc_dtor (void *ap_obj)
{
  return super_dtor (icerprc, ap_obj);
}

/*
 * from tizservant class
 */

static OMX_ERRORTYPE
icer_proc_allocate_resources (void *ap_obj, OMX_U32 a_pid)
{
  struct icerprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;
  void *p_krn = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_TIZONIA_PARAM_HTTPSERVERTYPE httpsrv;

  assert (NULL != ap_obj);
  assert (p_parent->p_hdl_);

  p_krn = tiz_get_krn (p_parent->p_hdl_);
  assert (p_krn);

  /* Retrieve http server configuration from the component's config port */
  httpsrv.nSize = sizeof (OMX_TIZONIA_PARAM_HTTPSERVERTYPE);
  httpsrv.nVersion.nVersion = OMX_VERSION;
  if (OMX_ErrorNone != (rc = tizapi_GetParameter
                        (p_krn, p_parent->p_hdl_,
                         OMX_TizoniaIndexParamHttpServer, &httpsrv)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                     TIZ_CBUF (p_parent->p_hdl_),
                     "[%s] : Error retrieving HTTPSERVERTYPE from port",
                     tiz_err_to_str (rc));
      return rc;
    }

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "nListeningPort = [%d] nMaxClients = [%d] ",
                 httpsrv.nListeningPort, httpsrv.nMaxClients);

  p_obj->listening_port_ = httpsrv.nListeningPort;
  p_obj->max_clients_ = httpsrv.nMaxClients;

  if (OMX_ErrorNone != (rc = tiz_event_loop_init ()))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                     TIZ_CBUF (p_parent->p_hdl_),
                     "[%s] : Error starting event loop",
                     tiz_err_to_str (rc));
      return rc;
    }

  return icer_con_setup_sockets (p_obj, p_parent->p_hdl_);
}

static OMX_ERRORTYPE
icer_proc_deallocate_resources (void *ap_obj)
{
  struct icerprc *p_obj = ap_obj;

  assert (NULL != ap_obj);

  icer_con_teardown_sockets (p_obj);
  tiz_event_loop_destroy ();

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_proc_prepare_to_transfer (void *ap_obj, OMX_U32 a_pid)
{
  struct icerprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;
/*   OMX_ERRORTYPE rc = OMX_ErrorNone; */

  assert (NULL != ap_obj);
  assert (NULL != p_parent->p_hdl_);

  TIZ_LOG (TIZ_LOG_TRACE,
           "Server starts listening on port [%d]", p_obj->listening_port_);

  return icer_con_start_listening (p_obj, p_parent->p_hdl_);
}

static OMX_ERRORTYPE
icer_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  struct icerprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;

  assert (NULL != ap_obj);
  assert (NULL != p_parent->p_hdl_);

  TIZ_LOG (TIZ_LOG_TRACE, "pid [%d]", a_pid);

  return icer_con_start_io_watchers (p_obj, p_parent->p_hdl_);
}

static OMX_ERRORTYPE
icer_proc_stop_and_return (void *ap_obj)
{
  struct icerprc *p_obj = ap_obj;
  const struct tizservant *p_parent = ap_obj;

  assert (NULL != ap_obj);

  TIZ_LOG (TIZ_LOG_TRACE, "Stopped buffer transfer...p_obj = [%p]", p_obj);

  return icer_con_stop_io_watchers (p_obj, p_parent->p_hdl_);
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
          TIZ_UTIL_TEST_ERR (read_buffer (ap_obj, p_hdr));
          tizkernel_relinquish_buffer (p_krn, 0, p_hdr);
        }
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_receive_event_io (void *ap_obj,
                       tiz_event_io_t * ap_ev_io, int a_fd,
                       int a_events)
{
  struct icerprc *p_obj = ap_obj;
  struct tizservant *p_parent = ap_obj;
  icer_listener_t *p_listener = NULL;

  assert (NULL != p_obj);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (p_parent->p_hdl_),
                 TIZ_CBUF (p_parent->p_hdl_),
                 "received io event on fd [%d] ", a_fd);

  if (a_fd == p_obj->srv_sockfd_)
    {
      p_listener = icer_con_accept_connection (p_obj, p_parent->p_hdl_);
      p_obj->p_listener_lst_ = p_listener;
    }

  icer_con_stop_io_watchers (p_obj, p_parent->p_hdl_);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_receive_event_timer (void *ap_obj, tiz_event_timer_t * ap_ev_timer)
{
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
         tizservant_stop_and_return, icer_proc_stop_and_return,
         tizproc_receive_event_io, icer_receive_event_io,
         tizproc_receive_event_timer, icer_receive_event_timer,
         0);
    }
}

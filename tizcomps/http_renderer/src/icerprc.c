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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#include "OMX_Core.h"
#include "OMX_TizoniaExt.h"

#include "tizkernel.h"
#include "tizscheduler.h"

#include "icerprc.h"
#include "icerprc_decls.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.prc"
#endif

#define ICE_RENDERER_SOCK_ERROR (int)-1
#define ICE_RENDERER_LISTEN_QUEUE 5

static int
create_server_socket (OMX_HANDLETYPE ap_hdl, int a_port,
                      const char *a_interface)
{
  struct sockaddr_storage sa;
  struct addrinfo hints, *res, *ai;
  char service[10];
  int sockfd, getaddrc;

  assert (a_port >= 0);
  assert (NULL != a_interface);

  tiz_mem_set (&sa, 0, sizeof (sa));
  tiz_mem_set (&hints, 0, sizeof (hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_flags =
    AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV | AI_NUMERICHOST;
  hints.ai_socktype = SOCK_STREAM;
  snprintf (service, sizeof (service), "%d", a_port);

  if ((getaddrc = getaddrinfo (a_interface, service, &hints, &res)) != 0)
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[ICE_RENDERER_SOCK_ERROR] : %s.",
                     gai_strerror (getaddrc));
      return ICE_RENDERER_SOCK_ERROR;
    }

  ai = res;
  do
    {
      int on = 1;
      sockfd = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol);
      if (sockfd < 0)
        {
          continue;
        }

      setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &on,
                  sizeof (on));
      on = 0;

      if (bind (sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
        {
          close (sockfd);
          continue;
        }

      freeaddrinfo (res);
      return sockfd;

    }
  while ((ai = ai->ai_next));

  freeaddrinfo (res);

  return ICE_RENDERER_SOCK_ERROR;
}

static OMX_ERRORTYPE
set_non_blocking (int sockfd)
{
  int rc, flags;

  if ((flags = fcntl (sockfd, F_GETFL, 0)) < 0)
    {
      return OMX_ErrorUndefined;
    }

  flags |= O_NONBLOCK;
  if ((rc = fcntl (sockfd, F_SETFL, flags)) < 0)
    {
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

static void
srv_ev_io_cback (tiz_event_io_t * ap_ev_io, int fd, int events)
{

}

static void
clnt_ev_io_cback (tiz_event_io_t * ap_ev_io, int fd, int events)
{

}

static inline void
clean_up_io_events (void *ap_obj)
{
  struct icerprc *p_obj = ap_obj;
  int i = 0;

  assert (NULL != p_obj);

  if (NULL != p_obj->p_srv_ev_io_)
    {
      tiz_event_io_destroy (p_obj->p_srv_ev_io_);
      p_obj->p_srv_ev_io_ = NULL;
    }

  for (i = 0; i < p_obj->max_clients_; i++)
    {
      if (NULL != p_obj->p_clnt_ev_io_lst_[i])
        {
          tiz_event_io_destroy (p_obj->p_clnt_ev_io_lst_[i]);
        }
    }

  tiz_mem_free (p_obj->p_clnt_ev_io_lst_);
  p_obj->p_clnt_ev_io_lst_ = NULL;
}

static OMX_ERRORTYPE
allocate_io_events (void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  struct icerprc *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int i = 0;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (OMX_ErrorNone !=
      (rc =
       tiz_event_io_init (&p_obj->p_srv_ev_io_, srv_ev_io_cback,
                          p_obj->srv_sockfd_, TIZ_EVENT_READ)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : Error initializing the server's io event",
                     tiz_err_to_str (rc));
      goto end;
    }

  p_obj->p_clnt_ev_io_lst_ = (tiz_event_io_t **)
    tiz_mem_calloc (p_obj->max_clients_, sizeof (tiz_event_io_t *));

  if (NULL == p_obj->p_clnt_ev_io_lst_)
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Error allocating "
                     "the clients io event list");
      goto end;
    }

  for (i = 0; i < p_obj->max_clients_; i++)
    {
      if (OMX_ErrorNone !=
          (rc =
           tiz_event_io_init (&(p_obj->p_clnt_ev_io_lst_[i]),
                              clnt_ev_io_cback, p_obj->p_clnt_socket_lst_[i],
                              TIZ_EVENT_READ)))
        {
          TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                         "[%s] : Error initializing the server's io event",
                         tiz_err_to_str (rc));
          goto end;
        }
    }

end:

  if (OMX_ErrorNone != rc)
    {
      clean_up_io_events (p_obj);
    }

  return rc;
}

static OMX_ERRORTYPE
setup_sockets (void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  struct icerprc *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int sockfd = -1;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (NULL ==
      (p_obj->p_clnt_socket_lst_ = (int *)
       tiz_mem_calloc (p_obj->max_clients_, sizeof (int))))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to allocate "
                     "the list of server sockets");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }

  if (ICE_RENDERER_SOCK_ERROR ==
      (sockfd =
       create_server_socket (ap_hdl, p_obj->listening_port_,
                             p_obj->bind_address_)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to create "
                     "the server socket.");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }

  if (OMX_ErrorNone != (rc = allocate_io_events (p_obj, ap_hdl)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : Unable to allocate io events.",
                     tiz_err_to_str (rc));
      goto end;
    }

end:

  if (OMX_ErrorNone != rc)
    {
      tiz_mem_free (p_obj->p_clnt_socket_lst_);
      p_obj->p_clnt_socket_lst_ = NULL;
      if (-1 != sockfd)
        {
          close (sockfd);
          sockfd = -1;
        }
    }

  p_obj->srv_sockfd_ = sockfd;

  return rc;
}

static void
teardown_sockets (void *ap_obj)
{
  struct icerprc *p_obj = ap_obj;

  assert (NULL != p_obj);

  p_obj->srv_sockfd_ = -1;
  tiz_mem_free (p_obj->p_clnt_socket_lst_);
  p_obj->p_clnt_socket_lst_ = NULL;
}

static OMX_ERRORTYPE
start_listening (void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  struct icerprc *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (listen (p_obj->srv_sockfd_, ICE_RENDERER_LISTEN_QUEUE) ==
      ICE_RENDERER_SOCK_ERROR)
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to mark "
                     "socket as passive (%s).", strerror (errno));
      return OMX_ErrorInsufficientResources;
    }

  if (OMX_ErrorNone != (rc = set_non_blocking (p_obj->srv_sockfd_)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to set"
                     "socket as non-blocking.");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

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
  p_obj->burst_size_ = 65536;
  p_obj->eos_ = false;
  p_obj->srv_sockfd_ = -1;
  p_obj->p_clnt_socket_lst_ = NULL;
  p_obj->p_srv_ev_io_ = NULL;
  p_obj->p_clnt_ev_io_lst_ = NULL;
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
  OMX_TIZONIA_AUDIO_PARAM_HTTPSERVERTYPE httpsrv;

  assert (NULL != ap_obj);
  assert (p_parent->p_hdl_);

  p_krn = tiz_get_krn (p_parent->p_hdl_);
  assert (p_krn);

  /* Retrieve http server configuration from the component's config port */
  httpsrv.nSize = sizeof (OMX_TIZONIA_AUDIO_PARAM_HTTPSERVERTYPE);
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

  return setup_sockets (p_obj, p_parent->p_hdl_);
}

static OMX_ERRORTYPE
icer_proc_deallocate_resources (void *ap_obj)
{
  struct icerprc *p_obj = ap_obj;

  assert (NULL != ap_obj);

  teardown_sockets (p_obj);
  clean_up_io_events (p_obj);
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
  assert (p_parent->p_hdl_);

  TIZ_LOG (TIZ_LOG_TRACE,
           "Prepared to transfer buffers...p_obj = [%p]!!!", p_obj);

  return start_listening (p_obj, p_parent->p_hdl_);
}

static OMX_ERRORTYPE
icer_proc_transfer_and_process (void *ap_obj, OMX_U32 a_pid)
{
  TIZ_LOG (TIZ_LOG_TRACE, "pid [%d]", a_pid);
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_proc_stop_and_return (void *ap_obj)
{
  struct icerprc *p_obj = ap_obj;
  assert (NULL != ap_obj);

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
          TIZ_UTIL_TEST_ERR (read_buffer (ap_obj, p_hdr));
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

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
#include <unistd.h>
#include <fcntl.h>

#include "OMX_Core.h"
#include "OMX_TizoniaExt.h"

#include "tizkernel.h"
#include "tizscheduler.h"

#include "icerprc.h"
#include "icerprc_decls.h"

#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.prc"
#endif

static int
get_server_socket (int port, const char *sinterface)
{
  struct sockaddr_storage sa;
  struct addrinfo hints, *res, *ai;
  char service [10];
  int sock;

  if (port < 0)
    return SOCK_ERROR;

  memset (&sa, 0, sizeof(sa));
  memset (&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV | AI_NUMERICHOST;
  hints.ai_socktype = SOCK_STREAM;
  snprintf (service, sizeof (service), "%d", port);

  if (getaddrinfo (sinterface, service, &hints, &res))
    return SOCK_ERROR;
  ai = res;
  do
    {
      int on = 1;
      sock = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol);
      if (sock < 0)
        continue;

      setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&on, sizeof(on));
      on = 0;
#ifdef IPV6_V6ONLY
      setsockopt (sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof on);
#endif

      if (bind (sock, ai->ai_addr, ai->ai_addrlen) < 0)
        {
          sock_close (sock);
          continue;
        }
      freeaddrinfo (res);
      return sock;

    } while ((ai = ai->ai_next));

  freeaddrinfo (res);
  return SOCK_ERROR;
}

static OMX_ERRORTYPE
set_non_blocking (int sockfd)
{
  int rc, flags;

  if ((flags = fcntl(sockfd, F_GETFL, 0)) < 0)
    {
      return OMX_ErrorUndefined;
    }

  flags |= O_NONBLOCK;
  if ((rc = fcntl(sockfd, F_SETFL, flags)) < 0)
    {
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
setup_sockets (void *ap_obj, OMX_HANDLETYPE ap_hdl, void *ap_krn)
{
  struct icerprc *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int sock = 0;

/*   OMX_ERRORTYPE ret_val = OMX_ErrorNone; */

  assert (ap_obj);
  assert (ap_hdl);
  assert (ap_krn);

  if (NULL ==
      (p_obj->p_srv_sock_lst_ =
       tiz_mem_calloc (p_obj->max_clients_, sizeof (int))))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to allocate "
                     "the list of server sockets");
      return OMX_ErrorInsufficientResources;
    }

  if (OMX_ErrorNone != (rc = set_non_blocking (sock)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to set "
                     "set socket as non-blocking.");
      tiz_mem_free (p_obj->p_srv_sock_lst_);
      p_obj->p_srv_sock_lst_ = NULL;
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
start_listening (void *ap_obj, OMX_HANDLETYPE ap_hdl, void *ap_krn)
{
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
icer_proc_read_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * p_hdr)
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
  p_obj->listening_port_ = 0;
  p_obj->max_clients_ = 0;
  p_obj->p_srv_sock_lst_ = NULL;
  p_obj->eos_ = false;
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
  OMX_ERRORTYPE ret_val = OMX_ErrorNone;
  OMX_TIZONIA_AUDIO_PARAM_HTTPSERVERTYPE httpsrv;

  assert (ap_obj);
  assert (p_parent->p_hdl_);

  p_krn = tiz_get_krn (p_parent->p_hdl_);
  assert (p_krn);

  /* Retrieve http server configuration from the component's config port */
  httpsrv.nSize = sizeof (OMX_TIZONIA_AUDIO_PARAM_HTTPSERVERTYPE);
  httpsrv.nVersion.nVersion = OMX_VERSION;
  if (OMX_ErrorNone != (ret_val = tizapi_GetParameter
                        (p_krn, p_parent->p_hdl_,
                         OMX_TizoniaIndexParamHttpServer, &httpsrv)))
    {
      TIZ_LOG (TIZ_LOG_ERROR,
               "[%s] : Error retrieving HTTPSERVERTYPE from port",
               tiz_err_to_str (ret_val));
      return ret_val;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "nListeningPort = [%d] nMaxClients = [%d] ",
           httpsrv.nListeningPort, httpsrv.nMaxClients);

  p_obj->listening_port_ = httpsrv.nListeningPort;
  p_obj->max_clients_ = httpsrv.nMaxClients;

  return setup_sockets (p_obj, p_parent->p_hdl_, p_krn);
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
  const struct tizservant *p_parent = ap_obj;
  void *p_krn = NULL;

  assert (ap_obj);
  assert (p_parent->p_hdl_);

  p_krn = tiz_get_krn (p_parent->p_hdl_);
  assert (p_krn);

  TIZ_LOG (TIZ_LOG_TRACE,
           "Prepared to transfer buffers...p_obj = [%p]!!!", p_obj);

  return start_listening (p_obj, p_parent->p_hdl_, p_krn);
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

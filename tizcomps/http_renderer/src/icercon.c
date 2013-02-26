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
 * @file   icercon.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - Http renderer socket management
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "tizosal.h"
#include "tizutils.h"
#include "icercon.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.prc.con"
#endif

#define ICE_RENDERER_SOCK_ERROR (int)-1
#define ICE_RENDERER_LISTEN_QUEUE 5

#ifdef INET6_ADDRSTRLEN
#define ICE_RENDERER_MAX_ADDR_LEN INET6_ADDRSTRLEN
#else
#define ICE_RENDERER_MAX_ADDR_LEN 46
#endif

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

static inline bool
valid_socket (int a_socketfd)
{
  int optval;
  socklen_t optlen = sizeof (int);
  return (0 == getsockopt (a_socketfd, SOL_SOCKET,
                           SO_TYPE, (void *) &optval, &optlen));
}

static inline int
set_nolinger (int sock)
{
  struct linger lin = { 0, 0 };
  return setsockopt (sock, SOL_SOCKET, SO_LINGER, (void *) &lin,
                     sizeof (struct linger));
}

static inline int
set_nodelay (int sock)
{
  int nodelay = 1;

  return setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (void *) &nodelay,
                     sizeof (int));
}

static inline int
set_keepalive (int sock)
{
  int keepalive = 1;
  return setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &keepalive,
                     sizeof (int));
}

static int
accept_socket (struct icerprc *ap_obj, OMX_HANDLETYPE ap_hdl, char *ap_ip,
               size_t a_ip_len)
{
  struct sockaddr_storage sa;
  int accepted_socket = ICE_RENDERER_SOCK_ERROR;
  socklen_t slen = sizeof (sa);

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_ip);

  if (!valid_socket (ap_obj->srv_sockfd_))
    {
      return ICE_RENDERER_SOCK_ERROR;
    }

  accepted_socket =
    accept (ap_obj->srv_sockfd_, (struct sockaddr *) &sa, &slen);

  if (accepted_socket != ICE_RENDERER_SOCK_ERROR)
    {
      if (getnameinfo ((struct sockaddr *) &sa, slen, ap_ip, a_ip_len,
                       NULL, 0, NI_NUMERICHOST))
        {
          /* TODO: Print log message with errno value  */
          snprintf (ap_ip, a_ip_len, "unknown");
        }

      set_nolinger (accepted_socket);
      set_keepalive (accepted_socket);
    }

  return accepted_socket;
}

static inline int
create_server_socket (OMX_HANDLETYPE ap_hdl, int a_port,
                      const char *a_interface)
{
  struct sockaddr_storage sa;
  struct addrinfo hints, *res, *ai;
  char service[10];
  int sockfd, getaddrc;

  assert (a_port >= 0);

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

static void
destroy_io_watchers (struct icerprc *ap_obj)
{
  int i = 0;

  assert (NULL != ap_obj);

  if (NULL != ap_obj->p_srv_ev_io_)
    {
      tiz_event_io_destroy (ap_obj->p_srv_ev_io_);
      ap_obj->p_srv_ev_io_ = NULL;
    }

  for (i = 0; i < ap_obj->max_clients_; i++)
    {
      if (NULL != ap_obj->p_clnt_ev_io_lst_[i])
        {
          tiz_event_io_destroy (ap_obj->p_clnt_ev_io_lst_[i]);
        }
    }

  tiz_mem_free (ap_obj->p_clnt_ev_io_lst_);
  ap_obj->p_clnt_ev_io_lst_ = NULL;
}

static OMX_ERRORTYPE
allocate_io_watchers (struct icerprc *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int i = 0;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (OMX_ErrorNone !=
      (rc =
       tiz_event_io_init (&ap_obj->p_srv_ev_io_, ap_hdl,
                          tiz_receive_event_io)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : Error initializing the server's io event",
                     tiz_err_to_str (rc));
      goto end;
    }

  tiz_event_io_set (ap_obj->p_srv_ev_io_, ap_obj->srv_sockfd_,
                    TIZ_EVENT_READ);

  ap_obj->p_clnt_ev_io_lst_ = (tiz_event_io_t **)
    tiz_mem_calloc (ap_obj->max_clients_, sizeof (tiz_event_io_t *));

  if (NULL == ap_obj->p_clnt_ev_io_lst_)
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Error allocating "
                     "the clients io event list");
      goto end;
    }

  for (i = 0; i < ap_obj->max_clients_; i++)
    {
      if (OMX_ErrorNone !=
          (rc =
           tiz_event_io_init (&(ap_obj->p_clnt_ev_io_lst_[i]), ap_hdl,
                              tiz_receive_event_io)))
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
      destroy_io_watchers (ap_obj);
    }

  return rc;
}


static icer_listener_t *
create_listener (struct icerprc *ap_obj, OMX_HANDLETYPE ap_hdl,
                 icer_connection_t * con)
{
  icer_listener_t *p_listener = NULL;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (NULL != p_listener)
    {
      if (ap_obj->max_clients_ == ap_obj->nclients_)
        {
          TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                         "client limit reached [%d]", ap_obj->max_clients_);
        }
      else
        {
          ap_obj->max_clients_++;
          p_listener = 
            (icer_listener_t *) tiz_mem_calloc (1, sizeof (icer_listener_t));
          p_listener->con = con;
          p_listener->pos = 0;

          set_non_blocking (p_listener->con->sock);
          set_nodelay (p_listener->con->sock);
        }
    }

  return p_listener;
}

static icer_connection_t *
create_connection (int connected_socketfd, char *ap_ip)
{
  icer_connection_t *con;
  con = (icer_connection_t *) tiz_mem_calloc (1, sizeof (icer_connection_t));
  if (con)
    {
      con->sock = connected_socketfd;
      con->con_time = time(NULL);
      con->ip = ap_ip;
    }

  return con;
}

OMX_ERRORTYPE
icer_con_setup_sockets (struct icerprc * ap_obj, OMX_HANDLETYPE ap_hdl)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int sockfd = -1;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (NULL ==
      (ap_obj->p_clnt_socket_lst_ = (int *)
       tiz_mem_calloc (ap_obj->max_clients_, sizeof (int))))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to allocate "
                     "the list of server sockets");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }

  if (ICE_RENDERER_SOCK_ERROR ==
      (sockfd =
       create_server_socket (ap_hdl, ap_obj->listening_port_,
                             ap_obj->bind_address_)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to create "
                     "the server socket.");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }

  ap_obj->srv_sockfd_ = sockfd;

  if (OMX_ErrorNone != (rc = allocate_io_watchers (ap_obj, ap_hdl)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : Unable to allocate io events.",
                     tiz_err_to_str (rc));
      goto end;
    }

 end:

  if (OMX_ErrorNone != rc)
    {
      tiz_mem_free (ap_obj->p_clnt_socket_lst_);
      ap_obj->p_clnt_socket_lst_ = NULL;
      if (-1 != sockfd)
        {
          close (sockfd);
          ap_obj->srv_sockfd_ = -1;
        }
    }

  return rc;
}

void
icer_con_teardown_sockets (struct icerprc *ap_obj)
{
  assert (NULL != ap_obj);

  tiz_mem_free (ap_obj->p_clnt_socket_lst_);
  ap_obj->p_clnt_socket_lst_ = NULL;

  if (-1 != ap_obj->srv_sockfd_)
    {
      close (ap_obj->srv_sockfd_);
      ap_obj->srv_sockfd_ = -1;
    }

  destroy_io_watchers (ap_obj);
}

OMX_ERRORTYPE
icer_con_start_listening (struct icerprc *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  if (listen (ap_obj->srv_sockfd_, ICE_RENDERER_LISTEN_QUEUE) ==
      ICE_RENDERER_SOCK_ERROR)
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to mark "
                     "socket as passive (%s).", strerror (errno));
      return OMX_ErrorInsufficientResources;
    }

  if (OMX_ErrorNone != (rc = set_non_blocking (ap_obj->srv_sockfd_)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to set"
                     "socket as non-blocking.");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

icer_listener_t *
icer_con_accept_connection (struct icerprc * ap_obj, OMX_HANDLETYPE ap_hdl)
{
  char *p_ip;
  icer_connection_t *con = NULL;
  icer_listener_t *listener = NULL;
  int connected_socket = ICE_RENDERER_SOCK_ERROR;

  /* Allocate enough room for an ipv4 or ipv6 IP address */
  p_ip = (char *) tiz_mem_alloc (ICE_RENDERER_MAX_ADDR_LEN);
  connected_socket =
    accept_socket (ap_obj, ap_hdl, p_ip, ICE_RENDERER_MAX_ADDR_LEN);

  if (connected_socket != ICE_RENDERER_SOCK_ERROR)
    {
      /* Make any IPv4 mapped IPv6 address look like a normal IPv4 address */
      if (strncmp (p_ip, "::ffff:", 7) == 0)
        {
          memmove (p_ip, p_ip + 7, strlen (p_ip + 7) + 1);
        }

      con = create_connection (connected_socket, p_ip);
      if (NULL == con)
        {
          close (connected_socket);
        }
    }

  if (NULL != con)
    {
      listener = create_listener (ap_obj, ap_hdl, con);
    }

  return listener;
}

OMX_ERRORTYPE
icer_con_start_io_watchers (struct icerprc * ap_obj, OMX_HANDLETYPE ap_hdl)
{
  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Starting server io watcher on socket fd [%d] ",
                 ap_obj->srv_sockfd_);

  return tiz_event_io_start (ap_obj->p_srv_ev_io_);
}

OMX_ERRORTYPE
icer_con_stop_io_watchers (struct icerprc * ap_obj, OMX_HANDLETYPE ap_hdl)
{
  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl),
                 TIZ_CBUF (ap_hdl),
                 "stopping io watcher on fd [%d] ", ap_obj->srv_sockfd_);

  return tiz_event_io_stop (ap_obj->p_srv_ev_io_);
}

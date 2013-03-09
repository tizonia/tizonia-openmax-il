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
 * @brief Tizonia OpenMAX IL - Http renderer's socket management functions
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
valid_socket (int a_sockfd)
{
  int optval;
  socklen_t optlen = sizeof (int);
  return (0 == getsockopt (a_sockfd, SOL_SOCKET,
                           SO_TYPE, (void *) &optval, &optlen));
}

static inline int
set_nolinger (int sock)
{
  struct linger lin = { 0, 0 };
  /* linger inactive. close call will return immediately to the caller, and any
     pending data will be delivered if possible. */
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
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                     TIZ_CBUF (ap_hdl),
                     "Invalid server socket fd [%d] ", ap_obj->srv_sockfd_);
      return ICE_RENDERER_SOCK_ERROR;
    }

  accepted_socket =
    accept (ap_obj->srv_sockfd_, (struct sockaddr *) &sa, &slen);

  if (accepted_socket != ICE_RENDERER_SOCK_ERROR)
    {
      int err = 0;
      if (0 != (err = getnameinfo ((struct sockaddr *) &sa, slen,
                                   ap_ip, a_ip_len, NULL, 0, NI_NUMERICHOST)))
        {
          snprintf (ap_ip, a_ip_len, "unknown");
          TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                         TIZ_CBUF (ap_hdl),
                         "getnameinfo returned error [%s]",
                         gai_strerror (err));
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

  tiz_event_io_set (ap_obj->p_srv_ev_io_, ap_obj->srv_sockfd_, TIZ_EVENT_READ);

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

static void
destroy_connection (icer_connection_t * ap_con)
{
  if (NULL != ap_con)
    {
      tiz_mem_free (ap_con->p_ip);
      tiz_mem_free (ap_con);
    }
}

static void
destroy_listener (icer_listener_t * ap_lstnr)
{
  if (NULL != ap_lstnr)
    {
      if (NULL != ap_lstnr->p_parser)
        {
          tiz_http_parser_destroy (ap_lstnr->p_parser);
        }
      tiz_mem_free (ap_lstnr->buf.p_data);
      /* NOTE: About ap_lstnr->p_con */
      /* The connection should be destroyed outside this function */
      tiz_mem_free (ap_lstnr);
    }
}

static OMX_ERRORTYPE
create_listener (struct icerprc *ap_obj, OMX_HANDLETYPE ap_hdl,
                 icer_listener_t ** app_lstnr,  icer_connection_t * ap_con)
{
  icer_listener_t *p_lstnr = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != app_lstnr);
  assert (NULL != ap_con);

  if (NULL == (p_lstnr = (icer_listener_t *)
               tiz_mem_calloc (1, sizeof (icer_listener_t))))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : "
                     "Could not allocate the listener structure.");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }

  p_lstnr->p_next = NULL;
  p_lstnr->p_con = ap_con;
  p_lstnr->respcode = 200;
  p_lstnr->intro_offset = 0;
  p_lstnr->pos = 0;
  p_lstnr->buf.len = ICE_LISTENER_BUF_SIZE;
  p_lstnr->buf.count = 1;
  p_lstnr->buf.sync_point = false;
  p_lstnr->p_parser = NULL;

  if (NULL !=
      (p_lstnr->buf.p_data =
       (char *) tiz_mem_alloc (ICE_LISTENER_BUF_SIZE)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : "
                     "Could not allocate the listener's buffer.");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }
  p_lstnr->buf.p_data[ICE_LISTENER_BUF_SIZE - 1] = '\000';

  if (OMX_ErrorNone != (tiz_http_parser_init
                        (&(p_lstnr->p_parser),
                         ETIZHttpParserTypeRequest)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : Error while initializing the http parser.",
                     tiz_err_to_str (rc));
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }

  set_non_blocking (p_lstnr->p_con->sock);
  set_nodelay (p_lstnr->p_con->sock);

 end:

  if (OMX_ErrorNone != rc)
    {
      destroy_listener (p_lstnr);
      p_lstnr = NULL;
    }
  else
    {
      ap_obj->nclients_++;
      *app_lstnr = p_lstnr;
    }

  return rc;
}

static int
read_from_listener (struct icerprc *ap_obj, OMX_HANDLETYPE ap_hdl,
                    icer_listener_t * ap_lstnr, char *ap_buf,
                    unsigned int a_len)
{
  icer_connection_t *p_con = NULL;
  icer_listener_buffer_t *p_buf = NULL;
  int len = ICE_LISTENER_BUF_SIZE - 1;  /* - offset */

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_con);

  p_con = ap_lstnr->p_con;
  p_buf = &ap_lstnr->buf;
  assert (NULL != p_buf->p_data);

  if (p_con->con_time + ICE_DEFAULT_HEADER_TIMEOUT <= time (NULL))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Connection timed out.");
      len = 0;
    }
  else
    {
      len = recv (p_con->sock, p_buf->p_data, p_buf->len, 0);
    }

  return len;
}

static ssize_t
build_http_response (char *ap_buf, size_t len, int status, const char *ap_msg)
{
  const char *http_version = "1.0";
  time_t now;
  struct tm result;
  struct tm *gmtime_result;
  char currenttime_buffer[80];
  char status_buffer[80];
  char contenttype_buffer[80];
  ssize_t ret;
  const char *statusmsg = NULL;
  const char *contenttype = "text/html";

  assert (NULL != ap_buf);

  switch (status)
    {
    case 200:
      {
        statusmsg = "OK";
      }
      break;
    case 400:
      {
        statusmsg = "Bad Request";
      }
      break;
    case 403:
      {
        statusmsg = "Forbidden";
      }
      break;
    case 404:
      {
        statusmsg = "File Not Found";
      }
      break;
    default:
      {
        statusmsg = "(unknown status code)";
      }
      break;
    }

  snprintf (status_buffer, sizeof (status_buffer), "HTTP/%s %d %s\r\n",
            http_version, status, statusmsg);
  snprintf (contenttype_buffer, sizeof (contenttype_buffer),
            "Content-Type: %s\r\n", contenttype);

  time (&now);
  gmtime_result = gmtime_r (&now, &result);


  strftime (currenttime_buffer, sizeof (currenttime_buffer),
            "Date: %a, %d-%b-%Y %X GMT\r\n", gmtime_result);

  ret = snprintf (ap_buf, len, "%sServer: %s\r\n%s%s%s%s",
                  status_buffer,
                  "Tizonia HTTP Sink",
                  currenttime_buffer,
                  contenttype_buffer,
                  (ap_msg ? "\r\n" : ""), (ap_msg ? ap_msg : ""));

  return ret;
}

static void
send_http_error (struct icerprc *ap_obj, OMX_HANDLETYPE ap_hdl,
                 icer_listener_t * ap_lstnr, int a_error,
                 const char *ap_err_msg)
{
  ssize_t resp_size = 0;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->buf.p_data);
  assert (NULL != ap_lstnr->p_con);
  assert (NULL != ap_err_msg);

  ap_lstnr->buf.p_data[ICE_LISTENER_BUF_SIZE - 1] = '\000';
  resp_size = build_http_response (ap_lstnr->buf.p_data,
                                   ICE_LISTENER_BUF_SIZE - 1, a_error, "");

  snprintf (ap_lstnr->buf.p_data + resp_size,
            ICE_LISTENER_BUF_SIZE - resp_size,
            "<html><head><title>Error %i</title></head><body><b>%i - %s</b>"
            "</body></html>\r\n", a_error, a_error, ap_err_msg);

  ap_lstnr->buf.len = strlen (ap_lstnr->buf.p_data);

  send (ap_lstnr->p_con->sock, ap_lstnr->buf.p_data, ap_lstnr->buf.len, 0);
}

static OMX_ERRORTYPE
handle_listener (struct icerprc *ap_obj, OMX_HANDLETYPE ap_hdl,
                 icer_listener_t * ap_lstnr)
{
  int nparsed = 0;
  int nread = -1;
  bool some_error = true;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_obj);
  assert (NULL != ap_hdl);
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_parser);

  if (ap_obj->nclients_ > ap_obj->max_clients_)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "client limit reached [%d]", ap_obj->max_clients_);
      send_http_error (ap_obj, ap_hdl, ap_lstnr, 400, "Client limit reached");
      goto end;
    }

  nread = read_from_listener (ap_obj, ap_hdl, ap_lstnr, ap_lstnr->buf.p_data,
                              ap_lstnr->buf.len);

  if (nread > 0)
    {
      nparsed = tiz_http_parser_parse (ap_lstnr->p_parser,
                                   ap_lstnr->buf.p_data, nread);
    }
  else
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Could not read any data from socket.");
      /* This could be due to a connection time out or to the peer having
         closed the connection already */
      goto end;
    }

  if (nparsed != nread)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Bad http request");
      send_http_error (ap_obj, ap_hdl, ap_lstnr, 400, "Bad request");
      goto end;
    }

  if (0 != strcmp ("GET", tiz_http_parser_get_method (ap_lstnr->p_parser)))
    {
      send_http_error (ap_obj, ap_hdl, ap_lstnr, 405, "Method not allowed");
      goto end;
    }

  if (0 != strcmp ("/", tiz_http_parser_get_url (ap_lstnr->p_parser)))
    {
      send_http_error (ap_obj, ap_hdl, ap_lstnr, 405, "Unathorized");
      goto end;
    }

  some_error = false;

end:

  if (some_error)
    {
      rc = OMX_ErrorInsufficientResources;
    }

  return rc;
}

static icer_connection_t *
create_connection (int connected_sockfd, char *ap_ip)
{
  icer_connection_t *p_con = NULL;
  if (NULL != (p_con = (icer_connection_t *)
               tiz_mem_calloc (1, sizeof (icer_connection_t))));
  {
    p_con->sock = connected_sockfd;
    p_con->con_time = time (NULL);
    p_con->p_ip = ap_ip;
  }

  return p_con;
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
  char *p_ip = NULL;
  icer_connection_t *p_con = NULL;
  icer_listener_t *p_lstnr = NULL;
  int conn_sock = ICE_RENDERER_SOCK_ERROR;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  bool some_error = true;

  if (NULL != (p_ip = (char *) tiz_mem_alloc (ICE_RENDERER_MAX_ADDR_LEN)))
    {
      conn_sock =
        accept_socket (ap_obj, ap_hdl, p_ip, ICE_RENDERER_MAX_ADDR_LEN);

      if (ICE_RENDERER_SOCK_ERROR == conn_sock)
        {
          TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                         TIZ_CBUF (ap_hdl),
                         "Error found while accepting the connection");
          goto end;
        }

      if (NULL == (p_con = create_connection (conn_sock, p_ip)))
        {
          TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                         TIZ_CBUF (ap_hdl),
                         "Error found while creating the connection");
          goto end;
        }

      if (OMX_ErrorNone != (rc = create_listener (ap_obj, ap_hdl, &p_lstnr,
                                                  p_con)))
        {
          goto end;
        }

      if (OMX_ErrorNone != handle_listener (ap_obj, ap_hdl, p_lstnr))
        {
          goto end;
        }

      some_error = false;
    }

end:

  if (some_error)
    {
      if (NULL != p_lstnr)
        {
          destroy_listener (p_lstnr);
          p_lstnr = NULL;
        }

      if (NULL != p_con)
        {
          destroy_connection (p_con);
          p_con = NULL;
          /* ip address has been deleted already by destroy_connection */
          p_ip = NULL;
        }
      if (ICE_RENDERER_SOCK_ERROR != conn_sock)
        {
          close (conn_sock);
        }
    }

  return p_lstnr;
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

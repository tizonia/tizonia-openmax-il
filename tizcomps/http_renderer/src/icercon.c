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

#define ICE_LISTENER_BUF_SIZE  4096
#define ICE_DEFAULT_HEADER_TIMEOUT 10
#define ICE_RENDERER_LISTEN_QUEUE 5

#ifdef INET6_ADDRSTRLEN
#define ICE_RENDERER_MAX_ADDR_LEN INET6_ADDRSTRLEN
#else
#define ICE_RENDERER_MAX_ADDR_LEN 46
#endif

typedef struct icer_listener_buffer icer_listener_buffer_t;
struct icer_listener_buffer
{
  unsigned int len;
  unsigned int count;
  char *p_data;
  bool sync_point;
};

typedef struct icer_connection icer_connection_t;
struct icer_connection
{
  time_t con_time;
  time_t discon_time;
  uint64_t sent_total;
  uint64_t sent_bytes;
  int sock;
  bool error;
  bool full;
  char *p_ip;
  char *p_host;
  tiz_event_io_t *p_ev_io;
};

typedef struct icer_listener icer_listener_t;
struct icer_listener
{
  icer_connection_t *p_con;
  int respcode;
  long intro_offset;
  unsigned long pos;
  icer_listener_buffer_t buf;
  tiz_http_parser_t *p_parser;
};

struct icer_server
{
  int lstn_sockfd;
  char *p_ip;
  tiz_event_io_t *p_srv_ev_io;
  OMX_U32 max_clients;
  OMX_U32 nclients;
  tiz_map_t *p_lstnrs;
  OMX_BUFFERHEADERTYPE *p_hdr;
  icer_buffer_emptied_f pf_emptied;
  icer_buffer_needed_f pf_needed;
  OMX_PTR p_arg;
};

static OMX_S32
listeners_map_compare_func (OMX_PTR ap_key1, OMX_PTR ap_key2)
{
  int *p_sockfd1 = (int *) ap_key1;
  int *p_sockfd2 = (int *) ap_key2;

  assert (NULL != ap_key1);
  assert (NULL != ap_key2);

  if (*p_sockfd1 == *p_sockfd2)
    {
      return 0;
    }
  else if (*p_sockfd1 < *p_sockfd2)
    {
      return -1;
    }
  else
    {
      return 1;
    }
}

static void
listeners_map_free_func (OMX_PTR ap_key, OMX_PTR ap_value)
{
  tiz_mem_free (ap_key);
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
   * pending data will be delivered if possible. */
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
accept_socket (icer_server_t * ap_server, OMX_HANDLETYPE ap_hdl, char *ap_ip,
               size_t a_ip_len)
{
  struct sockaddr_storage sa;
  int accepted_socket = ICE_RENDERER_SOCK_ERROR;
  socklen_t slen = sizeof (sa);

  assert (NULL != ap_server);
  assert (NULL != ap_hdl);
  assert (NULL != ap_ip);

  if (!valid_socket (ap_server->lstn_sockfd))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                     TIZ_CBUF (ap_hdl),
                     "Invalid server socket fd [%d] ", ap_server->lstn_sockfd);
      return ICE_RENDERER_SOCK_ERROR;
    }

  accepted_socket =
    accept (ap_server->lstn_sockfd, (struct sockaddr *) &sa, &slen);

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
destroy_server_io_watcher (icer_server_t * ap_server)
{
/*   int i = 0; */

  assert (NULL != ap_server);

  if (NULL != ap_server->p_srv_ev_io)
    {
      tiz_event_io_destroy (ap_server->p_srv_ev_io);
      ap_server->p_srv_ev_io = NULL;
    }

}

static OMX_ERRORTYPE
allocate_server_io_watcher (icer_server_t * ap_server, OMX_HANDLETYPE ap_hdl)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_server);
  assert (NULL != ap_hdl);

  if (OMX_ErrorNone !=
      (rc =
       tiz_event_io_init (&(ap_server->p_srv_ev_io), ap_hdl,
                          tiz_receive_event_io)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : Error initializing the server's io event",
                     tiz_err_to_str (rc));
      goto end;
    }

  tiz_event_io_set (ap_server->p_srv_ev_io, ap_server->lstn_sockfd,
                    TIZ_EVENT_READ);

end:

  if (OMX_ErrorNone != rc)
    {
      destroy_server_io_watcher (ap_server);
    }

  return rc;
}

static void
destroy_connection (icer_connection_t * ap_con)
{
  if (NULL != ap_con)
    {
      tiz_mem_free (ap_con->p_ip);
      tiz_mem_free (ap_con->p_host);
      tiz_event_io_destroy (ap_con->p_ev_io);
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
create_listener (icer_server_t * ap_server, OMX_HANDLETYPE ap_hdl,
                 icer_listener_t ** app_lstnr, icer_connection_t * ap_con)
{
  icer_listener_t *p_lstnr = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_server);
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

  p_lstnr->p_con = ap_con;
  p_lstnr->respcode = 200;
  p_lstnr->intro_offset = 0;
  p_lstnr->pos = 0;
  p_lstnr->buf.len = ICE_LISTENER_BUF_SIZE;
  p_lstnr->buf.count = 1;
  p_lstnr->buf.sync_point = false;
  p_lstnr->p_parser = NULL;

  if (NULL !=
      (p_lstnr->buf.p_data = (char *) tiz_mem_alloc (ICE_LISTENER_BUF_SIZE)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : "
                     "Could not allocate the listener's buffer.");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }
  p_lstnr->buf.p_data[ICE_LISTENER_BUF_SIZE - 1] = '\000';

  if (OMX_ErrorNone != (tiz_http_parser_init
                        (&(p_lstnr->p_parser), ETIZHttpParserTypeRequest)))
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
      ap_server->nclients++;
      *app_lstnr = p_lstnr;
    }

  return rc;
}

static int
read_from_listener (icer_server_t * ap_server, OMX_HANDLETYPE ap_hdl,
                    icer_listener_t * ap_lstnr, char *ap_buf,
                    unsigned int a_len)
{
  icer_connection_t *p_con = NULL;
  icer_listener_buffer_t *p_buf = NULL;
  int len = ICE_LISTENER_BUF_SIZE - 1;  /* - offset */

  assert (NULL != ap_server);
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
send_http_error (icer_server_t * ap_server, OMX_HANDLETYPE ap_hdl,
                 icer_listener_t * ap_lstnr, int a_error,
                 const char *ap_err_msg)
{
  ssize_t resp_size = 0;

  assert (NULL != ap_server);
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
handle_listener (icer_server_t * ap_server, OMX_HANDLETYPE ap_hdl,
                 icer_listener_t * ap_lstnr)
{
  int nparsed = 0;
  int nread = -1;
  bool some_error = true;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_server);
  assert (NULL != ap_hdl);
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_parser);

  if (ap_server->nclients > ap_server->max_clients)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Client limit reached [%d]", ap_server->max_clients);
      send_http_error (ap_server, ap_hdl, ap_lstnr, 400,
                       "Client limit reached");
      goto end;
    }

  nread =
    read_from_listener (ap_server, ap_hdl, ap_lstnr, ap_lstnr->buf.p_data,
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
       * closed the connection already */
      goto end;
    }

  if (nparsed != nread)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Bad http request");
      send_http_error (ap_server, ap_hdl, ap_lstnr, 400, "Bad request");
      goto end;
    }

  if (0 != strcmp ("GET", tiz_http_parser_get_method (ap_lstnr->p_parser)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Bad http method");
      send_http_error (ap_server, ap_hdl, ap_lstnr, 405, "Method not allowed");
      goto end;
    }

  if (0 != strcmp ("/", tiz_http_parser_get_url (ap_lstnr->p_parser)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Bad uri");
      send_http_error (ap_server, ap_hdl, ap_lstnr, 405, "Unathorized");
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
create_connection (OMX_HANDLETYPE ap_hdl, int connected_sockfd, char *ap_ip)
{
  icer_connection_t *p_con = NULL;

  assert (NULL != ap_hdl);

  if (NULL != (p_con = (icer_connection_t *)
               tiz_mem_calloc (1, sizeof (icer_connection_t))));
  {
    OMX_ERRORTYPE rc = OMX_ErrorNone;

    p_con->con_time = time (NULL);
    p_con->discon_time = 0;
    p_con->sent_total = 0;
    p_con->sent_bytes = 0;
    p_con->sock = connected_sockfd;
    p_con->error = false;
    p_con->full = false;
    p_con->p_ip = ap_ip;
    p_con->p_host = NULL;
    p_con->p_ev_io = NULL;

    if (OMX_ErrorNone
        != (rc = tiz_event_io_init (&(p_con->p_ev_io), ap_hdl,
                                    tiz_receive_event_io)))
      {
        TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                       "[%s] : Error initializing a client io event",
                       tiz_err_to_str (rc));
        tiz_mem_free (p_con);
        p_con = NULL;
      }
  }

  return p_con;
}

static OMX_S32
map_for_each_delete_listener (OMX_PTR ap_key, OMX_PTR ap_value, OMX_PTR ap_arg)
{
  icer_listener_t *p_lstnr = (icer_listener_t *) ap_value;
  assert (NULL != p_lstnr);
  destroy_listener (p_lstnr);
  return 0;
}

static bool
error_recoverable (int error)
{
  switch (error)
    {
    case 0:
    case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif
      {
        return true;
      }
    default:
      {
        return false;
      }
    };
}

static OMX_S32
write_to_clients (OMX_PTR ap_key, OMX_PTR ap_value, OMX_PTR ap_arg)
{
  icer_server_t *p_server = ap_arg;
  int sock = ICE_RENDERER_SOCK_ERROR;
  icer_listener_t *p_lstnr = NULL;
  OMX_U8 *p_buffer = NULL;
  size_t len = 0;
  int bytes = 0;

  assert (NULL != p_server);
  assert (NULL != ap_key);
  assert (NULL != ap_value);

  sock = *(int *) ap_key;
  p_lstnr = (icer_listener_t *) ap_value;

  assert (NULL != p_lstnr->p_con);

  assert (NULL != p_server->p_hdr);
  assert (NULL != p_server->p_hdr->pBuffer);

  p_buffer = p_server->p_hdr->pBuffer + p_server->p_hdr->nOffset
    + p_lstnr->pos;

  assert (p_server->p_hdr->nFilledLen > 0);
  assert (p_server->p_hdr->nAllocLen >=
          (p_server->p_hdr->nOffset + p_server->p_hdr->nFilledLen));
  assert (p_lstnr->pos <= p_server->p_hdr->nFilledLen);

  len = p_server->p_hdr->nFilledLen - p_lstnr->pos;
  p_lstnr->p_con->sent_bytes = 0;
  p_lstnr->p_con->full = false;

  bytes = send (sock, (const void *) p_buffer, len, 0);
  if (bytes < 0)
    {
      if (!error_recoverable (errno))
        {
          p_lstnr->p_con->error = true;
        }
    }
  else
    {
      p_lstnr->pos += bytes;
      p_lstnr->p_con->sent_bytes = bytes;
      p_lstnr->p_con->sent_total += bytes;
      if (bytes < len)
        {
          p_lstnr->p_con->full = true;
        }
    }

  /* always return success */
  return 0;
}


/*                      */
/* Non-static functions */
/*                      */

OMX_ERRORTYPE
icer_con_setup_server (icer_server_t ** app_server, OMX_HANDLETYPE ap_hdl,
                       OMX_STRING a_address, OMX_U32 a_port,
                       OMX_U32 a_max_clients,
                       icer_buffer_emptied_f a_pf_emptied,
                       icer_buffer_needed_f a_pf_needed, OMX_PTR ap_arg)
{
  icer_server_t *p_server = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int sockfd = ICE_RENDERER_SOCK_ERROR;;
  bool some_error = true;

  assert (NULL != app_server);
  assert (NULL != ap_hdl);
  assert (NULL != a_pf_emptied);
  assert (NULL != a_pf_needed);

  if (NULL == (p_server = (icer_server_t *)
               tiz_mem_calloc (1, sizeof (p_server))))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to allocate "
                     "the icer_server_t struct.");
      return OMX_ErrorInsufficientResources;
    }

  p_server->max_clients = a_max_clients;
  p_server->nclients = 0;
  p_server->p_hdr = NULL;
  p_server->pf_emptied = a_pf_emptied;
  p_server->pf_needed = a_pf_needed;
  p_server->p_arg = ap_arg;

  if (NULL != a_address)
    {
      p_server->p_ip = strndup (a_address, ICE_RENDERER_MAX_ADDR_LEN);
    }

  if (OMX_ErrorNone != (rc = tiz_map_init (&(p_server->p_lstnrs),
                                           listeners_map_compare_func,
                                           listeners_map_free_func, NULL)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : Error initializing the listeners map",
                     tiz_err_to_str (rc));
      goto end;
    }

  if (ICE_RENDERER_SOCK_ERROR ==
      (p_server->lstn_sockfd =
       create_server_socket (ap_hdl, a_port, a_address)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Unable to create the server socket.");
      goto end;
    }

  if (OMX_ErrorNone != (rc = allocate_server_io_watcher (p_server, ap_hdl)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] : Unable to allocate the server io event.",
                     tiz_err_to_str (rc));
      goto end;
    }

  some_error = false;

end:

  if (some_error)
    {
      if (ICE_RENDERER_SOCK_ERROR != p_server->lstn_sockfd)
        {
          close (sockfd);
          p_server->lstn_sockfd = ICE_RENDERER_SOCK_ERROR;
        }
      if (NULL != p_server)
        {
          tiz_mem_free (p_server);
          tiz_mem_free (p_server->p_ip);
          p_server = NULL;
        }
      rc = OMX_ErrorInsufficientResources;
    }
  else
    {
      *app_server = p_server;
    }

  return rc;
}

void
icer_con_teardown_server (icer_server_t * ap_server)
{
  assert (NULL != ap_server);

  if (!tiz_map_empty (ap_server->p_lstnrs))
    {
      (void) tiz_map_for_each (ap_server->p_lstnrs,
                               map_for_each_delete_listener, NULL);
    }

  if (-1 != ap_server->lstn_sockfd)
    {
      close (ap_server->lstn_sockfd);
      ap_server->lstn_sockfd = ICE_RENDERER_SOCK_ERROR;;
    }

  tiz_map_clear (ap_server->p_lstnrs);
  tiz_map_destroy (ap_server->p_lstnrs);
  ap_server->p_lstnrs = NULL;

  destroy_server_io_watcher (ap_server);
}

int
icer_con_get_server_fd (const icer_server_t * ap_server)
{
  assert (NULL != ap_server);
  return ap_server->lstn_sockfd;
}

OMX_ERRORTYPE
icer_con_start_listening (icer_server_t * ap_server, OMX_HANDLETYPE ap_hdl)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (NULL != ap_server);
  assert (NULL != ap_hdl);

  if (listen (ap_server->lstn_sockfd, ICE_RENDERER_LISTEN_QUEUE) ==
      ICE_RENDERER_SOCK_ERROR)
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to mark "
                     "socket as passive (%s).", strerror (errno));
      return OMX_ErrorInsufficientResources;
    }

  if (OMX_ErrorNone != (rc = set_non_blocking (ap_server->lstn_sockfd)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[OMX_ErrorInsufficientResources] : Unable to set"
                     "socket as non-blocking.");
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
icer_con_accept_connection (icer_server_t * ap_server, OMX_HANDLETYPE ap_hdl)
{
  char *p_ip = NULL;
  icer_connection_t *p_con = NULL;
  icer_listener_t *p_lstnr = NULL;
  int conn_sock = ICE_RENDERER_SOCK_ERROR;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  bool some_error = true;

  assert (NULL != ap_server);
  assert (NULL != ap_hdl);

  if (NULL != (p_ip = (char *) tiz_mem_alloc (ICE_RENDERER_MAX_ADDR_LEN)))
    {
      conn_sock =
        accept_socket (ap_server, ap_hdl, p_ip, ICE_RENDERER_MAX_ADDR_LEN);

      if (ICE_RENDERER_SOCK_ERROR == conn_sock)
        {
          TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                         TIZ_CBUF (ap_hdl),
                         "Error found while accepting the connection");
          goto end;
        }

      if (NULL == (p_con = create_connection (ap_hdl, conn_sock, p_ip)))
        {
          TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                         TIZ_CBUF (ap_hdl),
                         "Error found while creating the connection");
          goto end;
        }

      if (OMX_ErrorNone != (rc = create_listener (ap_server, ap_hdl, &p_lstnr,
                                                  p_con)))
        {
          goto end;
        }

      if (OMX_ErrorNone != handle_listener (ap_server, ap_hdl, p_lstnr))
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

      if (OMX_ErrorNone == rc)
        {
          rc = OMX_ErrorInsufficientResources;
        }
    }
  else
    {
      OMX_U32 index;
      /* TODO: Check return code */
      tiz_map_insert (ap_server->p_lstnrs, &conn_sock, p_lstnr, &index);
    }

  return rc;
}

OMX_ERRORTYPE
icer_con_start_server_io_watcher (icer_server_t * ap_server,
                                  OMX_HANDLETYPE ap_hdl)
{
  assert (NULL != ap_server);
  assert (NULL != ap_hdl);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Starting server io watcher on socket fd [%d] ",
                 ap_server->lstn_sockfd);

  return tiz_event_io_start (ap_server->p_srv_ev_io);
}

OMX_ERRORTYPE
icer_con_stop_server_io_watcher (icer_server_t * ap_server,
                                 OMX_HANDLETYPE ap_hdl)
{
  assert (NULL != ap_server);
  assert (NULL != ap_hdl);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl),
                 TIZ_CBUF (ap_hdl),
                 "stopping io watcher on fd [%d] ", ap_server->lstn_sockfd);

  return tiz_event_io_stop (ap_server->p_srv_ev_io);
}

static OMX_ERRORTYPE
start_listener_io_watcher (icer_listener_t * ap_lstnr, OMX_HANDLETYPE ap_hdl)
{
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_con);
  assert (NULL != ap_hdl);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Starting listener io watcher on  fd [%d] ",
                 ap_lstnr->p_con->sock);

  return tiz_event_io_start (ap_lstnr->p_con->p_ev_io);
}

static OMX_ERRORTYPE
stop_listener_io_watcher (icer_listener_t * ap_lstnr, OMX_HANDLETYPE ap_hdl)
{
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_con);
  assert (NULL != ap_hdl);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "stopping listener io watcher on fd [%d] ",
                 ap_lstnr->p_con->sock);

  return tiz_event_io_stop (ap_lstnr->p_con->p_ev_io);
}

OMX_ERRORTYPE
icer_con_write_data (icer_server_t * ap_server, OMX_HANDLETYPE ap_hdl)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  icer_listener_t *p_lstnr = NULL;

  assert (NULL != ap_server);
  assert (NULL != ap_hdl);

  if (tiz_map_empty (ap_server->p_lstnrs))
    {
      /* no clients connected yet */
      return OMX_ErrorNotReady;
    }

  /* Do this hack for now until support for multiple listeners is
     implemented */
  p_lstnr = tiz_map_at (ap_server->p_lstnrs, 0);
  stop_listener_io_watcher (p_lstnr, ap_hdl);

  while (1)
    {

      if (NULL == p_hdr)
        {
          if (NULL == (p_hdr = ap_server->pf_needed (ap_server->p_arg)))
            {
              /* no data available at the moment */
              return OMX_ErrorNone;
            }

          ap_server->p_hdr = p_hdr;
        }

      /* TODO: check return code */
      tiz_map_for_each (ap_server->p_lstnrs, write_to_clients, ap_server);

      if (p_lstnr->p_con->error)
        {
          /* TODO: close the connection and destroy the listener */
          return OMX_ErrorUndefined;
        }

      if (p_lstnr->p_con->full)
        {
          /* Socket buffer is full. Start the io watcher to get notified when
           * some room is made available */
          return start_listener_io_watcher (p_lstnr, ap_hdl);
        }

      if (0 == p_lstnr->p_con->sent_bytes)
        {
          /* TODO: close the connection and destroy the listener */
          return OMX_ErrorUndefined;
        }

      if (p_lstnr->pos == ap_server->p_hdr->nFilledLen)
        {
          /* Buffer emptied */
          p_lstnr->pos = 0;
          ap_server->p_hdr->nFilledLen = 0;
          ap_server->pf_emptied (p_hdr, ap_server->p_arg);
          p_hdr = NULL;
        }
    };

  return OMX_ErrorNone;
}

/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   httprsrv.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia - HTTP renderer's networking functions
 *
 * NOTE: This is work in progress!!!!
 *
 * TODO: Better flow control
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
#include <sys/ioctl.h>

#include <tizplatform.h>
#include <tizutils.h>

#include <OMX_TizoniaExt.h>

#include "httpr.h"
#include "httprsrv.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.prc.net"
#endif

#ifdef INET6_ADDRSTRLEN
#define ICE_RENDERER_MAX_ADDR_LEN INET6_ADDRSTRLEN
#else
#define ICE_RENDERER_MAX_ADDR_LEN 46
#endif

typedef struct httpr_connection httpr_connection_t;
typedef struct httpr_listener httpr_listener_t;
typedef struct httpr_listener_buffer httpr_listener_buffer_t;
typedef struct httpr_mount httpr_mount_t;

struct httpr_listener_buffer
{
  unsigned int len;
  unsigned int metadata_offset;
  unsigned int metadata_bytes;
  char * p_data;
};

struct httpr_mount
{
  OMX_U8 mount_name[OMX_MAX_STRINGNAME_SIZE];
  OMX_U8 station_name[OMX_MAX_STRINGNAME_SIZE];
  OMX_U8 station_description[OMX_MAX_STRINGNAME_SIZE];
  OMX_U8 station_genre[OMX_MAX_STRINGNAME_SIZE];
  OMX_U8 station_url[OMX_MAX_STRINGNAME_SIZE];
  OMX_U32 metadata_period;
  OMX_U8 stream_title[OMX_MAX_STRINGNAME_SIZE];
  OMX_U32 initial_burst_size;
  OMX_U32 max_clients;
};

struct httpr_connection
{
  httpr_listener_t * p_lstnr;
  time_t con_time;
  uint64_t sent_total;
  unsigned int sent_last;
  unsigned int burst_bytes;
  OMX_S32 initial_burst_bytes;
  bool metadata_delivered;
  int sockfd;
  char * p_host;
  char * p_ip;
  unsigned short port;
  tiz_event_io_t * p_ev_io;
  tiz_event_timer_t * p_ev_timer;
};

struct httpr_listener
{
  httpr_server_t * p_server;
  httpr_connection_t * p_con;
  int respcode;
  long intro_offset;
  unsigned long pos;
  httpr_listener_buffer_t buf;
  tiz_http_parser_t * p_parser;
  bool need_response;
  bool timer_started;
  bool want_metadata;
};

struct httpr_server
{
  void * p_parent;
  int lstn_sockfd;
  char * p_ip;
  tiz_event_io_t * p_srv_ev_io;
  OMX_U32 max_clients; /* In future, more than one will be allowed;
                          only one allowed at the moment. */
  tiz_map_t * p_lstnrs;
  OMX_BUFFERHEADERTYPE * p_hdr;
  httpr_srv_release_buffer_f pf_release_buf;
  httpr_srv_acquire_buffer_f pf_acquire_buf;
  bool need_more_data;
  bool running;
  OMX_PTR p_arg;
  OMX_U32 bitrate;
  OMX_U32 num_channels;
  OMX_U32 sample_rate;
  OMX_U32 bytes_per_frame;
  OMX_U32 burst_size;
  double wait_time;
  double pkts_per_sec;
  httpr_mount_t mountpoint;
};

static void
srv_destroy_listener (httpr_listener_t * ap_lstnr);

static OMX_S32
listeners_map_compare_func (OMX_PTR ap_key1, OMX_PTR ap_key2)
{
  int * p_sockfd1 = (int *) ap_key1;
  int * p_sockfd2 = (int *) ap_key2;
  assert (ap_key1);
  assert (ap_key2);
  return (*p_sockfd1 == *p_sockfd2) ? 0 : ((*p_sockfd1 < *p_sockfd2) ? -1 : 1);
}

static void
listeners_map_free_func (OMX_PTR ap_key, OMX_PTR ap_value)
{
  httpr_listener_t * p_lstnr = (httpr_listener_t *) ap_value;
  assert (p_lstnr);
  srv_destroy_listener (p_lstnr);
}

static bool
srv_is_recoverable_error (httpr_server_t * ap_server, int sockfd, int error)
{
  bool rc = false;
  assert (ap_server);
  TIZ_PRINTF_DBG_RED ("Socket [%d] - error [%s]", sockfd, strerror (error));

  switch (error)
    {
      case 0:
      case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
      case EWOULDBLOCK:
#endif
        {
          rc = true;
        }
        break;
      default:
        {
          rc = false;
        }
        break;
    };
  return rc;
}

static inline bool
srv_is_valid_socket (int a_sockfd)
{
  int optval;
  socklen_t optlen = sizeof (int);
  return (
    0 == getsockopt (a_sockfd, SOL_SOCKET, SO_TYPE, (void *) &optval, &optlen));
}

static inline int
srv_get_listeners_count (const httpr_server_t * ap_server)
{
  int rc = 0;
  if (ap_server && ap_server->p_lstnrs)
    {
      rc = tiz_map_size (ap_server->p_lstnrs);
    }
  return rc;
}

static httpr_listener_t *
srv_get_first_listener (const httpr_server_t * ap_server)
{
  httpr_listener_t * p_lstnr = NULL;
  if (srv_get_listeners_count (ap_server) > 0)
    {
      p_lstnr = tiz_map_value_at (ap_server->p_lstnrs, 0);
    }
  return p_lstnr;
}

static int
srv_set_non_blocking (const int sockfd)
{
  int rc = ICE_SOCK_ERROR;
  int flags;
  errno = 0;
  if ((flags = fcntl (sockfd, F_GETFL, 0)) != ICE_SOCK_ERROR)
    {
      errno = 0;
      flags |= O_NONBLOCK;
      if (fcntl (sockfd, F_SETFL, flags) != ICE_SOCK_ERROR)
        {
          rc = 0;
        }
    }
  return rc;
}

static inline int
srv_set_nolinger (const int sock)
{
  struct linger lin = {0, 0};
  errno = 0;
  /* linger inactive. close call will return immediately to the caller, and any
   * pending data will be delivered if possible. */
  return setsockopt (sock, SOL_SOCKET, SO_LINGER, (void *) &lin,
                     sizeof (struct linger));
}

static inline int
srv_set_nodelay (const int sock)
{
  int nodelay = 1;
  errno = 0;
  return setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (void *) &nodelay,
                     sizeof (int));
}

static inline int
srv_set_keepalive (const int sock)
{
  int keepalive = 1;
  errno = 0;
  return setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &keepalive,
                     sizeof (int));
}

static int
srv_accept_socket (httpr_server_t * ap_server, char * ap_ip,
                   const size_t a_ip_len, unsigned short * ap_port)
{
#define bail_on_accept_error(some_error, msg) \
  do                                          \
    {                                         \
      if (some_error)                         \
        {                                     \
          TIZ_ERROR (p_hdl, "[%s]", msg);     \
          goto end;                           \
        }                                     \
    }                                         \
  while (0)

  int accepted_sockfd = ICE_SOCK_ERROR;
  struct sockaddr_storage sa;
  socklen_t slen = sizeof (sa);
  OMX_HANDLETYPE p_hdl = NULL;
  bool some_error = true;
  int err = 0;

  assert (ap_server);
  assert (ap_ip);
  assert (ap_port);
  p_hdl = handleOf (ap_server->p_parent);

  some_error = (!srv_is_valid_socket (ap_server->lstn_sockfd));
  bail_on_accept_error (some_error, "Invalid server socket");

  errno = 0;
  accepted_sockfd
    = accept (ap_server->lstn_sockfd, (struct sockaddr *) &sa, &slen);
  some_error = (ICE_SOCK_ERROR == accepted_sockfd);
  bail_on_accept_error (some_error, strerror (errno));

  if (0 != (err = getnameinfo ((struct sockaddr *) &sa, slen, ap_ip, a_ip_len,
                               NULL, 0, NI_NUMERICHOST)))
    {
      snprintf (ap_ip, a_ip_len, "unknown");
      TIZ_ERROR (p_hdl, "getnameinfo error [%s]", gai_strerror (err));
    }
  else
    {
      if (sa.ss_family == AF_INET)
        {
          struct sockaddr_in * p_sa_in = (struct sockaddr_in *) &sa;
          *ap_port = ntohs (p_sa_in->sin_port);
        }
    }

  err = srv_set_nolinger (accepted_sockfd);
  some_error = (ICE_SOCK_ERROR == err);
  bail_on_accept_error (some_error, strerror (errno));

  err = srv_set_keepalive (accepted_sockfd);
  some_error = (ICE_SOCK_ERROR == err);
  bail_on_accept_error (some_error, strerror (errno));

  TIZ_TRACE (p_hdl, "Accepted [%s:%u] fd [%d]", ap_ip, *ap_port,
             accepted_sockfd);
end:
  if (some_error)
    {
      close (accepted_sockfd);
      accepted_sockfd = ICE_SOCK_ERROR;
    }

  return accepted_sockfd;
}

static inline int
srv_create_server_socket (httpr_server_t * ap_server, const int a_port,
                          const char * a_interface)
{
  struct sockaddr_storage sa;
  struct addrinfo hints;
  struct addrinfo * res;
  struct addrinfo * ai;
  char service[10];
  int sockfd = ICE_SOCK_ERROR;
  int getaddrc = ICE_SOCK_ERROR;

  assert (ap_server);
  assert (a_port >= 0);

  tiz_mem_set (&sa, 0, sizeof (sa));
  tiz_mem_set (&hints, 0, sizeof (hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV | AI_NUMERICHOST;
  hints.ai_socktype = SOCK_STREAM;
  snprintf (service, sizeof (service), "%d", a_port);

  if ((getaddrc = getaddrinfo (a_interface, service, &hints, &res)) != 0)
    {
      TIZ_ERROR (handleOf (ap_server->p_parent), "[ICE_SOCK_ERROR] : %s.",
                 gai_strerror (getaddrc));
    }
  else
    {
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
    }
  return ICE_SOCK_ERROR;
}

static void
srv_destroy_server_io_watcher (httpr_server_t * ap_server)
{
  assert (ap_server);
  tiz_srv_io_watcher_destroy (ap_server->p_parent, ap_server->p_srv_ev_io);
  ap_server->p_srv_ev_io = NULL;
}

static OMX_ERRORTYPE
srv_allocate_server_io_watcher (httpr_server_t * ap_server)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_server);

  rc = tiz_srv_io_watcher_init (
    ap_server->p_parent, &(ap_server->p_srv_ev_io), ap_server->lstn_sockfd,
    TIZ_EVENT_READ, /* Interested in read events only */
    true            /* Only one event at a time */
    );
  if (OMX_ErrorNone != rc)
    {
      srv_destroy_server_io_watcher (ap_server);
    }

  return rc;
}

static OMX_ERRORTYPE
srv_start_server_io_watcher (httpr_server_t * ap_server)
{
  assert (ap_server);
  TIZ_PRINTF_DBG_RED (
    "Starting server io watcher "
    "on fd [%d]",
    ap_server->lstn_sockfd);
  return tiz_srv_io_watcher_start (ap_server->p_parent, ap_server->p_srv_ev_io);
}

static inline OMX_ERRORTYPE
srv_stop_server_io_watcher (httpr_server_t * ap_server)
{
  assert (ap_server);
  TIZ_PRINTF_DBG_RED (
    "Stopping server io watcher "
    "on fd [%d]",
    ap_server->lstn_sockfd);
  return tiz_srv_io_watcher_stop (ap_server->p_parent, ap_server->p_srv_ev_io);
}

static OMX_ERRORTYPE
srv_start_listener_io_watcher (httpr_listener_t * ap_lstnr)
{
  assert (ap_lstnr);
  assert (ap_lstnr->p_server);
  assert (ap_lstnr->p_con);
  return tiz_srv_io_watcher_start (ap_lstnr->p_server->p_parent,
                                   ap_lstnr->p_con->p_ev_io);
}

static void
srv_stop_listener_io_watcher (httpr_listener_t * ap_lstnr)
{
  assert (ap_lstnr);
  assert (ap_lstnr->p_con);
  (void) tiz_srv_io_watcher_stop (ap_lstnr->p_server->p_parent,
                                  ap_lstnr->p_con->p_ev_io);
}

static OMX_ERRORTYPE
srv_start_listener_timer_watcher (httpr_listener_t * ap_lstnr,
                                  const double a_wait_time)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_lstnr);
  if (!ap_lstnr->timer_started)
    {
      assert (ap_lstnr->p_server);
      assert (ap_lstnr->p_con);
      tiz_check_omx (tiz_srv_timer_watcher_start (
        ap_lstnr->p_server->p_parent, ap_lstnr->p_con->p_ev_timer, a_wait_time,
        a_wait_time));
      ap_lstnr->timer_started = true;
    }
  return rc;
}

static void
srv_stop_listener_timer_watcher (httpr_listener_t * ap_lstnr)
{
  assert (ap_lstnr);
  if (ap_lstnr->timer_started)
    {
      assert (ap_lstnr->p_server);
      assert (ap_lstnr->p_con);
      (void) tiz_srv_timer_watcher_stop (ap_lstnr->p_server->p_parent,
                                         ap_lstnr->p_con->p_ev_timer);
      ap_lstnr->timer_started = false;
    }
}

static void
srv_destroy_connection (httpr_connection_t * ap_con)
{
  if (ap_con)
    {
      if (ICE_SOCK_ERROR != ap_con->sockfd)
        {
          close (ap_con->sockfd);
        }
      tiz_mem_free (ap_con->p_ip);
      tiz_mem_free (ap_con->p_host);
      assert (ap_con->p_lstnr && ap_con->p_lstnr->p_server);
      tiz_srv_io_watcher_destroy (ap_con->p_lstnr->p_server->p_parent,
                                  ap_con->p_ev_io);
      tiz_srv_timer_watcher_destroy (ap_con->p_lstnr->p_server->p_parent,
                                     ap_con->p_ev_timer);
      tiz_mem_free (ap_con);
    }
}

static void
srv_destroy_listener (httpr_listener_t * ap_lstnr)
{
  if (ap_lstnr)
    {
      srv_stop_listener_timer_watcher (ap_lstnr);
      if (ap_lstnr->p_parser)
        {
          tiz_http_parser_destroy (ap_lstnr->p_parser);
        }
      tiz_mem_free (ap_lstnr->buf.p_data);
      srv_destroy_connection (ap_lstnr->p_con);
      tiz_mem_free (ap_lstnr);
    }
}

static void
srv_remove_listener (httpr_server_t * ap_server, httpr_listener_t * ap_lstnr)
{
  int nlstnrs = 0;

  assert (ap_server);
  assert (ap_lstnr);

  nlstnrs = srv_get_listeners_count (ap_server);
  assert (nlstnrs > 0);

  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "Destroyed listener [%s] - [%d] listeners remaining",
           ap_lstnr->p_con->p_ip, nlstnrs - 1);

  tiz_map_erase (ap_server->p_lstnrs, &ap_lstnr->p_con->sockfd);
  assert (nlstnrs - 1 == srv_get_listeners_count (ap_server));

  /* NOTE: No need to call srv_destroy_listener as this has been called already
   * by
   * the map's listeners_map_free_func */
}

static httpr_connection_t *
srv_create_connection (httpr_server_t * ap_server, httpr_listener_t * ap_lstnr,
                       const int connected_sockfd, char * ap_ip,
                       const unsigned short ap_port, const double a_wait_time)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  httpr_connection_t * p_con = NULL;
  OMX_HANDLETYPE p_hdl = NULL;

  assert (ap_server);
  assert (ap_lstnr);
  p_hdl = handleOf (ap_server->p_parent);

  p_con
    = (httpr_connection_t *) tiz_mem_calloc (1, sizeof (httpr_connection_t));
  rc = p_con ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, p_hdl, "Unable to alloc the connection struct");

  p_con->p_lstnr = ap_lstnr;
  p_con->con_time = 0; /* time (NULL); */
  p_con->sent_total = 0;
  p_con->sent_last = 0;
  p_con->burst_bytes = 0;
  p_con->initial_burst_bytes = ap_server->mountpoint.initial_burst_size;
  p_con->sockfd = connected_sockfd;
  p_con->p_host = NULL;
  p_con->p_ip = ap_ip;
  p_con->port = ap_port;
  p_con->p_ev_io = NULL;
  p_con->p_ev_timer = NULL;

  /* We are interested in knowing when a listener socket is available for
   * writing */
  rc = tiz_srv_io_watcher_init (ap_server->p_parent, &(p_con->p_ev_io),
                                p_con->sockfd, TIZ_EVENT_WRITE, true);
  goto_end_on_omx_error (rc, p_hdl, "Unable to init the client's io event");

  rc = tiz_srv_timer_watcher_init (ap_server->p_parent, &(p_con->p_ev_timer));
  goto_end_on_omx_error (rc, p_hdl, "Unable to init the client's timer event");

end:
  if (OMX_ErrorNone != rc)
    {
      srv_destroy_connection (p_con);
      p_con = NULL;
    }

  return p_con;
}

static OMX_ERRORTYPE
srv_create_listener (httpr_server_t * ap_server, httpr_listener_t ** app_lstnr,
                     const int a_connected_sockfd, char * ap_ip,
                     const unsigned short ap_port)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  httpr_listener_t * p_lstnr = NULL;
  httpr_connection_t * p_con = NULL;
  OMX_HANDLETYPE p_hdl = NULL;
  int sockrc = ICE_SOCK_ERROR;

  assert (ap_server);
  assert (app_lstnr);
  assert (ICE_SOCK_ERROR != a_connected_sockfd);
  assert (ap_ip);
  p_hdl = handleOf (ap_server->p_parent);

  p_lstnr = (httpr_listener_t *) tiz_mem_calloc (1, sizeof (httpr_listener_t));
  rc = p_lstnr ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, p_hdl, "Unable to alloc the listener structure");

  p_con = srv_create_connection (ap_server, p_lstnr, a_connected_sockfd, ap_ip,
                                 ap_port, ap_server->wait_time);
  rc = p_con ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, p_hdl, "Unable to init the listener's connection");

  p_lstnr->p_server = ap_server;
  p_lstnr->p_con = p_con;
  p_lstnr->respcode = 200;
  p_lstnr->intro_offset = 0;
  p_lstnr->pos = 0;
  p_lstnr->buf.len = ICE_LISTENER_BUF_SIZE;
  p_lstnr->buf.metadata_offset = 0;
  p_lstnr->buf.metadata_bytes = 0;
  p_lstnr->p_parser = NULL;
  p_lstnr->need_response = true;
  p_lstnr->timer_started = false;
  p_lstnr->want_metadata = false;

  p_lstnr->buf.p_data = (char *) tiz_mem_alloc (ICE_LISTENER_BUF_SIZE);
  rc = p_lstnr->buf.p_data ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, p_hdl, "Unable to alloc the listener's buffer");
  p_lstnr->buf.p_data[ICE_LISTENER_BUF_SIZE - 1] = '\0';

  rc = tiz_http_parser_init (&(p_lstnr->p_parser), ETIZHttpParserTypeRequest);
  goto_end_on_omx_error (rc, p_hdl, "Unable to init the http parser");

  sockrc = srv_set_non_blocking (p_lstnr->p_con->sockfd);
  rc = sockrc < 0 ? OMX_ErrorInsufficientResources : OMX_ErrorNone;
  goto_end_on_socket_error (sockrc, p_hdl, strerror (errno));

  sockrc = srv_set_nodelay (p_lstnr->p_con->sockfd);
  rc = sockrc < 0 ? OMX_ErrorInsufficientResources : OMX_ErrorNone;
  goto_end_on_socket_error (sockrc, p_hdl, strerror (errno));

  rc = OMX_ErrorNone;

end:

  if (OMX_ErrorNone != rc)
    {
      srv_destroy_listener (p_lstnr);
      p_lstnr = NULL;
    }

  *app_lstnr = p_lstnr;

  return rc;
}

static int
srv_read_from_listener (httpr_listener_t * ap_lstnr)
{
  httpr_connection_t * p_con = NULL;
  httpr_listener_buffer_t * p_buf = NULL;

  assert (ap_lstnr);
  assert (ap_lstnr->p_con);

  p_con = ap_lstnr->p_con;
  p_buf = &ap_lstnr->buf;
  assert (p_buf->p_data);
  assert (p_buf->len > 0);

  errno = 0;
  return recv (p_con->sockfd, p_buf->p_data, p_buf->len, 0);
}

static ssize_t
srv_build_http_negative_response (char * ap_buf, size_t len, int status,
                                  const char * ap_msg)
{
  const char * http_version = "1.0";
  time_t now;
  struct tm result;
  struct tm * gmtime_result;
  char currenttime_buffer[80];
  char status_buffer[80];
  char contenttype_buffer[80];
  ssize_t ret;
  const char * statusmsg = NULL;
  const char * contenttype = "text/html";

  assert (ap_buf);

  switch (status)
    {
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
      case 500:
        {
          statusmsg = "Internal Server Error";
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

  ret = snprintf (ap_buf, len, "%sServer: %s%s\r\n%s%s%s%s", status_buffer,
                  "Tizonia HTTP Server ", PACKAGE_VERSION, currenttime_buffer,
                  contenttype_buffer, (ap_msg ? "\r\n" : ""),
                  (ap_msg ? ap_msg : ""));

  return ret;
}

static void
srv_send_http_error (httpr_server_t * ap_server, httpr_listener_t * ap_lstnr,
                     int a_error, const char * ap_err_msg)
{
  ssize_t resp_size = 0;

  assert (ap_server);
  assert (ap_lstnr);
  assert (ap_lstnr->buf.p_data);
  assert (ap_lstnr->p_con);
  assert (ap_err_msg);

  ap_lstnr->buf.p_data[ICE_LISTENER_BUF_SIZE - 1] = '\0';
  resp_size = srv_build_http_negative_response (
    ap_lstnr->buf.p_data, ICE_LISTENER_BUF_SIZE - 1, a_error, "");

  snprintf (ap_lstnr->buf.p_data + resp_size, ICE_LISTENER_BUF_SIZE - resp_size,
            "<html><head><title>Error %i</title></head><body><b>%i - %s</b>"
            "</body></html>\r\n",
            a_error, a_error, ap_err_msg);

  ap_lstnr->buf.len = strnlen (ap_lstnr->buf.p_data, ICE_LISTENER_BUF_SIZE);

  /* Ignore send error */
  (void) send (ap_lstnr->p_con->sockfd, ap_lstnr->buf.p_data, ap_lstnr->buf.len, 0);
  ap_lstnr->buf.len = 0;
}

static ssize_t
srv_build_http_positive_response (httpr_server_t * ap_server, char * ap_buf,
                                  size_t len, OMX_U32 a_bitrate,
                                  OMX_U32 a_num_channels, OMX_U32 a_sample_rate,
                                  bool a_want_metadata)
{
  const char * http_version = "1.0";
  char status_buffer[80];
  char contenttype_buffer[80];
  char icybr_buffer[80];
  char icyname_buffer[OMX_MAX_STRINGNAME_SIZE * 2];
  char icydescription_buffer[OMX_MAX_STRINGNAME_SIZE * 2];
  char icygenre_buffer[OMX_MAX_STRINGNAME_SIZE * 2];
  char icyurl_buffer[OMX_MAX_STRINGNAME_SIZE * 2];
  char icypub_buffer[80];
  char iceaudioinfo_buffer[80];
  char icymetaint_buffer[80];
  ssize_t ret;
  const char * statusmsg = "OK";
  const char * contenttype = "audio/mpeg";
  int status = 200;
  int pub = 0;
  bool metadata_needed = false;

  assert (ap_server);
  assert (ap_buf);

  /* HTTP status line */
  snprintf (status_buffer, sizeof (status_buffer), "HTTP/%s %d %s\r\n",
            http_version, status, statusmsg);

  /* HTTP Content-Type header */
  snprintf (contenttype_buffer, sizeof (contenttype_buffer),
            "Content-Type: %s\r\n", contenttype);

  /* icy-br header */
  snprintf (icybr_buffer, sizeof (icybr_buffer), "icy-br:%d\r\n",
            (int) a_bitrate / 1000);

  /* ice-audio-info header */
  snprintf (iceaudioinfo_buffer, sizeof (iceaudioinfo_buffer),
            "ice-audio-info: "
            "bitrate=%d;channels=%d;samplerate=%d\r\n",
            (int) a_bitrate, (int) a_num_channels, (int) a_sample_rate);

  /* icy-name header */
  snprintf (icyname_buffer, sizeof (icyname_buffer), "icy-name:%s\r\n",
            ap_server->mountpoint.station_name);

  /* icy-decription header */
  snprintf (icydescription_buffer, sizeof (icydescription_buffer),
            "icy-description:%s\r\n",
            ap_server->mountpoint.station_description);

  /* icy-genre header */
  snprintf (icygenre_buffer, sizeof (icygenre_buffer), "icy-genre:%s\r\n",
            ap_server->mountpoint.station_genre);

  /* icy-url header */
  snprintf (icyurl_buffer, sizeof (icyurl_buffer), "icy-url:%s\r\n",
            ap_server->mountpoint.station_url);

  /* icy-pub header */
  snprintf (icypub_buffer, sizeof (icypub_buffer), "icy-pub:%u\r\n", pub);

  if (ap_server->mountpoint.metadata_period > 0 && a_want_metadata)
    {
      metadata_needed = true;
      /* icy-metaint header */
      snprintf (icymetaint_buffer, sizeof (icymetaint_buffer),
                "icy-metaint:%lu\r\n", ap_server->mountpoint.metadata_period);
    }

  ret = snprintf (
    ap_buf, len, "%s%s%s%s%s%s%s%s%s%s%s%s%s\r\n", status_buffer,
    contenttype_buffer, icybr_buffer, iceaudioinfo_buffer, icyname_buffer,
    icydescription_buffer, icygenre_buffer, icyurl_buffer, icypub_buffer,
    (metadata_needed ? icymetaint_buffer : ""),
    "Server: Tizonia HTTP Renderer ", PACKAGE_VERSION, "\r\nCache-Control: no-cache\r\n");

  return ret;
}

static int
srv_send_http_response (httpr_server_t * ap_server, httpr_listener_t * ap_lstnr)
{
  ssize_t sent_bytes = 0;

  assert (ap_server);
  assert (ap_lstnr);
  assert (ap_lstnr->buf.p_data);
  assert (ap_lstnr->p_con);

  ap_lstnr->buf.len = strnlen (ap_lstnr->buf.p_data, ICE_LISTENER_BUF_SIZE);

  sent_bytes = send (ap_lstnr->p_con->sockfd, ap_lstnr->buf.p_data,
                     ap_lstnr->buf.len, MSG_NOSIGNAL);
  ap_lstnr->buf.len = 0;
  return sent_bytes;
}

static OMX_ERRORTYPE
srv_handle_listeners_request (httpr_server_t * ap_server,
                              httpr_listener_t * ap_lstnr)
{
#define bail_on_request_error(some_error, httperr, msg)                \
  do                                                                   \
    {                                                                  \
      if (some_error)                                                  \
        {                                                              \
          TIZ_ERROR (handleOf (ap_server->p_parent), "[%s]", msg);     \
          if (httperr > 0)                                             \
            {                                                          \
              srv_send_http_error (ap_server, ap_lstnr, httperr, msg); \
            }                                                          \
          goto end;                                                    \
        }                                                              \
    }                                                                  \
  while (0)

  int nparsed = 0;
  int nread = -1;
  bool some_error = true;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int to_write = -1;
  const char * parsed_string = NULL;

  assert (ap_server);
  assert (ap_lstnr);
  assert (ap_lstnr->p_con);
  assert (ap_lstnr->p_parser);

  some_error = (srv_get_listeners_count (ap_server) > ap_server->max_clients);
  bail_on_request_error (some_error, 400, "Client limit reached");

  /*   some_error */
  /*       = (ap_lstnr->p_con->con_time + ICE_DEFAULT_HEADER_TIMEOUT <= time
   * (NULL)); */
  /*   bail_on_request_error (some_error, -1, "Connection timed out"); */

  some_error = ((nread = srv_read_from_listener (ap_lstnr)) <= 0);
  rc
    = (some_error
         ? (srv_is_recoverable_error (ap_server, ap_lstnr->p_con->sockfd, errno)
              ? OMX_ErrorNotReady
              : OMX_ErrorNone)
         : OMX_ErrorNone);
  bail_on_request_error (some_error, -1, strerror (errno));

  nparsed
    = tiz_http_parser_parse (ap_lstnr->p_parser, ap_lstnr->buf.p_data, nread);
  some_error = (nparsed != nread);
  bail_on_request_error (some_error, 400, "Bad request");

  some_error
    = (NULL == (parsed_string = tiz_http_parser_get_method (ap_lstnr->p_parser))
       || (0 != strncmp ("GET", parsed_string, strlen ("GET"))));
  bail_on_request_error (some_error, 405, "Method not allowed");

  some_error
    = (NULL == (parsed_string = tiz_http_parser_get_url (ap_lstnr->p_parser))
       || (0 != strncmp ("/", parsed_string, strlen ("/"))));
  bail_on_request_error (some_error, 401, "Unathorized");

  if ((parsed_string
       = tiz_http_parser_get_header (ap_lstnr->p_parser, "Icy-MetaData"))
      && (0 == strncmp ("1", parsed_string, strlen ("1"))))
    {
      TIZ_TRACE (handleOf (ap_server->p_parent), "ICY metadata requested");
      ap_lstnr->want_metadata = true;
    }

  /* The request seems ok. Now build the response */
  some_error
    = (0 == (to_write = srv_build_http_positive_response (
               ap_server, ap_lstnr->buf.p_data, ICE_LISTENER_BUF_SIZE - 1,
               ap_server->bitrate, ap_server->num_channels,
               ap_server->sample_rate, ap_lstnr->want_metadata)));
  bail_on_request_error (some_error, 500, "Internal Server Error");

  some_error = (0 == srv_send_http_response (ap_server, ap_lstnr));
  bail_on_request_error (some_error, 500, "Internal Server Error");

  some_error = false;
  ap_lstnr->need_response = false;

end:
  if (some_error && OMX_ErrorNone == rc)
    {
      rc = OMX_ErrorInsufficientResources;
    }

  return rc;
}

static OMX_S32
srv_remove_existing_listener (OMX_PTR ap_key, OMX_PTR ap_value, OMX_PTR ap_arg)
{
  httpr_server_t * p_server = ap_arg;
  httpr_listener_t * p_lstnr = NULL;

  assert (p_server);
  assert (ap_key);
  assert (ap_value);

  p_lstnr = (httpr_listener_t *) ap_value;

  srv_remove_listener (p_server, p_lstnr);

  return 0;
}

inline static void
srv_release_empty_buffer (httpr_server_t * ap_server,
                          httpr_listener_t * ap_lstnr,
                          OMX_BUFFERHEADERTYPE ** app_hdr)
{
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;

  assert (ap_server);
  assert (ap_lstnr);
  assert (app_hdr);

  p_hdr = *app_hdr;

  ap_lstnr->pos = 0;
  p_hdr->nFilledLen = 0;
  ap_server->pf_release_buf (p_hdr, ap_server->p_arg);
  *app_hdr = NULL;
  ap_server->p_hdr = NULL;
}

static bool
srv_is_listener_ready (httpr_server_t * ap_server, httpr_listener_t * ap_lstnr)
{
  bool lstnr_ready = true;
  OMX_HANDLETYPE p_hdl = NULL;
  assert (ap_server);
  assert (ap_lstnr);
  p_hdl = handleOf (ap_server->p_parent);

  if (ap_lstnr->need_response)
    {
      OMX_ERRORTYPE rc = OMX_ErrorNone;
      if (OMX_ErrorNone
          != (rc = srv_handle_listeners_request (ap_server, ap_lstnr)))
        {
          if (OMX_ErrorNotReady == rc)
            {
              TIZ_ERROR (p_hdl, "no data yet lets wait some time ");
              (void) srv_start_listener_io_watcher (ap_lstnr);
            }
          else
            {
              TIZ_ERROR (p_hdl,
                         "[%s] : while handling the "
                         "listener's initial request. Will remove the listener",
                         tiz_err_to_str (rc));
              srv_remove_listener (ap_server, ap_lstnr);
            }
          lstnr_ready = false;
        }
    }
  return lstnr_ready;
}

static inline bool
srv_is_time_to_send_metadata (httpr_server_t * ap_server,
                              httpr_listener_t * ap_lstnr)
{
  if (ap_lstnr->p_con->sent_total == 0)
    {
      return false;
    }

  if (((ap_lstnr->p_con->sent_total + ap_server->burst_size)
       % ap_server->mountpoint.metadata_period)
      <= ap_server->burst_size)
    {
      return true;
    }

  return false;
}

static inline size_t
srv_get_metadata_offset (const httpr_server_t * ap_server,
                         const httpr_listener_t * ap_lstnr)
{
  size_t offset = 0;
  if (ap_lstnr->p_con->sent_total != 0)
    {
      offset = (ap_server->mountpoint.metadata_period
                * ((ap_lstnr->p_con->sent_total + ap_server->burst_size)
                   / ap_server->mountpoint.metadata_period))
               - ap_lstnr->p_con->sent_total;
    }

  return offset;
}

static inline size_t
srv_get_metadata_length (const httpr_server_t * ap_server,
                         const httpr_listener_t * ap_lstnr)
{
  if (ap_lstnr->p_con->sent_total == 0 || ap_lstnr->p_con->metadata_delivered)
    {
      return 0;
    }

  return strnlen ((char *) ap_server->mountpoint.stream_title,
                  OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE);
}

static void
srv_arrange_metadata (httpr_server_t * ap_server, httpr_listener_t * ap_lstnr,
                      OMX_U8 ** app_buffer, size_t * ap_len)
{
  size_t len = 0;
  httpr_listener_buffer_t * p_lstnr_buf = NULL;

  assert (ap_server);
  assert (ap_lstnr);
  assert (app_buffer);
  assert (ap_len);

  len = *ap_len;
  p_lstnr_buf = &ap_lstnr->buf;

  if (0 == len || !ap_lstnr->want_metadata
      || 0 == ap_server->mountpoint.metadata_period
      || !srv_is_time_to_send_metadata (ap_server, ap_lstnr))
    {
      /* p_lstnr_buf->metadata_bytes = 0; */
      return;
    }

  {
    OMX_U8 * p_buffer = *app_buffer;
    size_t metadata_offset = srv_get_metadata_offset (ap_server, ap_lstnr);

    if (metadata_offset < len && 0 == p_lstnr_buf->metadata_bytes)
      {
        OMX_U8 * p_dest = NULL;
        OMX_U8 * p_src = NULL;
        size_t metadata_len = srv_get_metadata_length (ap_server, ap_lstnr);
        size_t metadata_total = 0;
        size_t metadata_byte = 0;

        if (metadata_len > 0)
          {
            metadata_byte = metadata_len / 16;
            if (metadata_len % 16)
              {
                metadata_byte++;
              }
          }
        else
          {
            metadata_byte = 0;
          }

        metadata_total = (metadata_byte * 16) + 1;
        p_dest = p_buffer + metadata_offset + metadata_total;
        p_src = p_buffer + metadata_offset;

        /* Move content to make space for the metadata */
        memmove (p_dest, p_src, len - metadata_offset);

        p_lstnr_buf->metadata_bytes += metadata_total;
        p_lstnr_buf->metadata_offset += metadata_offset;

        if (metadata_len)
          {
            snprintf ((char *) p_src, metadata_total, "%c%s",
                      (int) metadata_byte,
                      (char *) ap_server->mountpoint.stream_title);
            ap_lstnr->p_con->metadata_delivered = true;
          }
        else
          {
            p_src[0] = metadata_byte;
          }

        len += metadata_total;
        p_lstnr_buf->len = len;

        *ap_len = len;
        *app_buffer = p_buffer;
      }
  }
}

static void
srv_arrange_data (httpr_server_t * ap_server, httpr_listener_t * ap_lstnr,
                  OMX_U8 ** app_buffer, size_t * ap_len)
{
  OMX_U8 * p_buffer = NULL;
  size_t len = 0;
  httpr_listener_buffer_t * p_lstnr_buf = NULL;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;

  assert (ap_server);
  assert (ap_lstnr);
  assert (app_buffer);
  assert (ap_len);

  p_lstnr_buf = &ap_lstnr->buf;
  p_hdr = ap_server->p_hdr;

  if (p_hdr && p_hdr->pBuffer && p_hdr->nFilledLen > 0)
    {
      int to_copy = 0;
      if (ap_server->burst_size > p_lstnr_buf->len)
        {
          to_copy = ap_server->burst_size - p_lstnr_buf->len;
        }
      if (to_copy > p_hdr->nFilledLen)
        {
          to_copy = p_hdr->nFilledLen;
        }
      memcpy (p_lstnr_buf->p_data + p_lstnr_buf->len,
              p_hdr->pBuffer + p_hdr->nOffset, to_copy);
      p_lstnr_buf->len += to_copy;
      p_hdr->nFilledLen -= to_copy;
      p_hdr->nOffset += to_copy;
    }

  p_buffer = (OMX_U8 *) p_lstnr_buf->p_data;
  len = p_lstnr_buf->len;

  srv_arrange_metadata (ap_server, ap_lstnr, &p_buffer, &len);

  *ap_len = len;
  *app_buffer = p_buffer;
}

static OMX_ERRORTYPE
srv_write_to_listener (httpr_server_t * ap_server, httpr_listener_t * ap_lstnr,
                       const void * ap_buffer, const size_t a_buf_len,
                       int * a_bytes_written)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int bytes = 0;
  httpr_connection_t * p_con = NULL;
  int sock = ICE_SOCK_ERROR;

  assert (ap_server);
  assert (ap_lstnr);
  assert (ap_buffer);
  assert (a_bytes_written);

  p_con = ap_lstnr->p_con;
  sock = p_con->sockfd;
  *a_bytes_written = 0;
  errno = 0;

  bytes = send (sock, (const void *) ap_buffer, a_buf_len, MSG_NOSIGNAL);

  if (bytes < 0)
    {
      if (!srv_is_recoverable_error (ap_server, sock, errno))
        {
          TIZ_PRINTF_DBG_RED (
            "Non-recoverable error while writing to the socket (will destroy "
            "listener)\n");
          /* Mark the listener as failed, so that it will get removed */
          rc = OMX_ErrorNoMore;
        }
      else
        {
          TIZ_PRINTF_DBG_RED (
            "Recoverable error while writing to the socket"
            "(re-starting io watcher)\n");
          (void) srv_start_listener_io_watcher (ap_lstnr);
          srv_stop_listener_timer_watcher (ap_lstnr);
          rc = OMX_ErrorNotReady;
        }
    }
  else
    {
      *a_bytes_written = bytes;
    }
  return rc;
}

static OMX_ERRORTYPE
srv_write_omx_buffer (httpr_server_t * ap_server, httpr_listener_t * ap_lstnr)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  httpr_connection_t * p_con = NULL;
  int sock = ICE_SOCK_ERROR;

  assert (ap_server);
  assert (ap_lstnr);
  assert (ap_lstnr->p_con);

  p_con = ap_lstnr->p_con;
  sock = p_con->sockfd;

  p_con->sent_last = 0;

  if (!srv_is_valid_socket (sock))
    {
      TIZ_WARN (handleOf (ap_server->p_parent),
                "Will destroy listener "
                "(Invalid listener socket fd [%d])",
                sock);
      /* The socket is not valid anymore. The listener will be removed. */
      rc = OMX_ErrorNoMore;
    }
  else
    {
      httpr_listener_buffer_t * p_lstnr_buf = &ap_lstnr->buf;
      OMX_U8 * p_buffer = NULL;
      size_t len = 0;
      int bytes = 0;

      /* Obtain a pointer to the data and the amount of data to be written */
      srv_arrange_data (ap_server, ap_lstnr, &p_buffer, &len);

      if (len > 0)
        {
          int metadata_sent = 0;
          tiz_check_omx (
            srv_write_to_listener (ap_server, ap_lstnr, p_buffer, len, &bytes));
          assert (bytes >= 0);

          p_lstnr_buf->len
            = bytes > p_lstnr_buf->len ? 0 : (p_lstnr_buf->len - bytes);

          if (p_lstnr_buf->len > 0)
            {
              memmove (p_lstnr_buf->p_data, p_lstnr_buf->p_data + bytes,
                       p_lstnr_buf->len);
            }

          if (bytes > p_lstnr_buf->metadata_offset)
            {
              metadata_sent = MIN ((bytes - p_lstnr_buf->metadata_offset),
                                   p_lstnr_buf->metadata_bytes);
            }

          if (p_con->initial_burst_bytes > 0)
            {
              p_con->initial_burst_bytes -= (bytes - metadata_sent);
            }
          else
            {
              if (p_con->con_time == 0)
                {
                  p_con->con_time = time (NULL);
                }
            }

          p_con->sent_total += (bytes - metadata_sent);
          p_con->sent_last = (bytes - metadata_sent);
          p_con->burst_bytes += (bytes - metadata_sent);

          {
            time_t t = time (NULL);
            double d = difftime (t, p_con->con_time);
            uint64_t rate = d ? p_con->sent_total / (uint64_t) d : 0;
            TIZ_PRINTF_DBG_BLU (
              "total [%lld] last [%d] burst [%d] time [%f] rate [%lld] "
              "server burst [%d] bytes [%d]\n",
              p_con->sent_total, p_con->sent_last, p_con->burst_bytes, d, rate,
              ap_server->burst_size, bytes);
          }

          TIZ_PRINTF_DBG_GRN (
            "metadata_bytes [%u] < metadata_offset [%u] metadata_sent [%u]\n",
            p_lstnr_buf->metadata_bytes, p_lstnr_buf->metadata_offset,
            metadata_sent);

          p_lstnr_buf->metadata_bytes -= metadata_sent;
          if (p_lstnr_buf->metadata_offset > 0 && p_lstnr_buf->metadata_bytes)
            {
              p_lstnr_buf->metadata_offset -= ((unsigned int) bytes);
            }
          else
            {
              p_lstnr_buf->metadata_offset = 0;
            }

          if (bytes < len)
            {
              TIZ_PRINTF_DBG_RED (
                "NEED TO STOP bytes [%d] < len [%u] p_lstnr_buf->len [%u]\n",
                bytes, len, p_lstnr_buf->len);
              (void) srv_start_listener_io_watcher (ap_lstnr);
              srv_stop_listener_timer_watcher (ap_lstnr);
              rc = OMX_ErrorNotReady;
            }
          else
            {
              if ((p_con->initial_burst_bytes <= 0)
                  && (p_con->burst_bytes >= ap_server->burst_size))
                {
                  rc = srv_start_listener_timer_watcher (ap_lstnr,
                                                         ap_server->wait_time);
                }
            }
        }
    }

  return rc;
}

static OMX_ERRORTYPE
srv_accept_connection (httpr_server_t * ap_server)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  char * p_ip = NULL;
  httpr_listener_t * p_lstnr = NULL;
  httpr_connection_t * p_con = NULL;
  int connected_sockfd = ICE_SOCK_ERROR;
  bool all_ok = false;
  OMX_HANDLETYPE p_hdl = NULL;

  assert (ap_server);
  p_hdl = handleOf (ap_server->p_parent);

  if (srv_get_listeners_count (ap_server) > 0)
    {
      /* This is a simple solution to prevent more than one connection at
       * a time. One day this could be a multi-client renderer */
      tiz_map_for_each (ap_server->p_lstnrs, srv_remove_existing_listener,
                        ap_server);
    }

  if ((p_ip = (char *) tiz_mem_alloc (ICE_RENDERER_MAX_ADDR_LEN)))
    {
      unsigned short port = 0;
      OMX_U32 index = 0;

      connected_sockfd
        = srv_accept_socket (ap_server, p_ip, ICE_RENDERER_MAX_ADDR_LEN, &port);
      goto_end_on_socket_error (connected_sockfd, p_hdl,
                                "Unable to accept the connection");

      rc = srv_create_listener (ap_server, &p_lstnr, connected_sockfd, p_ip,
                                port);
      goto_end_on_omx_error (rc, p_hdl, "Unable to instantiate the listener");

      assert (p_lstnr);
      assert (p_lstnr->p_con);
      p_con = p_lstnr->p_con;

      rc = tiz_map_insert (ap_server->p_lstnrs, &(p_con->sockfd), p_lstnr,
                           &index);
      goto_end_on_omx_error (rc, p_hdl,
                             "Unable to add the listener to the map");

      rc = srv_start_listener_io_watcher (p_lstnr);
      goto_end_on_omx_error (rc, p_hdl,
                             "Unable to start the listener's io watcher");

      all_ok = true;
    }

end:

  if (!all_ok)
    {
      if (ICE_SOCK_ERROR != connected_sockfd)
        {
          close (connected_sockfd);
          connected_sockfd = ICE_SOCK_ERROR;
        }

      if (p_lstnr)
        {
          srv_remove_listener (ap_server, p_lstnr);
          p_lstnr = NULL;
        }

      if (p_ip)
        {
          tiz_mem_free (p_ip);
          p_ip = NULL;
        }

      if (OMX_ErrorInsufficientResources != rc)
        {
          /* Use OMX_ErrorNotReady to signal an error other than OOM */
          rc = OMX_ErrorNotReady;
        }
    }
  else
    {
      TIZ_NOTICE (p_hdl, "Client [%s:%u] fd [%d] now connected", p_con->p_ip,
                  p_con->port, p_con->sockfd);

      TIZ_PRINTF_DBG_RED ("Client connected [%s:%u]\n", p_con->p_ip,
                          p_con->port);
      TIZ_PRINTF_DBG_GRN (
        "\tburst [%d] sample rate [%u] bitrate [%u] "
        "burst_size [%u] bytes per frame [%u] wait_time [%f] "
        "pkts/s [%f].\n",
        (unsigned int) p_con->initial_burst_bytes,
        (unsigned int) ap_server->sample_rate,
        (unsigned int) ap_server->bitrate, (unsigned int) ap_server->burst_size,
        (unsigned int) ap_server->bytes_per_frame, ap_server->wait_time,
        ap_server->pkts_per_sec);
    }

  /* Always restart the server's watcher, even if an error occurred */
  srv_start_server_io_watcher (ap_server);

  return rc;
}

static OMX_ERRORTYPE
srv_write (httpr_server_t * ap_server)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE * p_hdr = NULL;
  httpr_listener_t * p_lstnr = NULL;
  httpr_connection_t * p_con = NULL;

  assert (ap_server);

  if (srv_get_listeners_count (ap_server) <= 0)
    {
      return OMX_ErrorNoMore;
    }

  /* Until support for multiple listeners gets implemented, there will only be
     one listener in the map */
  p_lstnr = srv_get_first_listener (ap_server);
  assert (p_lstnr);
  p_con = p_lstnr->p_con;
  assert (p_con);

  srv_stop_listener_io_watcher (p_lstnr);
  if (!srv_is_listener_ready (ap_server, p_lstnr))
    {
      return OMX_ErrorNotReady;
    }

  srv_start_listener_timer_watcher (p_lstnr, ap_server->wait_time);

  if (p_con->initial_burst_bytes <= 0)
    {
      p_con->burst_bytes = 0;
    }

  while (1)
    {
      if (NULL == p_hdr)
        {
          if (NULL == (p_hdr = ap_server->pf_acquire_buf (ap_server->p_arg)))
            {
              /* no more buffers available at the moment */
              ap_server->need_more_data = true;
              srv_stop_listener_timer_watcher (p_lstnr);
              rc = OMX_ErrorNone;
              break;
            }
          ap_server->need_more_data = false;
          ap_server->p_hdr = p_hdr;
        }

      rc = srv_write_omx_buffer (ap_server, p_lstnr);

      if (OMX_ErrorNoMore == rc)
        {
          srv_remove_listener (ap_server, p_lstnr);
          break;
        }

      if (OMX_ErrorNotReady == rc
          || ((p_con->initial_burst_bytes <= 0)
              && (p_con->burst_bytes >= ap_server->burst_size)))
        {
          rc = OMX_ErrorNotReady;
          break;
        }

      /*       if (p_lstnr->pos == ap_server->p_hdr->nFilledLen) */
      if (0 == ap_server->p_hdr->nFilledLen)
        {
          /* Buffer emptied */
          (void) srv_release_empty_buffer (ap_server, p_lstnr, &p_hdr);
        }
    };

  return rc;
}

static OMX_ERRORTYPE
srv_stream_to_client (httpr_server_t * ap_server)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_server);

  rc = srv_write (ap_server);
  switch (rc)
    {
      case OMX_ErrorNone:
      case OMX_ErrorNoMore:
      /* Socket not ready, send buffer is full, or the burst limit has been
         reached */
      case OMX_ErrorNotReady:
        {
          /* No connected clients just yet */
          rc = OMX_ErrorNone;
        }
        break;

      default:
        {
          TIZ_ERROR (handleOf (ap_server->p_parent), "[%s]",
                     tiz_err_to_str (rc));
          assert (0);
        }
        break;
    };
  return rc;
}

static int
srv_get_descriptor (const httpr_server_t * ap_server)
{
  assert (ap_server);
  return ap_server->lstn_sockfd;
}

/*               */
/* httpr con APIs */
/*               */

void
httpr_srv_destroy (httpr_server_t * ap_server)
{
  if (ap_server)
    {
      srv_destroy_server_io_watcher (ap_server);
      if (ICE_SOCK_ERROR != ap_server->lstn_sockfd)
        {
          close (ap_server->lstn_sockfd);
        }

      tiz_mem_free (ap_server->p_ip);
      if (ap_server->p_lstnrs)
        {
          tiz_map_clear (ap_server->p_lstnrs);
          tiz_map_destroy (ap_server->p_lstnrs);
        }
      tiz_mem_free (ap_server);
    }
}

OMX_ERRORTYPE
httpr_srv_init (httpr_server_t ** app_server, void * ap_parent,
                OMX_STRING a_address, OMX_U32 a_port, OMX_U32 a_max_clients,
                httpr_srv_release_buffer_f a_pf_release_buf,
                httpr_srv_acquire_buffer_f a_pf_acquire_buf, OMX_PTR ap_arg)
{
  httpr_server_t * p_server = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  bool all_ok = false;

  assert (app_server);
  assert (ap_parent);
  assert (a_pf_release_buf);
  assert (a_pf_acquire_buf);

  p_server = (httpr_server_t *) tiz_mem_calloc (1, sizeof (httpr_server_t));
  rc = p_server ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, handleOf (ap_parent),
                         "Unable to alloc the server struct");

  p_server->p_parent = ap_parent;
  p_server->lstn_sockfd = ICE_SOCK_ERROR;
  p_server->p_ip = NULL;
  p_server->p_srv_ev_io = NULL;
  p_server->max_clients = a_max_clients;
  p_server->p_lstnrs = NULL;
  p_server->p_hdr = NULL;
  p_server->pf_release_buf = a_pf_release_buf;
  p_server->pf_acquire_buf = a_pf_acquire_buf;
  p_server->need_more_data = true;
  p_server->running = false;
  p_server->p_arg = ap_arg;
  p_server->bitrate = 0;
  p_server->num_channels = 0;
  p_server->sample_rate = 0;
  p_server->bytes_per_frame = 144 * 128000 / 44100;
  p_server->burst_size = ICE_MEDIUM_BURST_SIZE;
  p_server->pkts_per_sec = (((double) p_server->bytes_per_frame
                             * (double) ((double) 1000 / (double) 26)
                             / (double) p_server->burst_size));
  p_server->wait_time = (1 / p_server->pkts_per_sec);

  tiz_mem_set (&(p_server->mountpoint), 0, sizeof (httpr_mount_t));
  p_server->mountpoint.metadata_period = ICE_DEFAULT_METADATA_INTERVAL;
  p_server->mountpoint.initial_burst_size = ICE_INITIAL_BURST_SIZE;
  p_server->mountpoint.max_clients = 1;

  if (a_address)
    {
      p_server->p_ip = strndup (a_address, ICE_RENDERER_MAX_ADDR_LEN);
    }
  rc = p_server->p_ip ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, handleOf (ap_parent),
                         "Unable to duo the server ip address");

  rc = tiz_map_init (&(p_server->p_lstnrs), listeners_map_compare_func,
                     listeners_map_free_func, NULL);
  goto_end_on_omx_error (rc, handleOf (ap_parent),
                         "Unable to init the listeners map");

  p_server->lstn_sockfd
    = srv_create_server_socket (p_server, a_port, a_address);
  goto_end_on_socket_error (p_server->lstn_sockfd, handleOf (ap_parent),
                            "Unable to create the server socket");

  rc = srv_allocate_server_io_watcher (p_server);
  goto_end_on_omx_error (rc, handleOf (ap_parent),
                         "Unable to alloc the server's io event");

  /* All good so far */
  all_ok = true;

end:
  if (!all_ok)
    {
      httpr_srv_destroy (p_server);
      p_server = NULL;
      rc = OMX_ErrorInsufficientResources;
    }

  *app_server = p_server;
  return rc;
}

OMX_ERRORTYPE
httpr_srv_start (httpr_server_t * ap_server)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = NULL;
  int listen_rc = ICE_SOCK_ERROR;
  bool all_ok = false;

  assert (ap_server);
  p_hdl = handleOf (ap_server->p_parent);

  errno = 0;
  listen_rc = listen (ap_server->lstn_sockfd, ICE_LISTEN_QUEUE);
  goto_end_on_socket_error (listen_rc, p_hdl, strerror (errno));

  rc = srv_set_non_blocking (ap_server->lstn_sockfd);
  goto_end_on_omx_error (rc, p_hdl, "Unable to set socket as non-blocking");

  rc = srv_start_server_io_watcher (ap_server);
  goto_end_on_omx_error (rc, p_hdl, "Unable to start the server io watcher");

  /* so far so good */
  ap_server->running = true;
  all_ok = true;

end:
  if (!all_ok && OMX_ErrorNone == rc)
    {
      rc = OMX_ErrorInsufficientResources;
    }
  return rc;
}

OMX_ERRORTYPE
httpr_srv_stop (httpr_server_t * ap_server)
{
  httpr_listener_t * p_lstnr = NULL;
  assert (ap_server);
  (void) srv_stop_server_io_watcher (ap_server);
  if (ap_server->p_lstnrs)
    {
      /* Until support for multiple listeners gets implemented, there will only
         be one listener in the map */
      p_lstnr = srv_get_first_listener (ap_server);
      if (p_lstnr)
        {
          srv_stop_listener_io_watcher (p_lstnr);
          srv_stop_listener_timer_watcher (p_lstnr);
          srv_remove_listener (ap_server, p_lstnr);
        }
    }
  ap_server->running = false;
  ap_server->need_more_data = false;
  return OMX_ErrorNone;
}

void
httpr_srv_release_buffers (httpr_server_t * ap_server)
{
  assert (ap_server);
  if (ap_server->p_hdr)
    {
      ap_server->p_hdr->nFilledLen = 0;
      ap_server->pf_release_buf (ap_server->p_hdr, ap_server->p_arg);
      ap_server->p_hdr = NULL;
    }
}

void
httpr_srv_set_mp3_settings (httpr_server_t * ap_server, const OMX_U32 a_bitrate,
                            const OMX_U32 a_num_channels,
                            const OMX_U32 a_sample_rate)
{
  assert (ap_server);

  ap_server->bitrate = (a_bitrate != 0 ? a_bitrate : 448000);
  ap_server->num_channels = (a_num_channels != 0 ? a_num_channels : 2);
  ap_server->sample_rate = (a_sample_rate != 0 ? a_sample_rate : 44100);
  assert (0 != a_sample_rate);
  ap_server->bytes_per_frame = (144 * ap_server->bitrate / a_sample_rate) + 1;
  ap_server->burst_size = ICE_MIN_BURST_SIZE;

  ap_server->pkts_per_sec
    = (((double) ap_server->bytes_per_frame * (double) (1000 / 26)
        / (double) ap_server->burst_size));

  ap_server->wait_time = (1 / ap_server->pkts_per_sec);

  if (srv_get_listeners_count (ap_server) > 0)
    {
      httpr_listener_t * p_lstnr = srv_get_first_listener (ap_server);
      assert (p_lstnr);
      srv_stop_listener_timer_watcher (p_lstnr);
      srv_start_listener_timer_watcher (p_lstnr, ap_server->wait_time);
    }

  TIZ_PRINTF_DBG_MAG (
    "burst [%d] sample rate [%u] bitrate [%u] "
    "burst_size [%u] bytes per frame [%u] wait_time [%f] "
    "pkts/s [%f].\n",
    (unsigned int) ap_server->mountpoint.initial_burst_size,
    (unsigned int) ap_server->sample_rate, (unsigned int) ap_server->bitrate,
    (unsigned int) ap_server->burst_size,
    (unsigned int) ap_server->bytes_per_frame, ap_server->wait_time,
    ap_server->pkts_per_sec);
}

void
httpr_srv_set_mountpoint_settings (
  httpr_server_t * ap_server, OMX_U8 * ap_mount_name, OMX_U8 * ap_station_name,
  OMX_U8 * ap_station_description, OMX_U8 * ap_station_genre,
  OMX_U8 * ap_station_url, const OMX_U32 a_metadata_period,
  const OMX_U32 a_burst_size, const OMX_U32 a_max_clients)
{
  httpr_mount_t * p_mount = NULL;

  assert (ap_server);
  assert (ap_mount_name);
  assert (ap_station_name);
  assert (ap_station_description);
  assert (ap_station_genre);
  assert (ap_station_url);

  p_mount = &(ap_server->mountpoint);

  strncpy ((char *) p_mount->mount_name, (char *) ap_mount_name,
           OMX_MAX_STRINGNAME_SIZE);
  p_mount->mount_name[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';

  strncpy ((char *) p_mount->station_name, (char *) ap_station_name,
           OMX_MAX_STRINGNAME_SIZE);
  p_mount->station_name[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';

  strncpy ((char *) p_mount->station_description,
           (char *) ap_station_description, OMX_MAX_STRINGNAME_SIZE);
  p_mount->station_description[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';

  strncpy ((char *) p_mount->station_genre, (char *) ap_station_genre,
           OMX_MAX_STRINGNAME_SIZE);
  p_mount->station_genre[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';

  strncpy ((char *) p_mount->station_url, (char *) ap_station_url,
           OMX_MAX_STRINGNAME_SIZE);
  p_mount->station_url[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';

  p_mount->metadata_period = a_metadata_period;
  p_mount->initial_burst_size = a_burst_size;
  p_mount->max_clients = a_max_clients;

  TIZ_NOTICE (handleOf (ap_server->p_parent),
              "StationName [%s] IcyMetadataPeriod [%d]", p_mount->station_name,
              p_mount->metadata_period);
}

void
httpr_srv_set_stream_title (httpr_server_t * ap_server,
                            OMX_U8 * ap_stream_title)
{
  httpr_mount_t * p_mount = NULL;

  assert (ap_server);
  assert (ap_stream_title);

  p_mount = &(ap_server->mountpoint);

  TIZ_PRINTF_DBG_YEL ("stream_title [%s]\n", ap_stream_title);

  strncpy ((char *) p_mount->stream_title, (char *) ap_stream_title,
           OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE);
  p_mount->stream_title[OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE - 1] = '\0';

  if (srv_get_listeners_count (ap_server) > 0)
    {
      httpr_listener_t * p_lstnr = srv_get_first_listener (ap_server);
      assert (p_lstnr);
      assert (p_lstnr->p_con);
      p_lstnr->p_con->metadata_delivered = false;
      p_lstnr->p_con->initial_burst_bytes
        = ap_server->mountpoint.initial_burst_size * 0.1;
      srv_stop_listener_timer_watcher (p_lstnr);
      srv_start_listener_timer_watcher (p_lstnr, ap_server->wait_time);
    }
}

OMX_ERRORTYPE
httpr_srv_buffer_event (httpr_server_t * ap_server)
{
  assert (ap_server);
  return ((ap_server->running && ap_server->need_more_data)
            ? srv_stream_to_client (ap_server)
            : OMX_ErrorNone);
}

OMX_ERRORTYPE
httpr_srv_io_event (httpr_server_t * ap_server, const int a_fd)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (ap_server);
  if (ap_server->running)
    {
      if (a_fd == srv_get_descriptor (ap_server))
        {
          /* A new connection event. Try to accept it */
          rc = srv_accept_connection (ap_server);
          if (OMX_ErrorInsufficientResources != rc)
            {
              rc = OMX_ErrorNone;
            }
        }
      else
        {
          /* The client socket is ready */
          rc = srv_stream_to_client (ap_server);
        }
    }
  return rc;
}

OMX_ERRORTYPE
httpr_srv_timer_event (httpr_server_t * ap_server)
{
  assert (ap_server);
  return ap_server->running ? srv_stream_to_client (ap_server) : OMX_ErrorNone;
}

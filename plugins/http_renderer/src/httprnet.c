/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   httprnet.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - HTTP renderer's networking functions
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
#include "httprnet.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer.prc.net"
#endif

#ifdef INET6_ADDRSTRLEN
#define ICE_RENDERER_MAX_ADDR_LEN INET6_ADDRSTRLEN
#else
#define ICE_RENDERER_MAX_ADDR_LEN 46
#endif

typedef struct httpr_listener_buffer httpr_listener_buffer_t;
struct httpr_listener_buffer
{
  unsigned int len;
  unsigned int metadata_bytes;
  char *p_data;
};

typedef struct httpr_mount httpr_mount_t;
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

typedef struct httpr_connection httpr_connection_t;
struct httpr_connection
{
  time_t con_time;
  uint64_t sent_total;
  unsigned int sent_last;
  unsigned int burst_bytes;
  OMX_S32 initial_burst_bytes;
  bool metadata_delivered;
  int sockfd;
  bool error;
  bool full;
  bool not_ready;
  char *p_host;
  char *p_ip;
  unsigned short port;
  tiz_event_io_t *p_ev_io;
  tiz_event_timer_t *p_ev_timer;
};

typedef struct httpr_listener httpr_listener_t;
struct httpr_listener
{
  OMX_HANDLETYPE p_hdl;
  httpr_connection_t *p_con;
  int respcode;
  long intro_offset;
  unsigned long pos;
  httpr_listener_buffer_t buf;
  tiz_http_parser_t *p_parser;
  bool need_response;
  bool timer_started;
  bool want_metadata;
};

struct httpr_server
{
  OMX_HANDLETYPE p_hdl;
  int lstn_sockfd;
  char *p_ip;
  tiz_event_io_t *p_srv_ev_io;
  OMX_U32 max_clients;
  tiz_map_t *p_lstnrs;
  OMX_BUFFERHEADERTYPE *p_hdr;
  httpr_buffer_emptied_f pf_emptied;
  httpr_buffer_needed_f pf_needed;
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

static void net_destroy_listener (httpr_listener_t *ap_lstnr);

static OMX_S32 listeners_map_compare_func (OMX_PTR ap_key1, OMX_PTR ap_key2)
{
  int *p_sockfd1 = (int *)ap_key1;
  int *p_sockfd2 = (int *)ap_key2;
  assert (NULL != ap_key1);
  assert (NULL != ap_key2);
  return (*p_sockfd1 == *p_sockfd2) ? 0 : ((*p_sockfd1 < *p_sockfd2) ? -1 : 1);
}

static void listeners_map_free_func (OMX_PTR ap_key, OMX_PTR ap_value)
{
  httpr_listener_t *p_lstnr = (httpr_listener_t *)ap_value;
  assert (NULL != p_lstnr);
  net_destroy_listener (p_lstnr);
}

static bool net_is_recoverable_error (httpr_server_t *ap_server, int sockfd,
                                      int error)
{
  assert (NULL != ap_server);
  TIZ_TRACE (ap_server->p_hdl, "Socket [%d] error [%s]", sockfd,
             strerror (error));

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

static inline bool net_is_valid_socket (int a_sockfd)
{
  int optval;
  socklen_t optlen = sizeof(int);
  return (0 == getsockopt (a_sockfd, SOL_SOCKET, SO_TYPE, (void *)&optval,
                           &optlen));
}

static inline int net_get_socket_buffer_size (const int a_sockfd)
{
  int optval;
  socklen_t optlen = sizeof(int);
  getsockopt (a_sockfd, SOL_SOCKET, SO_SNDBUF, (void *)&optval, &optlen);
  return optval;
}

static inline int net_get_socket_buffer_utilization (const int a_sockfd)
{
  int remaining = -1;
  ioctl (a_sockfd, TIOCOUTQ, &remaining);
  return remaining;
}

static inline int net_get_listeners_count (const httpr_server_t *ap_server)
{
  int rc = 0;
  if (ap_server && ap_server->p_lstnrs)
    {
      rc = tiz_map_size (ap_server->p_lstnrs);
    }
  return rc;
}

static OMX_ERRORTYPE net_set_non_blocking (const int sockfd)
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

static inline int net_set_nolinger (const int sock)
{
  struct linger lin = { 0, 0 };
  /* linger inactive. close call will return immediately to the caller, and any
   * pending data will be delivered if possible. */
  return setsockopt (sock, SOL_SOCKET, SO_LINGER, (void *)&lin,
                     sizeof(struct linger));
}

static inline int net_set_nodelay (const int sock)
{
  int nodelay = 1;
  return setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (void *)&nodelay,
                     sizeof(int));
}

static inline int net_set_keepalive (const int sock)
{
  int keepalive = 1;
  return setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive,
                     sizeof(int));
}

static int net_accept_socket (httpr_server_t *ap_server, char *ap_ip,
                              const size_t a_ip_len, unsigned short *ap_port)
{
  int accepted_sockfd = ICE_RENDERER_SOCK_ERROR;
  struct sockaddr_storage sa;
  socklen_t slen = sizeof(sa);
  OMX_HANDLETYPE p_hdl = NULL;

  assert (NULL != ap_server);
  assert (NULL != ap_ip);
  assert (NULL != ap_port);
  p_hdl = ap_server->p_hdl;

  if (!net_is_valid_socket (ap_server->lstn_sockfd))
    {
      TIZ_ERROR (p_hdl, "Invalid server socket fd [%d] ",
                 ap_server->lstn_sockfd);
    }
  else
    {
      errno = 0;
      accepted_sockfd
          = accept (ap_server->lstn_sockfd, (struct sockaddr *)&sa, &slen);

      if (accepted_sockfd != ICE_RENDERER_SOCK_ERROR)
        {
          int err = 0;
          if (0 != (err = getnameinfo ((struct sockaddr *)&sa, slen, ap_ip,
                                       a_ip_len, NULL, 0, NI_NUMERICHOST)))
            {
              snprintf (ap_ip, a_ip_len, "unknown");
              TIZ_ERROR (p_hdl, "getnameinfo error [%s]", gai_strerror (err));
            }
          else
            {
              if (sa.ss_family == AF_INET)
                {
                  struct sockaddr_in *p_sa_in = (struct sockaddr_in *)&sa;
                  *ap_port = ntohs (p_sa_in->sin_port);
                }
            }
          (void)net_set_nolinger (accepted_sockfd);
          (void)net_set_keepalive (accepted_sockfd);

          TIZ_TRACE (p_hdl, "Accepted [%s:%u] fd [%d]", ap_ip, *ap_port,
                     accepted_sockfd);
        }
    }
  return accepted_sockfd;
}

static inline int net_create_server_socket (httpr_server_t *ap_server,
                                            const int a_port,
                                            const char *a_interface)
{
  struct sockaddr_storage sa;
  struct addrinfo hints;
  struct addrinfo *res;
  struct addrinfo *ai;
  char service[10];
  int sockfd = ICE_RENDERER_SOCK_ERROR;
  int getaddrc = ICE_RENDERER_SOCK_ERROR;

  assert (NULL != ap_server);
  assert (a_port >= 0);

  tiz_mem_set (&sa, 0, sizeof(sa));
  tiz_mem_set (&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV | AI_NUMERICHOST;
  hints.ai_socktype = SOCK_STREAM;
  snprintf (service, sizeof(service), "%d", a_port);

  if ((getaddrc = getaddrinfo (a_interface, service, &hints, &res)) != 0)
    {
      TIZ_TRACE (ap_server->p_hdl, "[ICE_RENDERER_SOCK_ERROR] : %s.",
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

          setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&on,
                      sizeof(on));
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
  return ICE_RENDERER_SOCK_ERROR;
}

static void net_destroy_server_io_watcher (httpr_server_t *ap_server)
{
  assert (NULL != ap_server);
  tiz_event_io_destroy (ap_server->p_srv_ev_io);
  ap_server->p_srv_ev_io = NULL;
}

static OMX_ERRORTYPE net_allocate_server_io_watcher (httpr_server_t *ap_server)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_server);

  rc = tiz_event_io_init (&(ap_server->p_srv_ev_io), ap_server->p_hdl,
                          tiz_comp_event_io);
  goto_end_on_omx_error (rc, ap_server->p_hdl,
                         "Unable to init the server's io event");

  tiz_event_io_set (ap_server->p_srv_ev_io, ap_server->lstn_sockfd,
                    TIZ_EVENT_READ, true);

end:
  if (OMX_ErrorNone != rc)
    {
      net_destroy_server_io_watcher (ap_server);
    }

  return rc;
}

static OMX_ERRORTYPE net_start_server_io_watcher (httpr_server_t *ap_server)
{
  assert (NULL != ap_server);
  TIZ_TRACE (ap_server->p_hdl,
             "Starting server io watcher "
             "on fd [%d]",
             ap_server->lstn_sockfd);
  return tiz_event_io_start (ap_server->p_srv_ev_io);
}

static inline OMX_ERRORTYPE net_stop_server_io_watcher (
    httpr_server_t *ap_server)
{
  assert (NULL != ap_server);
  TIZ_TRACE (ap_server->p_hdl, "Stopping server io watcher on fd [%d] ",
             ap_server->lstn_sockfd);
  return tiz_event_io_stop (ap_server->p_srv_ev_io);
}

static OMX_ERRORTYPE net_start_listener_io_watcher (httpr_listener_t *ap_lstnr)
{
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_con);
  TIZ_TRACE (ap_lstnr->p_hdl, "Starting listener io watcher");
  return tiz_event_io_start (ap_lstnr->p_con->p_ev_io);
}

static OMX_ERRORTYPE net_stop_listener_io_watcher (httpr_listener_t *ap_lstnr)
{
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_con);
  TIZ_TRACE (ap_lstnr->p_hdl, "Stopping listener io watcher");
  return tiz_event_io_stop (ap_lstnr->p_con->p_ev_io);
}

static OMX_ERRORTYPE net_start_listener_timer_watcher (
    httpr_listener_t *ap_lstnr, const double a_wait_time)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_con);
  if (!ap_lstnr->timer_started)
    {
      TIZ_TRACE (ap_lstnr->p_hdl, "Starting listener timer watcher");
      tiz_event_timer_set (ap_lstnr->p_con->p_ev_timer, a_wait_time,
                           a_wait_time);
      tiz_check_omx_err (tiz_event_timer_start (ap_lstnr->p_con->p_ev_timer));
      ap_lstnr->timer_started = true;
    }
  return rc;
}

static OMX_ERRORTYPE net_stop_listener_timer_watcher (
    httpr_listener_t *ap_lstnr)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_con);
  if (ap_lstnr->timer_started)
    {
      TIZ_TRACE (ap_lstnr->p_hdl, "Stopping listener timer watcher");
      tiz_check_omx_err (tiz_event_timer_stop (ap_lstnr->p_con->p_ev_timer));
      ap_lstnr->timer_started = false;
    }
  return rc;
}

static void net_destroy_connection (httpr_connection_t *ap_con)
{
  if (ap_con)
    {
      if (ICE_RENDERER_SOCK_ERROR != ap_con->sockfd)
        {
          close (ap_con->sockfd);
        }
      tiz_mem_free (ap_con->p_ip);
      tiz_mem_free (ap_con->p_host);
      tiz_event_io_destroy (ap_con->p_ev_io);
      tiz_event_timer_destroy (ap_con->p_ev_timer);
      tiz_mem_free (ap_con);
    }
}

static void net_destroy_listener (httpr_listener_t *ap_lstnr)
{
  if (NULL != ap_lstnr)
    {
      net_stop_listener_timer_watcher (ap_lstnr);
      if (NULL != ap_lstnr->p_parser)
        {
          tiz_http_parser_destroy (ap_lstnr->p_parser);
        }
      tiz_mem_free (ap_lstnr->buf.p_data);
      net_destroy_connection (ap_lstnr->p_con);
      tiz_mem_free (ap_lstnr);
    }
}

static void net_remove_listener (httpr_server_t *ap_server,
                                 httpr_listener_t *ap_lstnr)
{
  int nlstnrs = 0;

  assert (NULL != ap_server);
  assert (NULL != ap_lstnr);

  nlstnrs = net_get_listeners_count (ap_server);
  assert (nlstnrs > 0);

  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "Destroyed listener [%s] - [%d] listeners remaining",
           ap_lstnr->p_con->p_ip, nlstnrs - 1);

  tiz_map_erase (ap_server->p_lstnrs, &ap_lstnr->p_con->sockfd);
  assert (nlstnrs - 1 == net_get_listeners_count (ap_server));

  /* NOTE: No need to call net_destroy_listener as this has been called already
   * by
   * the map's listeners_map_free_func */
}

static httpr_connection_t *net_create_connection (httpr_server_t *ap_server,
                                                  httpr_listener_t *ap_lstnr,
                                                  const int connected_sockfd,
                                                  char *ap_ip,
                                                  const unsigned short ap_port,
                                                  const double a_wait_time)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  httpr_connection_t *p_con = NULL;
  OMX_HANDLETYPE p_hdl = NULL;

  assert (NULL != ap_server);
  assert (NULL != ap_lstnr);
  p_hdl = ap_server->p_hdl;

  p_con = (httpr_connection_t *)tiz_mem_calloc (1, sizeof(httpr_connection_t));
  rc = p_con ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, p_hdl, "Unable to alloc the connection struct");

  p_con->con_time = time (NULL);
  p_con->sent_total = 0;
  p_con->sent_last = 0;
  p_con->burst_bytes = 0;
  p_con->initial_burst_bytes = ap_server->mountpoint.initial_burst_size;
  p_con->sockfd = connected_sockfd;
  p_con->error = false;
  p_con->full = false;
  p_con->not_ready = false;
  p_con->p_host = NULL;
  p_con->p_ip = ap_ip;
  p_con->port = ap_port;
  p_con->p_ev_io = NULL;
  p_con->p_ev_timer = NULL;

  rc = tiz_event_io_init (&(p_con->p_ev_io), p_hdl, tiz_comp_event_io);
  goto_end_on_omx_error (rc, p_hdl, "Unable to init the client's io event");

  /* We are interested in knowing when a listener socket is available for
   * writing  */
  tiz_event_io_set (p_con->p_ev_io, p_con->sockfd, TIZ_EVENT_WRITE, true);

  rc = tiz_event_timer_init (&(p_con->p_ev_timer), p_hdl, tiz_comp_event_timer,
                             ap_lstnr);
  goto_end_on_omx_error (rc, p_hdl, "Unable to init the client's timer event");

end:
  if (OMX_ErrorNone != rc)
    {
      net_destroy_connection (p_con);
      p_con = NULL;
    }

  return p_con;
}

static OMX_ERRORTYPE net_create_listener (httpr_server_t *ap_server,
                                          httpr_listener_t **app_lstnr,
                                          const int a_connected_sockfd,
                                          char *ap_ip,
                                          const unsigned short ap_port)
{
  OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
  httpr_listener_t *p_lstnr = NULL;
  httpr_connection_t *p_con = NULL;
  OMX_HANDLETYPE p_hdl = NULL;

  assert (NULL != ap_server);
  assert (NULL != app_lstnr);
  assert (ICE_RENDERER_SOCK_ERROR != a_connected_sockfd);
  assert (NULL != ap_ip);
  p_hdl = ap_server->p_hdl;

  p_lstnr = (httpr_listener_t *)tiz_mem_calloc (1, sizeof(httpr_listener_t));
  rc = p_lstnr ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, p_hdl, "Unable to alloc the listener structure");

  p_con = net_create_connection (ap_server, p_lstnr, a_connected_sockfd, ap_ip,
                                 ap_port, ap_server->wait_time);
  rc = p_con ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, p_hdl, "Unable to init the listener's connection");

  p_lstnr->p_hdl = ap_server->p_hdl;
  p_lstnr->p_con = p_con;
  p_lstnr->respcode = 200;
  p_lstnr->intro_offset = 0;
  p_lstnr->pos = 0;
  p_lstnr->buf.len = ICE_LISTENER_BUF_SIZE;
  p_lstnr->buf.metadata_bytes = 0;
  p_lstnr->p_parser = NULL;
  p_lstnr->need_response = true;
  p_lstnr->timer_started = false;
  p_lstnr->want_metadata = false;

  p_lstnr->buf.p_data = (char *)tiz_mem_alloc (ICE_LISTENER_BUF_SIZE);
  rc = p_lstnr->buf.p_data ? OMX_ErrorNone : OMX_ErrorInsufficientResources;
  goto_end_on_omx_error (rc, p_hdl, "Unable to alloc the listener's buffer");
  p_lstnr->buf.p_data[ICE_LISTENER_BUF_SIZE - 1] = '\000';

  rc = tiz_http_parser_init (&(p_lstnr->p_parser), ETIZHttpParserTypeRequest);
  goto_end_on_omx_error (rc, p_hdl, "Unable to init the http parser");

  rc = net_set_non_blocking (p_lstnr->p_con->sockfd);
  goto_end_on_omx_error (rc, p_hdl, "Unable to set socket non-blocking");

  rc = net_set_nodelay (p_lstnr->p_con->sockfd);
  goto_end_on_omx_error (rc, p_hdl, "Unable to set no delay");

  rc = OMX_ErrorNone;

end:

  if (OMX_ErrorNone != rc)
    {
      net_destroy_listener (p_lstnr);
      p_lstnr = NULL;
    }

  *app_lstnr = p_lstnr;

  return rc;
}

static int net_read_from_listener (httpr_listener_t *ap_lstnr)
{
  httpr_connection_t *p_con = NULL;
  httpr_listener_buffer_t *p_buf = NULL;

  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_con);

  p_con = ap_lstnr->p_con;
  p_buf = &ap_lstnr->buf;
  assert (NULL != p_buf->p_data);
  assert (p_buf->len > 0);

  errno = 0;
  return recv (p_con->sockfd, p_buf->p_data, p_buf->len, 0);
}

static ssize_t net_build_http_negative_response (char *ap_buf, size_t len,
                                                 int status, const char *ap_msg)
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

  snprintf (status_buffer, sizeof(status_buffer), "HTTP/%s %d %s\r\n",
            http_version, status, statusmsg);
  snprintf (contenttype_buffer, sizeof(contenttype_buffer),
            "Content-Type: %s\r\n", contenttype);

  time (&now);
  gmtime_result = gmtime_r (&now, &result);

  strftime (currenttime_buffer, sizeof(currenttime_buffer),
            "Date: %a, %d-%b-%Y %X GMT\r\n", gmtime_result);

  ret = snprintf (ap_buf, len, "%sServer: %s\r\n%s%s%s%s", status_buffer,
                  "Tizonia HTTP Server 0.1.0", currenttime_buffer,
                  contenttype_buffer, (ap_msg ? "\r\n" : ""),
                  (ap_msg ? ap_msg : ""));

  return ret;
}

static void net_send_http_error (httpr_server_t *ap_server,
                                 httpr_listener_t *ap_lstnr, int a_error,
                                 const char *ap_err_msg)
{
  ssize_t resp_size = 0;

  assert (NULL != ap_server);
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->buf.p_data);
  assert (NULL != ap_lstnr->p_con);
  assert (NULL != ap_err_msg);

  ap_lstnr->buf.p_data[ICE_LISTENER_BUF_SIZE - 1] = '\000';
  resp_size = net_build_http_negative_response (
      ap_lstnr->buf.p_data, ICE_LISTENER_BUF_SIZE - 1, a_error, "");

  snprintf (ap_lstnr->buf.p_data + resp_size, ICE_LISTENER_BUF_SIZE - resp_size,
            "<html><head><title>Error %i</title></head><body><b>%i - %s</b>"
            "</body></html>\r\n",
            a_error, a_error, ap_err_msg);

  ap_lstnr->buf.len = strnlen (ap_lstnr->buf.p_data, ICE_LISTENER_BUF_SIZE);

  send (ap_lstnr->p_con->sockfd, ap_lstnr->buf.p_data, ap_lstnr->buf.len, 0);
  ap_lstnr->buf.len = 0;
}

static ssize_t net_build_http_positive_response (
    httpr_server_t *ap_server, char *ap_buf, size_t len, OMX_U32 a_bitrate,
    OMX_U32 a_num_channels, OMX_U32 a_sample_rate, bool a_want_metadata)
{
  const char *http_version = "1.0";
  char status_buffer[80];
  char contenttype_buffer[80];
  char icybr_buffer[80];
  char icyname_buffer[OMX_MAX_STRINGNAME_SIZE];
  char icydescription_buffer[OMX_MAX_STRINGNAME_SIZE];
  char icygenre_buffer[OMX_MAX_STRINGNAME_SIZE];
  char icyurl_buffer[OMX_MAX_STRINGNAME_SIZE];
  char icypub_buffer[80];
  char iceaudioinfo_buffer[80];
  char icymetaint_buffer[80];
  ssize_t ret;
  const char *statusmsg = "OK";
  const char *contenttype = "audio/mpeg";
  int status = 200;
  int pub = 0;
  bool metadata_needed = false;

  assert (NULL != ap_server);
  assert (NULL != ap_buf);

  /* HTTP status line */
  snprintf (status_buffer, sizeof(status_buffer), "HTTP/%s %d %s\r\n",
            http_version, status, statusmsg);

  /* HTTP Content-Type header */
  snprintf (contenttype_buffer, sizeof(contenttype_buffer),
            "Content-Type: %s\r\n", contenttype);

  /* icy-br header */
  snprintf (icybr_buffer, sizeof(icybr_buffer), "icy-br:%d\r\n",
            (int)a_bitrate / 1000);

  /* ice-audio-info header */
  snprintf (iceaudioinfo_buffer, sizeof(iceaudioinfo_buffer),
            "ice-audio-info: "
            "bitrate=%d;channels=%d;samplerate=%d\r\n",
            (int)a_bitrate, (int)a_num_channels, (int)a_sample_rate);

  /* icy-name header */
  snprintf (icyname_buffer, sizeof(icyname_buffer), "icy-name:%s\r\n",
            ap_server->mountpoint.station_name);

  /* icy-decription header */
  snprintf (icydescription_buffer, sizeof(icydescription_buffer),
            "icy-description:%s\r\n",
            ap_server->mountpoint.station_description);

  /* icy-genre header */
  snprintf (icygenre_buffer, sizeof(icygenre_buffer), "icy-genre:%s\r\n",
            ap_server->mountpoint.station_genre);

  /* icy-url header */
  snprintf (icyurl_buffer, sizeof(icyurl_buffer), "icy-url:%s\r\n",
            ap_server->mountpoint.station_url);

  /* icy-pub header */
  snprintf (icypub_buffer, sizeof(icypub_buffer), "icy-pub:%u\r\n", pub);

  if (ap_server->mountpoint.metadata_period > 0 && a_want_metadata)
    {
      metadata_needed = true;
      /* icy-metaint header */
      snprintf (icymetaint_buffer, sizeof(icymetaint_buffer),
                "icy-metaint:%lu\r\n", ap_server->mountpoint.metadata_period);
    }

  ret = snprintf (
      ap_buf, len, "%s%s%s%s%s%s%s%s%s%s%s%s\r\n", status_buffer,
      contenttype_buffer, icybr_buffer, iceaudioinfo_buffer, icyname_buffer,
      icydescription_buffer, icygenre_buffer, icyurl_buffer, icypub_buffer,
      (metadata_needed ? icymetaint_buffer : ""),
      "Server: Tizonia HTTP Renderer 0.1.0\r\n", "Cache-Control: no-cache\r\n");

  return ret;
}

static int net_send_http_response (httpr_server_t *ap_server,
                                   httpr_listener_t *ap_lstnr)
{
  ssize_t sent_bytes = 0;

  assert (NULL != ap_server);
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->buf.p_data);
  assert (NULL != ap_lstnr->p_con);

  ap_lstnr->buf.len = strnlen (ap_lstnr->buf.p_data, ICE_LISTENER_BUF_SIZE);

  sent_bytes = send (ap_lstnr->p_con->sockfd, ap_lstnr->buf.p_data,
                     ap_lstnr->buf.len, MSG_NOSIGNAL);
  ap_lstnr->buf.len = 0;
  return sent_bytes;
}

static OMX_ERRORTYPE net_handle_listeners_request (httpr_server_t *ap_server,
                                                   httpr_listener_t *ap_lstnr)
{
#define bail_on_request_error(some_error, httperr, msg)                \
  do                                                                   \
    {                                                                  \
      if (some_error)                                                  \
        {                                                              \
          TIZ_ERROR (ap_server->p_hdl, "[%s]", msg);                   \
          if (httperr > 0)                                             \
            {                                                          \
              net_send_http_error (ap_server, ap_lstnr, httperr, msg); \
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
  const char *parsed_string = NULL;

  assert (NULL != ap_server);
  assert (NULL != ap_lstnr);
  assert (NULL != ap_lstnr->p_con);
  assert (NULL != ap_lstnr->p_parser);

  some_error = (net_get_listeners_count (ap_server) > ap_server->max_clients);
  bail_on_request_error (some_error, 400, "Client limit reached");

  some_error
      = (ap_lstnr->p_con->con_time + ICE_DEFAULT_HEADER_TIMEOUT <= time (NULL));
  bail_on_request_error (some_error, -1, "Connection timed out");

  some_error = ((nread = net_read_from_listener (ap_lstnr)) <= 0);
  rc = (some_error ? (net_is_recoverable_error (ap_server,
                                                ap_lstnr->p_con->sockfd, errno)
                          ? OMX_ErrorNotReady
                          : OMX_ErrorNone)
                   : OMX_ErrorNone);
  bail_on_request_error (some_error, -1, strerror (errno));

  nparsed
      = tiz_http_parser_parse (ap_lstnr->p_parser, ap_lstnr->buf.p_data, nread);
  some_error = (nparsed != nread);
  bail_on_request_error (some_error, 400, "Bad request");

  some_error = (NULL == (parsed_string
                         = tiz_http_parser_get_method (ap_lstnr->p_parser))
                || (0 != strncmp ("GET", parsed_string, strlen ("GET"))));
  bail_on_request_error (some_error, 405, "Method not allowed");

  some_error
      = (NULL == (parsed_string = tiz_http_parser_get_url (ap_lstnr->p_parser))
         || (0 != strncmp ("/", parsed_string, strlen ("/"))));
  bail_on_request_error (some_error, 401, "Unathorized");

  if (NULL != (parsed_string = tiz_http_parser_get_header (ap_lstnr->p_parser,
                                                           "Icy-MetaData"))
      && (0 == strncmp ("1", parsed_string, strlen ("1"))))
    {
      TIZ_TRACE (ap_server->p_hdl, "ICY metadata requested");
      ap_lstnr->want_metadata = true;
    }

  /* The request seems ok. Now build the response */
  some_error
      = (0 == (to_write = net_build_http_positive_response (
                   ap_server, ap_lstnr->buf.p_data, ICE_LISTENER_BUF_SIZE - 1,
                   ap_server->bitrate, ap_server->num_channels,
                   ap_server->sample_rate, ap_lstnr->want_metadata)));
  bail_on_request_error (some_error, 500, "Internal Server Error");

  some_error = (0 == net_send_http_response (ap_server, ap_lstnr));
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

static OMX_S32 net_remove_existing_listener (OMX_PTR ap_key, OMX_PTR ap_value,
                                             OMX_PTR ap_arg)
{
  httpr_server_t *p_server = ap_arg;
  httpr_listener_t *p_lstnr = NULL;

  assert (NULL != p_server);
  assert (NULL != ap_key);
  assert (NULL != ap_value);

  p_lstnr = (httpr_listener_t *)ap_value;

  net_remove_listener (p_server, p_lstnr);

  return 0;
}

static OMX_S32 net_remove_failed_listener (OMX_PTR ap_key, OMX_PTR ap_value,
                                           OMX_PTR ap_arg)
{
  httpr_server_t *p_server = ap_arg;
  httpr_listener_t *p_lstnr = NULL;

  assert (NULL != p_server);
  assert (NULL != ap_key);
  assert (NULL != ap_value);

  p_lstnr = (httpr_listener_t *)ap_value;

  if (NULL != p_lstnr->p_con && true == p_lstnr->p_con->error)
    {
      net_remove_listener (p_server, p_lstnr);
    }

  return 0;
}

inline static void net_release_empty_buffer (httpr_server_t *ap_server,
                                             httpr_listener_t *ap_lstnr,
                                             OMX_BUFFERHEADERTYPE **app_hdr)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  assert (NULL != ap_server);
  assert (NULL != ap_lstnr);
  assert (NULL != app_hdr);

  p_hdr = *app_hdr;

  ap_lstnr->pos = 0;
  p_hdr->nFilledLen = 0;
  ap_server->pf_emptied (p_hdr, ap_server->p_arg);
  *app_hdr = NULL;
  ap_server->p_hdr = NULL;
}

static bool net_listener_ready (httpr_server_t *ap_server,
                                httpr_listener_t *ap_lstnr)
{
  bool lstnr_ready = true;
  OMX_HANDLETYPE p_hdl = NULL;
  assert (NULL != ap_server);
  assert (NULL != ap_lstnr);
  p_hdl = ap_server->p_hdl;

  if (ap_lstnr->need_response)
    {
      OMX_ERRORTYPE rc = OMX_ErrorNone;
      if (OMX_ErrorNone
          != (rc = net_handle_listeners_request (ap_server, ap_lstnr)))
        {
          if (OMX_ErrorNotReady == rc)
            {
              TIZ_ERROR (p_hdl, "no data yet lets wait some time ");
              (void)net_start_listener_io_watcher (ap_lstnr);
            }
          else
            {
              TIZ_ERROR (p_hdl,
                         "[%s] : while handling the "
                         "listener's initial request. Will remove the listener",
                         tiz_err_to_str (rc));
              net_remove_listener (ap_server, ap_lstnr);
            }
          lstnr_ready = false;
        }
    }
  return lstnr_ready;
}

static inline bool net_is_time_to_send_metadata (httpr_server_t *ap_server,
                                                 httpr_listener_t *ap_lstnr)
{
  if (ap_lstnr->p_con->sent_total == 0)
    {
      return false;
    }

  if (((ap_lstnr->p_con->sent_total + ap_server->burst_size)
       % ap_server->mountpoint.metadata_period) <= ap_server->burst_size)
    {
      return true;
    }

  return false;
}

static inline size_t net_get_metadata_offset (const httpr_server_t *ap_server,
                                              const httpr_listener_t *ap_lstnr)
{
  if (ap_lstnr->p_con->sent_total == 0)
    {
      return 0;
    }

  return (ap_server->mountpoint.metadata_period
          * ((ap_lstnr->p_con->sent_total + ap_server->burst_size)
             / ap_server->mountpoint.metadata_period))
         - ap_lstnr->p_con->sent_total;
}

static inline size_t net_get_metadata_length (const httpr_server_t *ap_server,
                                              const httpr_listener_t *ap_lstnr)
{
  if (ap_lstnr->p_con->sent_total == 0 || ap_lstnr->p_con->metadata_delivered)
    {
      return 0;
    }

  return strnlen ((char *)ap_server->mountpoint.stream_title,
                  OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE);
}

static void net_arrange_metadata (httpr_server_t *ap_server,
                                  httpr_listener_t *ap_lstnr,
                                  OMX_U8 **app_buffer, size_t *ap_len)
{
  size_t len = 0;
  httpr_listener_buffer_t *p_lstnr_buf = NULL;

  assert (NULL != ap_server);
  assert (NULL != ap_lstnr);
  assert (NULL != app_buffer);
  assert (NULL != ap_len);

  len = *ap_len;
  p_lstnr_buf = &ap_lstnr->buf;

  if (0 == len || !ap_lstnr->want_metadata
      || 0 == ap_server->mountpoint.metadata_period
      || !net_is_time_to_send_metadata (ap_server, ap_lstnr))
    {
      p_lstnr_buf->metadata_bytes = 0;
      return;
    }

  /* If metadata needs to be sent in this burst, copy both data + metadata into
   * the listener buffer */
  {
    OMX_HANDLETYPE p_hdl = ap_server->p_hdl;
    OMX_U8 *p_buffer = *app_buffer;
    size_t metadata_offset = net_get_metadata_offset (ap_server, ap_lstnr);

    TIZ_TRACE (p_hdl, "metadata_offset=[%d] p_lstnr_buf->len [%d] len [%d]",
               metadata_offset, p_lstnr_buf->len, len);

    if (metadata_offset < len)
      {
        OMX_U8 *p_dest = NULL;
        OMX_U8 *p_src = NULL;
        size_t metadata_len = net_get_metadata_length (ap_server, ap_lstnr);
        size_t metadata_total = 0;
        size_t metadata_byte = 0;

        /* We use the listener's buffer to inline the metadata */
        if (p_lstnr_buf->len == 0)
          {
            memcpy (p_lstnr_buf->p_data, p_buffer, len);
            p_buffer = (OMX_U8 *)p_lstnr_buf->p_data;
            p_lstnr_buf->len = len;
            ap_lstnr->pos += len;
          }

        if (metadata_len > 0)
          {
            metadata_byte = (metadata_len - 1) / 16 + 1;
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

        p_lstnr_buf->metadata_bytes = metadata_total;

        if (metadata_len)
          {
            snprintf ((char *)p_src, metadata_total, "%c%s", (int)metadata_byte,
                      (char *)ap_server->mountpoint.stream_title);
            ap_lstnr->p_con->metadata_delivered = true;
            TIZ_TRACE (p_hdl, "p_src[0]=[%u] offset [%d] (metadata [%s])",
                       (unsigned int)p_src[0], p_src - p_buffer,
                       (char *)p_src + 1);
          }
        else
          {
            p_src[0] = metadata_byte;
          }

        len += metadata_total;
        p_lstnr_buf->len = len;

        TIZ_TRACE (p_hdl,
                   "p_src[0]=[%u] "
                   "(metadata_len [%d] metadata_offset [%d] "
                   "metadata_total [%d]) inserting at byte [%d] "
                   "p_lstnr_buf->len [%d] stream_title_len [%d]",
                   (unsigned int)p_src[0], metadata_len, metadata_offset,
                   metadata_total,
                   ap_lstnr->p_con->sent_total + metadata_offset,
                   p_lstnr_buf->len, p_lstnr_buf->metadata_bytes);

        *ap_len = len;
        *app_buffer = p_buffer;
      }
  }
}

static void net_arrange_data (httpr_server_t *ap_server,
                              httpr_listener_t *ap_lstnr, OMX_U8 **app_buffer,
                              size_t *ap_len)
{
  OMX_U8 *p_buffer = NULL;
  size_t len = 0;
  httpr_listener_buffer_t *p_lstnr_buf = NULL;
  OMX_HANDLETYPE p_hdl = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  assert (NULL != ap_server);
  assert (NULL != ap_lstnr);
  assert (NULL != app_buffer);
  assert (NULL != ap_len);

  p_lstnr_buf = &ap_lstnr->buf;
  p_hdr = ap_server->p_hdr;
  p_hdl = ap_server->p_hdl;
  (void)p_hdl;

  if (p_lstnr_buf->len > 0)
    {
      TIZ_TRACE (p_hdl,
                 "lstnr buffer len : [%u] "
                 "p_lstnr->pos [%d] p_hdr [%p]",
                 p_lstnr_buf->len, ap_lstnr->pos, p_hdr);

      if (NULL != p_hdr && NULL != p_hdr->pBuffer && p_hdr->nFilledLen > 0)
        {
          int to_copy = ap_server->burst_size - p_lstnr_buf->len;
          if (to_copy > p_hdr->nFilledLen)
            {
              to_copy = p_hdr->nFilledLen;
            }
          memcpy (p_lstnr_buf->p_data + p_lstnr_buf->len,
                  p_hdr->pBuffer + p_hdr->nOffset, to_copy);
          ap_lstnr->pos = to_copy;
          p_lstnr_buf->len += to_copy;
        }

      p_buffer = (OMX_U8 *)p_lstnr_buf->p_data;
      len = p_lstnr_buf->len;
      TIZ_TRACE (p_hdl, "len : [%d] p_lstnr->pos [%d]", len, ap_lstnr->pos);
    }
  else
    {
      if (p_hdr->nFilledLen > 0)
        {
          p_buffer = p_hdr->pBuffer + p_hdr->nOffset + ap_lstnr->pos;

          assert (p_hdr->nAllocLen >= (p_hdr->nOffset + p_hdr->nFilledLen));
          assert (ap_lstnr->pos <= p_hdr->nFilledLen);

          len = p_hdr->nFilledLen - ap_lstnr->pos;
          if (len > ap_server->burst_size)
            {
              len = ap_server->burst_size;
            }
        }
    }

  net_arrange_metadata (ap_server, ap_lstnr, &p_buffer, &len);

  *ap_len = len;
  *app_buffer = p_buffer;
}

static OMX_S32 net_write_omx_buffer (OMX_PTR ap_key, OMX_PTR ap_value,
                                     OMX_PTR ap_arg)
{
  httpr_server_t *p_server = ap_arg;
  httpr_listener_t *p_lstnr = NULL;
  httpr_listener_buffer_t *p_lstnr_buf = NULL;
  httpr_connection_t *p_con = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_U8 *p_buffer = NULL;
  int sock = ICE_RENDERER_SOCK_ERROR;
  size_t len = 0;
  int bytes = 0;

  assert (NULL != p_server);
  assert (NULL != p_server->p_hdr);
  assert (NULL != ap_key);
  assert (NULL != ap_value);

  p_hdr = p_server->p_hdr;
  p_lstnr = (httpr_listener_t *)ap_value;
  assert (NULL != p_lstnr->p_con);
  p_con = p_lstnr->p_con;
  p_lstnr_buf = &p_lstnr->buf;
  sock = p_con->sockfd;

  p_con->sent_last = 0;
  p_con->error = false;
  p_con->full = false;
  p_con->not_ready = false;

  TIZ_TRACE (p_server->p_hdl, "socket fd [%d] sent_total [%d]", sock,
             p_con->sent_total);

  if (!net_is_valid_socket (sock))
    {
      TIZ_WARN (p_server->p_hdl,
                "Destroying listener "
                "(Invalid listener socket fd [%d])",
                sock);
      /* Mark the listener as failed, so that it will get removed */
      p_con->error = true;
      return 0;
    }

  /* Obtain a pointer to the data and the amount of data to be written */
  net_arrange_data (p_server, p_lstnr, &p_buffer, &len);

  if (len > 0)
    {
      errno = 0;
      bytes = send (sock, (const void *)p_buffer, len, MSG_NOSIGNAL);

      if (bytes < 0)
        {
          if (!net_is_recoverable_error (p_server, sock, errno))
            {
              TIZ_WARN (p_server->p_hdl,
                        "Destroying listener "
                        "(non-recoverable error while writing to socket ");
              /* Mark the listener as failed, so that it will get removed */
              p_con->error = true;
            }
          else
            {
              TIZ_TRACE (p_server->p_hdl,
                         "Recoverable error "
                         "(re-starting io watcher)");
              (void)net_start_listener_io_watcher (p_lstnr);
              (void)net_stop_listener_timer_watcher (p_lstnr);
              p_con->not_ready = true;
            }
        }
      else
        {
          TIZ_TRACE (p_server->p_hdl,
                     "bytes [%d] p_lstnr_buf->len [%d] "
                     "burst_bytes [%d] stream_title_len [%d]",
                     bytes, p_lstnr_buf->len, p_con->burst_bytes,
                     p_lstnr_buf->metadata_bytes);

          if (p_lstnr_buf->len > 0)
            {
              p_lstnr_buf->len -= bytes;
            }
          else
            {
              p_lstnr->pos += (bytes - p_lstnr_buf->metadata_bytes);
            }

          if (p_con->initial_burst_bytes > 0)
            {
              p_con->initial_burst_bytes
                  -= (bytes - p_lstnr_buf->metadata_bytes);
            }
          else
            {
              if (p_con->con_time == 0)
                {
                  p_con->con_time = time (NULL);
                }
            }

          p_con->sent_total += (bytes - p_lstnr_buf->metadata_bytes);
          p_con->sent_last = (bytes - p_lstnr_buf->metadata_bytes);
          p_con->burst_bytes += (bytes - p_lstnr_buf->metadata_bytes);

          p_lstnr_buf->metadata_bytes = 0;

          if (bytes < len)
            {
              TIZ_TRACE (p_server->p_hdl,
                         "Send buffer full "
                         "(re-starting io watcher)");
              (void)net_start_listener_io_watcher (p_lstnr);
              (void)net_stop_listener_timer_watcher (p_lstnr);
              p_con->full = true;
            }
          else
            {
              if ((p_con->initial_burst_bytes <= 0)
                  && (p_con->burst_bytes >= p_server->burst_size))
                {
                  /* Let's not send too much data in one go */

                  TIZ_TRACE (p_server->p_hdl, "burst limit reached");

                  if ((p_hdr->nFilledLen - p_lstnr->pos) < p_server->burst_size)
                    {
                      /* copy the remaining data into the listener's buffer */
                      int bytes_to_copy = p_hdr->nFilledLen - p_lstnr->pos;
                      memcpy (p_lstnr->buf.p_data,
                              p_hdr->pBuffer + p_lstnr->pos, bytes_to_copy);
                      p_lstnr->buf.len = bytes_to_copy;
                      TIZ_TRACE (p_server->p_hdl,
                                 "Copied to lstnr buffer : "
                                 "%d bytes",
                                 bytes_to_copy);

                      /* Buffer emptied */
                      net_release_empty_buffer (p_server, p_lstnr,
                                                &p_server->p_hdr);
                    }

                  net_start_listener_timer_watcher (p_lstnr,
                                                    p_server->wait_time);
                }
            }
        }
    }

  /* always return success */
  return 0;
}

/*               */
/* httpr con APIs */
/*               */

OMX_ERRORTYPE
httpr_net_server_init (httpr_server_t **app_server, OMX_HANDLETYPE ap_hdl,
                       OMX_STRING a_address, OMX_U32 a_port,
                       OMX_U32 a_max_clients,
                       httpr_buffer_emptied_f a_pf_emptied,
                       httpr_buffer_needed_f a_pf_needed, OMX_PTR ap_arg)
{
  httpr_server_t *p_server = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int sockfd = ICE_RENDERER_SOCK_ERROR;
  ;
  bool some_error = true;

  assert (NULL != app_server);
  assert (NULL != ap_hdl);
  assert (NULL != a_pf_emptied);
  assert (NULL != a_pf_needed);

  if (NULL == (p_server
               = (httpr_server_t *)tiz_mem_calloc (1, sizeof(httpr_server_t))))
    {
      TIZ_TRACE (ap_hdl,
                 "[OMX_ErrorInsufficientResources] : "
                 "allocating the httpr_server_t struct.");
      return OMX_ErrorInsufficientResources;
    }

  p_server->p_hdl = ap_hdl;
  p_server->lstn_sockfd = ICE_RENDERER_SOCK_ERROR;
  p_server->p_ip = NULL;
  p_server->p_srv_ev_io = NULL;
  p_server->max_clients = a_max_clients;
  p_server->p_lstnrs = NULL;
  p_server->p_hdr = NULL;
  p_server->pf_emptied = a_pf_emptied;
  p_server->pf_needed = a_pf_needed;
  p_server->p_arg = ap_arg;
  p_server->bitrate = 0;
  p_server->num_channels = 0;
  p_server->sample_rate = 0;
  p_server->bytes_per_frame = 144 * 128000 / 44100;
  p_server->burst_size = ICE_MEDIUM_BURST_SIZE;
  p_server->pkts_per_sec
      = (((double)p_server->bytes_per_frame * (double)(1000 / 26)
          / (double)p_server->burst_size));
  p_server->wait_time = (1 / p_server->pkts_per_sec);

  tiz_mem_set (&(p_server->mountpoint), 0, sizeof(httpr_mount_t));
  p_server->mountpoint.metadata_period = ICE_DEFAULT_METADATA_INTERVAL;
  p_server->mountpoint.initial_burst_size = ICE_INITIAL_BURST_SIZE;
  ;
  p_server->mountpoint.max_clients = 1;

  if (NULL != a_address)
    {
      /* TODO : Check against NULL */
      p_server->p_ip = strndup (a_address, ICE_RENDERER_MAX_ADDR_LEN);
    }

  if (OMX_ErrorNone
      != (rc = tiz_map_init (&(p_server->p_lstnrs), listeners_map_compare_func,
                             listeners_map_free_func, NULL)))
    {
      TIZ_ERROR (ap_hdl, "[%s] : initializing the listeners map",
                 tiz_err_to_str (rc));
      goto end;
    }

  if (ICE_RENDERER_SOCK_ERROR
      == (p_server->lstn_sockfd
          = net_create_server_socket (p_server, a_port, a_address)))
    {
      TIZ_ERROR (ap_hdl,
                 "[OMX_ErrorInsufficientResources] : "
                 "creating the server socket.");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }

  if (OMX_ErrorNone != (rc = net_allocate_server_io_watcher (p_server)))
    {
      TIZ_ERROR (ap_hdl, "[%s] : allocating the server's io event.",
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
          tiz_mem_free (p_server->p_ip);
          tiz_mem_free (p_server);
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

void httpr_net_server_destroy (httpr_server_t *ap_server)
{
  if (ap_server)
    {
      net_destroy_server_io_watcher (ap_server);
      if (ICE_RENDERER_SOCK_ERROR != ap_server->lstn_sockfd)
        {
          close (ap_server->lstn_sockfd);
        }

      tiz_mem_free (ap_server->p_ip);
      tiz_map_clear (ap_server->p_lstnrs);
      tiz_map_destroy (ap_server->p_lstnrs);
      tiz_mem_free (ap_server);
    }
}

OMX_ERRORTYPE
httpr_net_start_listening (httpr_server_t *ap_server)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = NULL;
  int listen_rc = ICE_RENDERER_SOCK_ERROR;
  bool all_ok = false;

  assert (NULL != ap_server);
  p_hdl = ap_server->p_hdl;

  errno = 0;
  listen_rc = listen (ap_server->lstn_sockfd, ICE_LISTEN_QUEUE);
  goto_end_on_socket_error (listen_rc, p_hdl, strerror (errno));

  rc = net_set_non_blocking (ap_server->lstn_sockfd);
  goto_end_on_omx_error (rc, p_hdl, "Unable to set socket as non-blocking");

  rc = net_start_server_io_watcher (ap_server);
  goto_end_on_omx_error (rc, p_hdl, "Unable to start the server io watcher");

  /* so far so good */
  all_ok = true;

end:
  if (!all_ok && OMX_ErrorNone == rc)
    {
      rc = OMX_ErrorInsufficientResources;
    }
  return rc;
}

OMX_ERRORTYPE
httpr_net_accept_connection (httpr_server_t *ap_server)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  char *p_ip = NULL;
  httpr_listener_t *p_lstnr = NULL;
  httpr_connection_t *p_con = NULL;
  int connected_sockfd = ICE_RENDERER_SOCK_ERROR;
  bool all_ok = false;
  OMX_HANDLETYPE p_hdl = NULL;

  assert (NULL != ap_server);
  p_hdl = ap_server->p_hdl;

  /* This is a simple solution to prevent more than one connection at
   * a time. One day this mmay be a multi-client renderer */
  if (net_get_listeners_count (ap_server) > 0)
    {
      tiz_map_for_each (ap_server->p_lstnrs, net_remove_existing_listener,
                        ap_server);
    }

  if (NULL != (p_ip = (char *)tiz_mem_alloc (ICE_RENDERER_MAX_ADDR_LEN)))
    {
      unsigned short port = 0;
      OMX_U32 index = 0;

      connected_sockfd = net_accept_socket (ap_server, p_ip,
                                            ICE_RENDERER_MAX_ADDR_LEN, &port);
      goto_end_on_socket_error (connected_sockfd, p_hdl,
                                "Unable to accept the connection");

      rc = net_create_listener (ap_server, &p_lstnr, connected_sockfd, p_ip,
                                port);
      goto_end_on_omx_error (rc, p_hdl, "Unable to instantiate the listener");

      assert (NULL != p_lstnr->p_con);
      p_con = p_lstnr->p_con;

      rc = tiz_map_insert (ap_server->p_lstnrs, &(p_con->sockfd), p_lstnr,
                           &index);
      goto_end_on_omx_error (rc, p_hdl,
                             "Unable to add the listener to the map");

      rc = net_start_listener_io_watcher (p_lstnr);
      goto_end_on_omx_error (rc, p_hdl,
                             "Unable to start the listener's io watcher");

      all_ok = true;
    }

end:

  if (!all_ok)
    {
      if (ICE_RENDERER_SOCK_ERROR != connected_sockfd)
        {
          close (connected_sockfd);
          connected_sockfd = ICE_RENDERER_SOCK_ERROR;
        }

      if (p_lstnr)
        {
          net_remove_listener (ap_server, p_lstnr);
          p_lstnr = NULL;
        }

      if (p_ip)
        {
          tiz_mem_free (p_ip);
          p_ip = NULL;
        }

      if (OMX_ErrorInsufficientResources != rc)
        {
          /* For now use this error code to report the error condition */
          rc = OMX_ErrorNotReady;
        }
    }
  else
    {
      TIZ_TRACE (p_hdl, "Client [%s:%u] fd [%d] now connected", p_con->p_ip,
                 p_con->port, p_con->sockfd);

      TIZ_PRINTF_YEL (
          "burst [%d] sample rate [%u] bitrate [%u] "
          "burst_size [%u] bytes per frame [%u] wait_time [%f] "
          "pkts/s [%f].",
          (unsigned int)p_con->initial_burst_bytes,
          (unsigned int)ap_server->sample_rate,
          (unsigned int)ap_server->bitrate, (unsigned int)ap_server->burst_size,
          (unsigned int)ap_server->bytes_per_frame, ap_server->wait_time,
          ap_server->pkts_per_sec);
    }

  /* Always restart the server's watcher, even if an error occurred */
  net_start_server_io_watcher (ap_server);

  return rc;
}

OMX_ERRORTYPE
httpr_net_stop_listening (httpr_server_t *ap_server)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (ap_server)
    {
      rc = net_stop_server_io_watcher (ap_server);
    }
  return rc;
}

OMX_ERRORTYPE
httpr_net_write_to_listener (httpr_server_t *ap_server)
{
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  httpr_listener_t *p_lstnr = NULL;
  httpr_connection_t *p_con = NULL;
  OMX_HANDLETYPE p_hdl = NULL;

  if (NULL == ap_server)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "server does not exist yet.");
      return OMX_ErrorNotReady;
    }

  p_hdl = ap_server->p_hdl;

  if (tiz_map_empty (ap_server->p_lstnrs))
    {
      /* no clients connected yet */
      TIZ_TRACE (p_hdl, "no clients connected.");
      return OMX_ErrorNotReady;
    }

  /* Do like this for now until support for multiple listeners is implemented */
  p_lstnr = tiz_map_value_at (ap_server->p_lstnrs, 0);
  assert (NULL != p_lstnr);
  p_con = p_lstnr->p_con;
  assert (NULL != p_con);

  net_stop_listener_io_watcher (p_lstnr);
  net_start_listener_timer_watcher (p_lstnr, ap_server->wait_time);

  TIZ_TRACE (p_hdl,
             "total [%llu] last [%u] burst [%u] cache [%d] "
             "srate [%d] brate [%d] pkts/s [%f] wait [%f] socket [%d]",
             p_con->sent_total, p_con->sent_last, p_con->burst_bytes,
             p_con->initial_burst_bytes, ap_server->sample_rate,
             ap_server->bitrate, ap_server->pkts_per_sec, ap_server->wait_time,
             net_get_socket_buffer_size (p_con->sockfd)
             - net_get_socket_buffer_utilization (p_con->sockfd));

  if (!net_listener_ready (ap_server, p_lstnr))
    {
      return OMX_ErrorNone;
    }

  if (p_con->initial_burst_bytes <= 0)
    {
      p_con->burst_bytes = 0;
    }

  while (1)
    {

      if (NULL == p_hdr)
        {
          if (NULL == (p_hdr = ap_server->pf_needed (ap_server->p_arg)))
            {
              /* no more buffers available at the moment */
              TIZ_TRACE (p_hdl, "no more buffers available");
              net_stop_listener_timer_watcher (p_lstnr);
              rc = OMX_ErrorNone;
              break;
            }

          ap_server->p_hdr = p_hdr;
        }

      /* NOTE: We assume only one client for now. But still do it like this in
       * preparation for when multiple clients are fully supported */
      tiz_map_for_each (ap_server->p_lstnrs, net_write_omx_buffer, ap_server);
      tiz_map_for_each (ap_server->p_lstnrs, net_remove_failed_listener,
                        ap_server);

      if (net_get_listeners_count (ap_server) == 0)
        {
          break;
        }

      if (p_con->full || p_con->not_ready
          || (((p_con->initial_burst_bytes <= 0))
              && (p_con->burst_bytes >= ap_server->burst_size)))
        {
          rc = OMX_ErrorNoMore;
          break;
        }

      if (p_lstnr->pos == ap_server->p_hdr->nFilledLen)
        {
          TIZ_TRACE (p_hdl,
                     "Buffer emptied : sent_total "
                     "[%llu] sent_last [%u] burst [%u]",
                     p_con->sent_total, p_con->sent_last, p_con->burst_bytes);

          /* Buffer emptied */
          net_release_empty_buffer (ap_server, p_lstnr, &p_hdr);
        }
    };

  return rc;
}

int httpr_net_get_server_fd (const httpr_server_t *ap_server)
{
  assert (NULL != ap_server);
  return ap_server->lstn_sockfd;
}

void httpr_net_release_buffers (httpr_server_t *ap_server)
{
  assert (NULL != ap_server);
  if (ap_server->p_hdr)
    {
      ap_server->p_hdr->nFilledLen = 0;
      ap_server->pf_emptied (ap_server->p_hdr, ap_server->p_arg);
      ap_server->p_hdr = NULL;
    }
}

void httpr_net_set_mp3_settings (httpr_server_t *ap_server,
                                 const OMX_U32 a_bitrate,
                                 const OMX_U32 a_num_channels,
                                 const OMX_U32 a_sample_rate)
{
  assert (NULL != ap_server);

  ap_server->bitrate = (a_bitrate != 0 ? a_bitrate : 448000);
  ap_server->num_channels = (a_num_channels != 0 ? a_num_channels : 2);
  ap_server->sample_rate = (a_sample_rate != 0 ? a_sample_rate : 44100);
  assert (0 != a_sample_rate);
  ap_server->bytes_per_frame = (144 * ap_server->bitrate / a_sample_rate) + 1;
  ap_server->burst_size = ICE_MIN_BURST_SIZE;

  ap_server->pkts_per_sec
      = (((double)ap_server->bytes_per_frame * (double)(1000 / 26)
          / (double)ap_server->burst_size));

  ap_server->wait_time = (1 / ap_server->pkts_per_sec);

  if (net_get_listeners_count (ap_server) > 0)
    {
      httpr_listener_t *p_lstnr = tiz_map_value_at (ap_server->p_lstnrs, 0);
      assert (NULL != p_lstnr);
      net_stop_listener_timer_watcher (p_lstnr);
      net_start_listener_timer_watcher (p_lstnr, ap_server->wait_time);
    }

  TIZ_PRINTF_YEL (
      "burst [%d] sample rate [%u] bitrate [%u] "
      "burst_size [%u] bytes per frame [%u] wait_time [%f] "
      "pkts/s [%f].%s\n",
      (unsigned int)ap_server->mountpoint.initial_burst_size,
      (unsigned int)ap_server->sample_rate, (unsigned int)ap_server->bitrate,
      (unsigned int)ap_server->burst_size,
      (unsigned int)ap_server->bytes_per_frame, ap_server->wait_time,
      ap_server->pkts_per_sec);

  TIZ_TRACE (ap_server->p_hdl,
             "sample rate [%d] bitrate [%d] "
             "burst_size "
             "[%d] bytes per frame [%d] wait_time [%f] "
             "pkts/s [%f]",
             a_sample_rate, a_bitrate, ap_server->burst_size,
             ap_server->bytes_per_frame, ap_server->wait_time,
             ap_server->pkts_per_sec);
}

void httpr_net_set_mountpoint_settings (
    httpr_server_t *ap_server, OMX_U8 *ap_mount_name, OMX_U8 *ap_station_name,
    OMX_U8 *ap_station_description, OMX_U8 *ap_station_genre,
    OMX_U8 *ap_station_url, const OMX_U32 a_metadata_period,
    const OMX_U32 a_burst_size, const OMX_U32 a_max_clients)
{
  httpr_mount_t *p_mount = NULL;

  assert (NULL != ap_server);
  assert (NULL != ap_mount_name);
  assert (NULL != ap_station_name);
  assert (NULL != ap_station_description);
  assert (NULL != ap_station_genre);
  assert (NULL != ap_station_url);

  p_mount = &(ap_server->mountpoint);

  strncpy ((char *)p_mount->mount_name, (char *)ap_mount_name,
           OMX_MAX_STRINGNAME_SIZE);
  p_mount->mount_name[OMX_MAX_STRINGNAME_SIZE - 1] = '\000';

  strncpy ((char *)p_mount->station_name, (char *)ap_station_name,
           OMX_MAX_STRINGNAME_SIZE);
  p_mount->station_name[OMX_MAX_STRINGNAME_SIZE - 1] = '\000';

  strncpy ((char *)p_mount->station_description, (char *)ap_station_description,
           OMX_MAX_STRINGNAME_SIZE);
  p_mount->station_description[OMX_MAX_STRINGNAME_SIZE - 1] = '\000';

  strncpy ((char *)p_mount->station_genre, (char *)ap_station_genre,
           OMX_MAX_STRINGNAME_SIZE);
  p_mount->station_genre[OMX_MAX_STRINGNAME_SIZE - 1] = '\000';

  strncpy ((char *)p_mount->station_url, (char *)ap_station_url,
           OMX_MAX_STRINGNAME_SIZE);
  p_mount->station_url[OMX_MAX_STRINGNAME_SIZE - 1] = '\000';

  p_mount->metadata_period = a_metadata_period;
  p_mount->initial_burst_size = a_burst_size;
  p_mount->max_clients = a_max_clients;

  TIZ_TRACE (ap_server->p_hdl, "StationName [%s] IcyMetadataPeriod [%d]",
             p_mount->station_name, p_mount->metadata_period);
}

void httpr_net_set_icecast_metadata (httpr_server_t *ap_server,
                                     OMX_U8 *ap_stream_title)
{
  httpr_mount_t *p_mount = NULL;

  assert (NULL != ap_server);
  assert (NULL != ap_stream_title);

  p_mount = &(ap_server->mountpoint);

  TIZ_TRACE (ap_server->p_hdl, "ap_stream_title [%s]", ap_stream_title);

  strncpy ((char *)p_mount->stream_title, (char *)ap_stream_title,
           OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE);
  p_mount->stream_title[OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE - 1] = '\000';

  if (net_get_listeners_count (ap_server) > 0)
    {
      httpr_listener_t *p_lstnr = tiz_map_value_at (ap_server->p_lstnrs, 0);
      assert (NULL != p_lstnr);
      assert (NULL != p_lstnr->p_con);
      p_lstnr->p_con->metadata_delivered = false;
      p_lstnr->p_con->initial_burst_bytes
          = ap_server->mountpoint.initial_burst_size * 0.1;
      net_stop_listener_timer_watcher (p_lstnr);
      net_start_listener_timer_watcher (p_lstnr, ap_server->wait_time);
    }
}

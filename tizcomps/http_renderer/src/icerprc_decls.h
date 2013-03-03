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
 * @file   icerprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Icecast-like HTTP Sink processor class
 * declarations
 *
 *
 */

#ifndef ICERPRC_DECLS_H
#define ICERPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include "tizosal.h"

#include "icerprc.h"
#include "tizproc_decls.h"

#define ICE_LISTENER_BUF_SIZE  4096
#define ICE_DEFAULT_HEADER_TIMEOUT 10
  typedef struct icer_connection icer_connection_t;
  struct icer_connection
  {
    time_t con_time;
    time_t discon_time;
    uint64_t sent_bytes;
    int sock;
    int error;
    char *p_ip;
    char *p_host;
  };

  typedef struct icer_listener_buffer icer_listener_buffer_t;
  struct icer_listener_buffer
  {
    unsigned int len;
    unsigned int count;
    char *p_data;
    bool sync_point;
  };

  typedef struct icer_listener icer_listener_t;
  struct icer_listener
  {
    icer_listener_t *p_next;
    icer_connection_t *p_con;
    int respcode;
    long intro_offset;
    unsigned int pos;
    icer_listener_buffer_t buf;
    tiz_http_parser_t *p_parser;
  };

  struct icerprc
  {
    /* Object */
    const struct tizproc _;
    OMX_STRING bind_address_;   /* if this is null, the server will listen on all
                                   interfaces. */
    OMX_U32 listening_port_;
    OMX_STRING mount_name_;
    OMX_U32 max_clients_;
    OMX_U32 nclients_;
    OMX_U32 burst_size_;
    bool eos_;
    int srv_sockfd_;
    int *p_clnt_socket_lst_;
    tiz_event_io_t *p_srv_ev_io_;
    tiz_event_io_t **p_clnt_ev_io_lst_;
    icer_listener_t *p_listener_lst_;
  };

#ifdef __cplusplus
}
#endif

#endif                          /* ICERPRC_DECLS_H */

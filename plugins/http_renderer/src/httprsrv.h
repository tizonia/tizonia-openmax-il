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
 * @file   httprsrv.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia - HTTP renderer's server functions
 *
 *
 */

#ifndef HTTPRSRV_H
#define HTTPRSRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

typedef struct httpr_server httpr_server_t;

typedef void (*httpr_srv_release_buffer_f) (OMX_BUFFERHEADERTYPE * ap_hdr,
                                            OMX_PTR ap_arg);
typedef OMX_BUFFERHEADERTYPE * (*httpr_srv_acquire_buffer_f) (OMX_PTR ap_arg);

OMX_ERRORTYPE
httpr_srv_init (httpr_server_t ** app_server, void * ap_parent,
                OMX_STRING a_address, OMX_U32 a_port, OMX_U32 a_max_clients,
                httpr_srv_release_buffer_f a_pf_release_buf,
                httpr_srv_acquire_buffer_f a_pf_acquire_buf, OMX_PTR ap_arg);

void
httpr_srv_destroy (httpr_server_t * ap_server);

OMX_ERRORTYPE
httpr_srv_start (httpr_server_t * ap_server);

OMX_ERRORTYPE
httpr_srv_stop (httpr_server_t * ap_server);

void
httpr_srv_release_buffers (httpr_server_t * ap_server);

void
httpr_srv_set_mp3_settings (httpr_server_t * ap_server, const OMX_U32 a_bitrate,
                            const OMX_U32 a_num_channels,
                            const OMX_U32 a_sample_rate);

void
httpr_srv_set_mountpoint_settings (
  httpr_server_t * ap_server, OMX_U8 * ap_mount_name, OMX_U8 * ap_station_name,
  OMX_U8 * ap_station_description, OMX_U8 * ap_station_genre,
  OMX_U8 * ap_station_url, const OMX_U32 metadata_period,
  const OMX_U32 burst_size, const OMX_U32 max_clients);

void
httpr_srv_set_stream_title (httpr_server_t * ap_server,
                            OMX_U8 * ap_stream_title);

OMX_ERRORTYPE
httpr_srv_buffer_event (httpr_server_t * ap_server);
OMX_ERRORTYPE
httpr_srv_io_event (httpr_server_t * ap_server, const int a_fd);
OMX_ERRORTYPE
httpr_srv_timer_event (httpr_server_t * ap_server);

#ifdef __cplusplus
}
#endif

#endif /* HTTPRSRV_H */

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
 * @file   icernet.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - HTTP renderer's net functions
 *
 *
 */

#ifndef ICERNET_H
#define ICERNET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"
#include "OMX_Types.h"

#define ICE_RENDERER_SOCK_ERROR (int)-1

  typedef struct icer_server icer_server_t;

  typedef void (*icer_buffer_emptied_f) (OMX_BUFFERHEADERTYPE * ap_hdr,
                                         OMX_PTR ap_arg);
  typedef OMX_BUFFERHEADERTYPE *(*icer_buffer_needed_f) (OMX_PTR ap_arg);

  OMX_ERRORTYPE icer_net_server_init (icer_server_t ** app_server,
                                      OMX_HANDLETYPE ap_hdl,
                                      OMX_STRING a_address, OMX_U32 a_port,
                                      OMX_U32 a_max_clients,
                                      icer_buffer_emptied_f a_pf_emptied,
                                      icer_buffer_needed_f a_pf_needed,
                                      OMX_PTR ap_arg);

  void icer_net_server_destroy (icer_server_t * ap_server);

  OMX_ERRORTYPE icer_net_start_listening (icer_server_t * ap_server);

  OMX_ERRORTYPE icer_net_accept_connection (icer_server_t * ap_server);

  OMX_ERRORTYPE icer_net_stop_listening (icer_server_t * ap_server);

  OMX_ERRORTYPE icer_net_write_to_listeners (icer_server_t * ap_server);

  int icer_net_get_server_fd (const icer_server_t * ap_server);

  void icer_net_set_mp3_settings (icer_server_t * ap_server,
                                  OMX_U32 a_bitrate,
                                  OMX_U32 a_num_channels,
                                  OMX_U32 a_sample_rate);

  void icer_net_set_mountpoint_settings (icer_server_t * ap_server,
                                         OMX_U8 *ap_mount_name,
                                         OMX_U8 *ap_station_name,
                                         OMX_U8 *ap_station_description,
                                         OMX_U8 *ap_station_genre,
                                         OMX_U8 *ap_station_url,
                                         OMX_U32 metadata_period,
                                         OMX_U32 burst_size,
                                         OMX_U32 max_clients);

  void icer_net_set_icecast_metadata (icer_server_t * ap_server,
                                      OMX_U8 *ap_stream_title);

#ifdef __cplusplus
}
#endif

#endif                          /* ICERNET_H */

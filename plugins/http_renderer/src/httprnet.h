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
 * @file   httprnet.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - HTTP renderer's net functions
 *
 *
 */

#ifndef HTTPRNET_H
#define HTTPRNET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

  typedef struct httpr_server httpr_server_t;

  typedef void (*httpr_buffer_emptied_f) (OMX_BUFFERHEADERTYPE * ap_hdr,
                                         OMX_PTR ap_arg);
  typedef OMX_BUFFERHEADERTYPE *(*httpr_buffer_needed_f) (OMX_PTR ap_arg);

  OMX_ERRORTYPE httpr_net_server_init (httpr_server_t ** app_server,
                                      OMX_HANDLETYPE ap_hdl,
                                      OMX_STRING a_address, OMX_U32 a_port,
                                      OMX_U32 a_max_clients,
                                      httpr_buffer_emptied_f a_pf_emptied,
                                      httpr_buffer_needed_f a_pf_needed,
                                      OMX_PTR ap_arg);

  void httpr_net_server_destroy (httpr_server_t * ap_server);

  OMX_ERRORTYPE httpr_net_start_listening (httpr_server_t * ap_server);

  OMX_ERRORTYPE httpr_net_accept_connection (httpr_server_t * ap_server);

  OMX_ERRORTYPE httpr_net_stop_listening (httpr_server_t * ap_server);

  OMX_ERRORTYPE httpr_net_write_to_listener (httpr_server_t * ap_server);

  int httpr_net_get_server_fd (const httpr_server_t * ap_server);

  void httpr_net_release_buffers (httpr_server_t * ap_server);

  void httpr_net_set_mp3_settings (httpr_server_t * ap_server,
                                  const OMX_U32 a_bitrate,
                                  const OMX_U32 a_num_channels,
                                  const OMX_U32 a_sample_rate);

  void httpr_net_set_mountpoint_settings (httpr_server_t * ap_server,
                                         OMX_U8 *ap_mount_name,
                                         OMX_U8 *ap_station_name,
                                         OMX_U8 *ap_station_description,
                                         OMX_U8 *ap_station_genre,
                                         OMX_U8 *ap_station_url,
                                         const OMX_U32 metadata_period,
                                         const OMX_U32 burst_size,
                                         const OMX_U32 max_clients);

  void httpr_net_set_icecast_metadata (httpr_server_t * ap_server,
                                      OMX_U8 *ap_stream_title);

#ifdef __cplusplus
}
#endif

#endif                          /* HTTPRNET_H */

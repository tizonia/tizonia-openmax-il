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
 * @file   icercon.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - Http renderer socket management
 *
 *
 */

#ifndef ICERCON_H
#define ICERCON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"
#include "OMX_Types.h"

#define ICE_RENDERER_SOCK_ERROR (int)-1

  typedef struct icer_server icer_server_t;
  typedef struct icer_listener_buffer icer_listener_buffer_t;
  typedef struct icer_connection icer_connection_t;
  typedef struct icer_listener icer_listener_t;

  OMX_ERRORTYPE icer_con_setup_server (icer_server_t ** app_server,
                                       OMX_HANDLETYPE ap_hdl,
                                       OMX_STRING a_address, OMX_U32 a_port,
                                       OMX_U32 a_max_clients);

  void icer_con_teardown_server (icer_server_t * ap_server);

  int icer_con_get_server_fd (const icer_server_t * ap_server);

  OMX_ERRORTYPE icer_con_start_listening (icer_server_t * ap_server,
                                          OMX_HANDLETYPE ap_hdl);

  icer_listener_t *icer_con_accept_connection (icer_server_t * ap_server,
                                               OMX_HANDLETYPE ap_hdl);

  OMX_ERRORTYPE icer_con_start_server_io_watcher (icer_server_t * ap_server,
                                            OMX_HANDLETYPE ap_hdl);

  OMX_ERRORTYPE icer_con_stop_server_io_watcher (icer_server_t * ap_server,
                                           OMX_HANDLETYPE ap_hdl);

#ifdef __cplusplus
}
#endif

#endif                          /* ICERCON_H */

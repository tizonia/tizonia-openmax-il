/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   httpr.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief   Tizonia - HTTP renderer constants
 *
 *
 */

#ifndef HTTPR_H
#define HTTPR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>
#include <OMX_TizoniaExt.h>

#define ARATELIA_HTTP_RENDERER_DEFAULT_ROLE "audio_renderer.http"
#define ARATELIA_HTTP_RENDERER_COMPONENT_NAME "OMX.Aratelia.audio_renderer.http"
#define ARATELIA_HTTP_RENDERER_PORT_INDEX \
  0 /* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_SIZE (8 * 1024)
#define ARATELIA_HTTP_RENDERER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_HTTP_RENDERER_PORT_ALIGNMENT 0
#define ARATELIA_HTTP_RENDERER_PORT_SUPPLIERPREF OMX_BufferSupplyInput
#define ARATELIA_HTTP_RENDERER_DEFAULT_HTTP_SERVER_PORT 8010

#define ICE_DEFAULT_METADATA_INTERVAL 16000
#define ICE_INITIAL_BURST_SIZE 128000
#define ICE_MAX_CLIENTS_PER_MOUNTPOINT 10
#define ICE_DEFAULT_HEADER_TIMEOUT 10
#define ICE_LISTEN_QUEUE 5
#define ICE_MIN_BURST_SIZE 1400
#define ICE_MEDIUM_BURST_SIZE 2800 /* Not used for now */
#define ICE_MAX_BURST_SIZE 4200    /* Not used for now */
#define ICE_LISTENER_BUF_SIZE \
  (ICE_MAX_BURST_SIZE + OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE)

#define ICE_SOCK_ERROR (int) -1

#define goto_end_on_socket_error(expr, hdl, msg) \
  do                                             \
    {                                            \
      if (ICE_SOCK_ERROR == (expr))              \
        {                                        \
          TIZ_ERROR (hdl, "%s (%s)", msg);       \
          goto end;                              \
        }                                        \
    }                                            \
  while (0)

#define goto_end_on_omx_error(expr, hdl, msg)                     \
  do                                                              \
    {                                                             \
      OMX_ERRORTYPE rc = OMX_ErrorNone;                           \
      if (OMX_ErrorNone != (rc = (expr)))                         \
        {                                                         \
          TIZ_ERROR (hdl, "[%s] : %s", tiz_err_to_str (rc), msg); \
          goto end;                                               \
        }                                                         \
    }                                                             \
  while (0)

#ifdef __cplusplus
}
#endif

#endif /* HTTPR_H */

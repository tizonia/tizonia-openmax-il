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
 * @file   httprprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - HTTP renderer processor declarations
 * declarations
 *
 *
 */

#ifndef HTTPRPRC_DECLS_H
#define HTTPRPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include <OMX_TizoniaExt.h>

#include <tizprc_decls.h>

#include "httprnet.h"

  typedef struct httpr_prc httpr_prc_t;
  struct httpr_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_STRING mount_name_;
    bool awaiting_buffers_;
    bool port_disabled_;
    int lstn_sockfd_;
    httpr_server_t *p_server_;
    OMX_BUFFERHEADERTYPE *p_inhdr_;
    OMX_AUDIO_PARAM_MP3TYPE mp3type_;
    OMX_TIZONIA_HTTPSERVERTYPE server_info_;
    OMX_TIZONIA_ICECASTMOUNTPOINTTYPE mountpoint_;
  };

  typedef struct httpr_prc_class httpr_prc_class_t;
  struct httpr_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* HTTPRPRC_DECLS_H */

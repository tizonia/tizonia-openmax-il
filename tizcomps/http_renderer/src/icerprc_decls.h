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
#include "icercon.h"
#include "tizproc_decls.h"


  struct icerprc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_STRING bind_address_;   /* if this is null, the server will listen on all
                                 * interfaces. */
    OMX_U32 lstn_port_;
    OMX_STRING mount_name_;
    OMX_U32 max_clients_;
    OMX_U32 burst_size_;
    bool server_is_full_;
    bool eos_;
    int lstn_sockfd_;
    icer_server_t *p_server_;
    OMX_BUFFERHEADERTYPE *p_inhdr_;
  };

#ifdef __cplusplus
}
#endif

#endif                          /* ICERPRC_DECLS_H */

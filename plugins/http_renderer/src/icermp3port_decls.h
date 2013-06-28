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
 * @file   icermp3port_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Http renderer mp3 input port class decls
 *
 *
 */

#ifndef ICERMP3PORT_DECLS_H
#define ICERMP3PORT_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Types.h"
#include "OMX_TizoniaExt.h"
#include "tizmp3port_decls.h"

  typedef struct icer_mp3port icer_mp3port_t;
  struct icer_mp3port
  {
    /* Object */
    const tiz_mp3port_t               _;
    OMX_TIZONIA_ICECASTMOUNTPOINTTYPE mountpoint_;
    OMX_TIZONIA_ICECASTMETADATATYPE   metadata_;
    OMX_STRING p_stream_title_;
  };

#ifdef __cplusplus
}
#endif

#endif                          /* ICERMP3PORT_DECLS_H */

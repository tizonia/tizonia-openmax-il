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
 * @file   opusdprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - Opus decoder processor class decls
 * 
 * 
 */

#ifndef OPUSDPRC_DECLS_H
#define OPUSDPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "opusdprc.h"
#include "tizproc_decls.h"

#include "OMX_Core.h"

#include <stdbool.h>
#include <opus.h>

  typedef struct opusd_prc opusd_prc_t;
  struct opusd_prc
  {
    /* Object */
    const tiz_prc_t _;
    OpusDecoder *p_opus_dec_;
    OMX_BUFFERHEADERTYPE *pinhdr_;
    OMX_BUFFERHEADERTYPE *pouthdr_;
    bool eos_;
  };

#ifdef __cplusplus
}
#endif

#endif                          /* OPUSDPRC_DECLS_H */

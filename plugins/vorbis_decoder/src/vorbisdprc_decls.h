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
 * @file   vorbisdprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - Vorbis Decoder processor class decls
 * 
 * 
 */

#ifndef VORBISDPRC_DECLS_H
#define VORBISDPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "vorbisdprc.h"
#include "tizprc_decls.h"

#include <stdbool.h>
#include <fishsound/fishsound.h>

  typedef struct vorbisd_prc vorbisd_prc_t;
  struct vorbisd_prc
  {
    /* Object */
    const tiz_prc_t _;
    void *p_opus_dec_;
    OMX_BUFFERHEADERTYPE *p_in_hdr_;
    OMX_BUFFERHEADERTYPE *p_out_hdr_;
    FishSound *p_fsnd_;
    int packet_count_;
    int rate_;
    int channels_;
    bool eos_;
    bool in_port_disabled_;
    bool out_port_disabled_;
  };

  typedef struct vorbisd_prc_class vorbisd_prc_class_t;
  struct vorbisd_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* VORBISDPRC_DECLS_H */

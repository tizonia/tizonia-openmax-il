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
 * @file   flacdprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - FLAC Decoder processor class decls
 * 
 * 
 */

#ifndef FLACDPRC_DECLS_H
#define FLACDPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "flacdprc.h"
#include "tizprc_decls.h"

#include <stdbool.h>
#include <FLAC/all.h>                /* flac header */

  typedef struct flacd_prc flacd_prc_t;
  struct flacd_prc
  {
    /* Object */
    const tiz_prc_t   _;
    FLAC__StreamDecoder *p_flac_dec_;
    OMX_BUFFERHEADERTYPE *p_in_hdr_;
    OMX_BUFFERHEADERTYPE *p_out_hdr_;
    bool eos_;
    bool in_port_disabled_;
    bool out_port_disabled_;
    FLAC__uint64 total_samples_;
    unsigned sample_rate_;
    unsigned channels_;
    unsigned bps_;
    OMX_U8 *p_store_;
    OMX_U32 store_offset_;
    OMX_U32 store_size_;
  };

  typedef struct flacd_prc_class flacd_prc_class_t;
  struct flacd_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* FLACDPRC_DECLS_H */

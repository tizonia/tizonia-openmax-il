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
 * @file   vp8dprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - VP8 Decoder processor class decls
 * 
 * 
 */

#ifndef VP8DPRC_DECLS_H
#define VP8DPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include "OMX_Core.h"

#include "vp8dprc.h"
#include "tizproc_decls.h"

#define VPX_CODEC_DISABLE_COMPAT 1
#include <vpx_decoder.h>
#include <vp8dx.h>

  typedef enum vp8dprc_stream_type vp8dprc_stream_type_t;

  enum vp8dprc_stream_type
  {
    STREAM_RAW,
    STREAM_IVF
  };

  struct vp8dprc
  {
    /* Object */
    const struct tizproc _;
    OMX_BUFFERHEADERTYPE *p_inhdr_;
    OMX_BUFFERHEADERTYPE *p_outhdr_;
    vpx_codec_ctx_t vp8ctx_;
    bool first_buf_;
    bool eos_;
    vp8dprc_stream_type_t stream_type_;
    uint8_t *p_cbuf_;
    size_t cbuf_sz_;
    size_t cbuf_alloc_sz_;
    size_t cbuf_read_sz_;
  };

#ifdef __cplusplus
}
#endif

#endif                          /* VP8DPRC_DECLS_H */

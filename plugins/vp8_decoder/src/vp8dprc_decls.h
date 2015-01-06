/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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

#define VPX_CODEC_DISABLE_COMPAT 1
#include <vpx_decoder.h>
#include <vp8dx.h>

#include <tizprc_decls.h>

  typedef enum vp8dprc_stream_type vp8dprc_stream_type_t;
  enum vp8dprc_stream_type
  {
    STREAM_RAW,
    STREAM_IVF
  };

  typedef struct vp8d_codec_buffer vp8d_codec_buffer_t;
  struct vp8d_codec_buffer
  {
    uint8_t *p_data;
    size_t  frame_size;
    size_t  filled_len;
    size_t  alloc_len;
  };

  typedef struct vp8d_prc vp8d_prc_t;
  struct vp8d_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_BUFFERHEADERTYPE *p_inhdr_;
    OMX_BUFFERHEADERTYPE *p_outhdr_;
    vpx_codec_ctx_t vp8ctx_;
    bool first_buf_;
    bool eos_;
    vp8dprc_stream_type_t stream_type_;
    vp8d_codec_buffer_t codec_buf_;
  };

  typedef struct vp8d_prc_class vp8d_prc_class_t;
  struct vp8d_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* VP8DPRC_DECLS_H */

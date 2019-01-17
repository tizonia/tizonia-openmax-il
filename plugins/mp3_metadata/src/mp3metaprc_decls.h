/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   mp3metaprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Mp3 Metadata Eraser processor class decls
 *
 *
 */

#ifndef MP3METAPRC_DECLS_H
#define MP3METAPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <mpg123.h>

#include <tizprc_decls.h>

#include "mp3metaprc.h"

typedef struct mp3meta_prc mp3meta_prc_t;
struct mp3meta_prc
{
  /* Object */
  const tiz_prc_t _;
  mpg123_handle *p_mpg123_;
  OMX_BUFFERHEADERTYPE *p_out_hdr_;
  OMX_PARAM_CONTENTURITYPE *p_uri_param_;
  OMX_U32 counter_;
  bool eos_;
  bool out_port_disabled_;
};

typedef struct mp3meta_prc_class mp3meta_prc_class_t;
struct mp3meta_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* MP3METAPRC_DECLS_H */

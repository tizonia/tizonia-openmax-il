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
 * @file   sndfiledprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Sampled sound file decoder - processor class decls
 *
 *
 */

#ifndef SNDFILEDPRC_DECLS_H
#define SNDFILEDPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <sndfile.h>

#include <tizfilterprc.h>
#include <tizfilterprc_decls.h>

typedef struct sndfiled_prc sndfiled_prc_t;
struct sndfiled_prc
{
  /* Object */
  const tiz_filter_prc_t _;
  SNDFILE *p_sf_;
  SF_INFO sf_info_;
  SF_VIRTUAL_IO sf_io_;
  bool decoder_inited_;
  tiz_buffer_t *p_store_;
  OMX_U32 store_offset_;
};

typedef struct sndfiled_prc_class sndfiled_prc_class_t;
struct sndfiled_prc_class
{
  /* Class */
  const tiz_filter_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* SNDFILEDPRC_DECLS_H */

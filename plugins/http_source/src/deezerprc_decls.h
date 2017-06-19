/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   deezerprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Deezer client - processor declarations
 *
 *
 */

#ifndef DEEZERPRC_DECLS_H
#define DEEZERPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

#include <tizplatform.h>
#include <tizdeezer_c.h>

typedef struct deezer_prc deezer_prc_t;
struct deezer_prc
{
  /* Object */
  const tiz_prc_t _;
  OMX_BUFFERHEADERTYPE * p_outhdr_;
  OMX_TIZONIA_AUDIO_PARAM_DEEZERSESSIONTYPE session_;
  OMX_TIZONIA_AUDIO_PARAM_DEEZERPLAYLISTTYPE playlist_;
  OMX_TIZONIA_PLAYLISTSKIPTYPE playlist_skip_;
  OMX_PARAM_CONTENTURITYPE * p_uri_param_;
  tiz_deezer_t * p_deezer_;
  unsigned char * p_deezer_data_;
  size_t deezer_data_offset_;
  size_t deezer_data_len_;
  bool eos_;
  bool port_disabled_;
  bool uri_changed_;
  bool pause_needed_;
  OMX_S32 audio_coding_type_;
  OMX_U32 content_length_bytes_;
  OMX_U32 bytes_before_eos_;
  bool auto_detect_on_;
};

typedef struct deezer_prc_class deezer_prc_class_t;
struct deezer_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* DEEZERPRC_DECLS_H */

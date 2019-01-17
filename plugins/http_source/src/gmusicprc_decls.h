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
 * @file   gmusicprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Google Music client - processor declarations
 *
 *
 */

#ifndef GMUSICPRC_DECLS_H
#define GMUSICPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>
#include <tizgmusic_c.h>

#include <tizplatform.h>

typedef struct gmusic_prc gmusic_prc_t;
struct gmusic_prc
{
  /* Object */
  const tiz_prc_t _;
  OMX_BUFFERHEADERTYPE * p_outhdr_;
  OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE session_;
  OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE playlist_;
  OMX_TIZONIA_PLAYLISTSKIPTYPE playlist_skip_;
  OMX_PARAM_CONTENTURITYPE * p_uri_param_;
  tiz_urltrans_t * p_trans_;
  tiz_gmusic_t * p_gmusic_;
  bool eos_;
  bool port_disabled_;
  bool uri_changed_;
  OMX_S32 audio_coding_type_;
  OMX_U32 num_channels_;
  OMX_U32 samplerate_;
  OMX_U32 content_length_bytes_;
  OMX_U32 bytes_before_eos_;
  bool auto_detect_on_;
  int bitrate_;
  int cache_bytes_;
  bool connection_closed_;
};

typedef struct gmusic_prc_class gmusic_prc_class_t;
struct gmusic_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* GMUSICPRC_DECLS_H */

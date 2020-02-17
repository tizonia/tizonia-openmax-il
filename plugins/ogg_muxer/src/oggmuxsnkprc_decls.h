/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   oggmuxsnkprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Ogg muxer sink processor declarations
 *
 *
 */

#ifndef OGGDMUXSRCPRC_DECLS_H
#define OGGDMUXSRCPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

#include <tizplatform.h>

#include <oggz/oggz.h>

typedef struct oggmuxsnk_prc oggmuxsnk_prc_t;
struct oggmuxsnk_prc
{
  /* Object */
  const tiz_prc_t _;
  tiz_buffer_t * p_audio_store_;
  tiz_buffer_t * p_video_store_;
  FILE * p_file_;
  OMX_PARAM_CONTENTURITYPE * p_uri_;
  OGGZ * p_oggz_;
  long oggz_audio_serialno_;
  long oggz_video_serialno_;
  long oggz_audio_granulepos_;
  long oggz_video_granulepos_;
  long oggz_audio_packetno_;
  long oggz_video_packetno_;
  bool eos_;
  bool port_disabled_;
  bool uri_changed_;
  tiz_urltrans_t * p_trans_;
  int bitrate_;
  int cache_bytes_;
};

typedef struct oggmuxsnk_prc_class oggmuxsnk_prc_class_t;
struct oggmuxsnk_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* OGGDMUXSRCPRC_DECLS_H */

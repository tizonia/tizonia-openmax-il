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
 * @file   opusfiledprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Opus Decoder (libopusfile-based) processor declarations
 *
 *
 */

#ifndef OPUSDV2PRC_DECLS_H
#define OPUSDV2PRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <opusfile.h>

#include <OMX_Core.h>
#include <tizplatform.h>

#include <tizfilterprc.h>
#include <tizfilterprc_decls.h>

typedef struct opusfiled_prc opusfiled_prc_t;
struct opusfiled_prc
{
  /* Object */
  const tiz_filter_prc_t _;
  OggOpusFile * p_opus_dec_;
  bool decoder_inited_;
  tiz_buffer_t * p_store_;
  OMX_U32 store_offset_;
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode_;
};

typedef struct opusfiled_prc_class opusfiled_prc_class_t;
struct opusfiled_prc_class
{
  /* Class */
  const tiz_filter_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* OPUSDV2PRC_DECLS_H */

/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   opusdprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Opus decoder processor class decls
 *
 *
 */

#ifndef OPUSDPRC_DECLS_H
#define OPUSDPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <opus.h>
#include <opus_multistream.h>

#include <tizprc_decls.h>

typedef struct opusd_prc opusd_prc_t;
struct opusd_prc
{
  /* Object */
  const tiz_prc_t _;
  OpusMSDecoder * p_opus_dec_;
  OMX_BUFFERHEADERTYPE * p_in_hdr_;
  OMX_BUFFERHEADERTYPE * p_out_hdr_;
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode_;
  float * p_out_buf_;
  opus_int64 packet_count_;
  int rate_;
  int mapping_family_;
  int channels_;
  int preskip_;
  bool eos_;
  bool in_port_disabled_;
  bool out_port_disabled_;
  bool opus_header_parsed_;
  bool opus_comments_parsed_;
};

typedef struct opusd_prc_class opusd_prc_class_t;
struct opusd_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* OPUSDPRC_DECLS_H */

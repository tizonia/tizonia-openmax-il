/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Tizonia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   aacdecprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - AAC Decoder processor class decls
 *
 *
 */

#ifndef AACDECPRC_DECLS_H
#define AACDECPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <neaacdec.h>

#include <tizplatform.h>

#include <tizfilterprc.h>
#include <tizfilterprc_decls.h>

typedef struct aacdec_prc aacdec_prc_t;
struct aacdec_prc
{
  /* Object */
  const tiz_filter_prc_t _;
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode_;
  NeAACDecHandle p_aac_dec_;
  NeAACDecFrameInfo aac_info_;
  unsigned long samplerate_;
  unsigned char channels_;
  long nbytes_read_;
  bool first_buffer_read_;
  tiz_buffer_t *p_store_;
};

typedef struct aacdec_prc_class aacdec_prc_class_t;
struct aacdec_prc_class
{
  /* Class */
  const tiz_filter_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* AACDECPRC_DECLS_H */

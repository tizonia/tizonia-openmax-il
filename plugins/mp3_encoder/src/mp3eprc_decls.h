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
 * @file   mp3eprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - Mp3 Encoder processor class decls
 * 
 * 
 */

#ifndef MP3EPRC_DECLS_H
#define MP3EPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mp3eprc.h"
#include "tizprc_decls.h"

#include "OMX_Core.h"

#include <stdbool.h>
#include <lame/lame.h>

#define INPUT_BUFFER_SIZE   (5*8192)
#define OUTPUT_BUFFER_SIZE  8192        /* Must be an integer multiple of 4. */

  typedef struct mp3e_prc mp3e_prc_t;
  struct mp3e_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_AUDIO_PARAM_PCMMODETYPE pcmmode_;
    OMX_AUDIO_PARAM_MP3TYPE mp3type_;
    lame_t lame_;
    int frame_size_;
    OMX_BUFFERHEADERTYPE *p_inhdr_;
    OMX_BUFFERHEADERTYPE *p_outhdr_;
    bool eos_;
    bool lame_flushed_;
  };

  typedef struct mp3e_prc_class mp3e_prc_class_t;
  struct mp3e_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* MP3EPRC_DECLS_H */

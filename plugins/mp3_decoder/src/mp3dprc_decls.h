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
 * @file   mp3dprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Tizonia OpenMAX IL - Mp3 Decoder processor class decls
 * 
 * 
 */

#ifndef MP3DPRC_DECLS_H
#define MP3DPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mp3dprc.h"
#include "tizprc_decls.h"

#include "OMX_Core.h"

#include <stdbool.h>
#include <mad.h>

#define INPUT_BUFFER_SIZE   (5*8192)
#define OUTPUT_BUFFER_SIZE  8192        /* Must be an integer multiple of 4. */

  typedef struct mp3d_prc mp3d_prc_t;
  struct mp3d_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_AUDIO_PARAM_PCMMODETYPE pcmmode_;
    struct mad_stream stream_;
    struct mad_frame frame_;
    struct mad_synth synth_;
    size_t remaining_;
    mad_timer_t timer_;
    unsigned long frame_count_;
    unsigned char in_buff_[INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD];
    unsigned char out_buff_[OUTPUT_BUFFER_SIZE];
    OMX_BUFFERHEADERTYPE *p_inhdr_;
    OMX_BUFFERHEADERTYPE *p_outhdr_;
    int next_synth_sample_;
    bool eos_;
  };

  typedef struct mp3d_prc_class mp3d_prc_class_t;
  struct mp3d_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* MP3DPRC_DECLS_H */

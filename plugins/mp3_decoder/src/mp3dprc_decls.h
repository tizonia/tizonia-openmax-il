/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

#include <stdbool.h>
#include <mad.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

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
    OMX_BUFFERHEADERTYPE *p_inhdr_;
    OMX_BUFFERHEADERTYPE *p_outhdr_;
    int next_synth_sample_;
    bool eos_;
    bool in_port_disabled_;
    bool out_port_disabled_;
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

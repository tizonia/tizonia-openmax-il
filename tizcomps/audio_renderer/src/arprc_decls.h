/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   arprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - PCM Audio Renderer processor class decls
 *
 *
 */

#ifndef ARPRC_DECLS_H
#define ARPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"
#include "arprc.h"
#include "tizproc_decls.h"

#include <alsa/asoundlib.h>

  typedef struct ar_prc ar_prc_t;
  struct ar_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
    snd_pcm_t *p_playback_hdl;
    snd_pcm_hw_params_t *p_hw_params;
    char *p_alsa_pcm_;
  };

#ifdef __cplusplus
}
#endif

#endif                          /* ARPRC_DECLS_H */

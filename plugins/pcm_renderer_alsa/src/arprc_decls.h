/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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

#include <stdbool.h>
#include <alsa/asoundlib.h>
#include <poll.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

  typedef struct ar_prc ar_prc_t;
  struct ar_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
    snd_pcm_t *p_pcm_;
    snd_pcm_hw_params_t *p_hw_params_;
    char *p_pcm_name_;
    char *p_mixer_name_;
    bool swap_byte_order_;
    int descriptor_count_;
    struct pollfd *p_fds_;
    tiz_event_io_t *p_ev_io_;
    tiz_event_timer_t *p_ev_timer_;
    OMX_BUFFERHEADERTYPE *p_inhdr_;
    bool port_disabled_;
    bool awaiting_io_ev_;
    float gain_;
    long volume_;
    long ramp_step_;
    long ramp_step_count_;
    long ramp_volume_;
  };

  typedef struct ar_prc_class ar_prc_class_t;
  struct ar_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif                          /* ARPRC_DECLS_H */

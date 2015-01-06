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
 * @file   pulsearprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - PCM audio renderer based on pulseaudio processor
 *declarations
 *
 *
 */

#ifndef PULSEARPRC_DECLS_H
#define PULSEARPRC_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/stream.h>
#include <pulse/introspect.h>
#include <pulse/subscribe.h>
#include <pulse/error.h>
#include <pulse/version.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

typedef struct pulsear_prc pulsear_prc_t;
struct pulsear_prc
{
  /* Object */
  const tiz_prc_t _;
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode_;
  OMX_BUFFERHEADERTYPE *p_inhdr_;
  bool port_disabled_;
  bool paused_;
  bool stopped_;
  struct pa_threaded_mainloop *p_pa_loop_;
  struct pa_context *p_pa_context_;
  struct pa_stream *p_pa_stream_;
  struct pa_cvolume pa_vol_;
  pa_stream_state_t pa_stream_state_;
  size_t pa_nbytes_;
  tiz_event_timer_t *p_ev_timer_;
  float gain_;
  long volume_;
  long ramp_step_;
  long ramp_step_count_;
  long ramp_volume_;
};

typedef struct pulsear_prc_class pulsear_prc_class_t;
struct pulsear_prc_class
{
  /* Class */
  const tiz_prc_class_t _;
  /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* PULSEARPRC_DECLS_H */

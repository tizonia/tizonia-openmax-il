/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   ar.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - ALSA audio renderer component constants
 *
 *
 */
#ifndef AR_H
#define AR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_AUDIO_RENDERER_DEFAULT_ROLE "audio_renderer.pcm"
#define ARATELIA_AUDIO_RENDERER_COMPONENT_NAME \
  "OMX.Aratelia.audio_renderer.alsa.pcm"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_AUDIO_RENDERER_PORT_INDEX 0
#define ARATELIA_AUDIO_RENDERER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_AUDIO_RENDERER_PORT_MIN_BUF_SIZE 1024 * 4
#define ARATELIA_AUDIO_RENDERER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_AUDIO_RENDERER_PORT_ALIGNMENT 0
#define ARATELIA_AUDIO_RENDERER_PORT_SUPPLIERPREF OMX_BufferSupplyInput
#define ARATELIA_AUDIO_RENDERER_MAX_GAIN_VALUE 11.0f   /* 11.00 dB ~ 150% */
#define ARATELIA_AUDIO_RENDERER_MIN_GAIN_VALUE -100.0f /* -100.00 dB ~ 2% */
#define ARATELIA_AUDIO_RENDERER_DEFAULT_GAIN_VALUE 0.f /* 0 dB = 100% */
#define ARATELIA_AUDIO_RENDERER_MAX_VOLUME_VALUE 100
#define ARATELIA_AUDIO_RENDERER_MIN_VOLUME_VALUE 0
#define ARATELIA_AUDIO_RENDERER_DEFAULT_VOLUME_VALUE 75
#define ARATELIA_AUDIO_RENDERER_NULL_ALSA_DEVICE "null"
#define ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_DEVICE \
  ARATELIA_AUDIO_RENDERER_NULL_ALSA_DEVICE
#define ARATELIA_AUDIO_RENDERER_DEFAULT_ALSA_MIXER "Master"

#define ARATELIA_AUDIO_RENDERER_DEFAULT_RAMP_STEP_COUNT 20

#ifdef __cplusplus
}
#endif

#endif /* AR_H */

/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   mp3e.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Mp3 encoder component constants
 *
 *
 */
#ifndef MP3E_H
#define MP3E_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_MP3_ENCODER_DEFAULT_ROLE             "audio_encoder.mp3"
#define ARATELIA_MP3_ENCODER_COMPONENT_NAME           "OMX.Aratelia.audio_encoder.mp3"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_MP3_ENCODER_INPUT_PORT_INDEX         0
#define ARATELIA_MP3_ENCODER_OUTPUT_PORT_INDEX        1
#define ARATELIA_MP3_ENCODER_PORT_MIN_BUF_COUNT       2
/* Assuming worst case of 16 bit per sample per channel adn 48khz, lets try to
   fit 25ms of audio (1200 samples per channel) */
#define ARATELIA_MP3_ENCODER_PORT_MIN_INPUT_BUF_SIZE  (2*4800)
/* output buffer size in bytes = 1.25*num_samples + 7200 */
#define ARATELIA_MP3_ENCODER_PORT_MIN_OUTPUT_BUF_SIZE 10200
#define ARATELIA_MP3_ENCODER_PORT_NONCONTIGUOUS       OMX_FALSE
#define ARATELIA_MP3_ENCODER_PORT_ALIGNMENT           0
#define ARATELIA_MP3_ENCODER_PORT_SUPPLIERPREF        OMX_BufferSupplyInput

#ifdef __cplusplus
}
#endif

#endif                          /* MP3E_H */

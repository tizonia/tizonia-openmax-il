/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   mp3d.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Mp3 decoder component constants
 *
 *
 */

#ifndef MP3D_H
#define MP3D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_MP3_DECODER_DEFAULT_ROLE "audio_decoder.mp3"
#define ARATELIA_MP3_DECODER_COMPONENT_NAME "OMX.Aratelia.audio_decoder.mp3"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_MP3_DECODER_INPUT_PORT_INDEX 0
#define ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX 1
#define ARATELIA_MP3_DECODER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_MP3_DECODER_PORT_MIN_INPUT_BUF_SIZE (5 * 8192)
#define ARATELIA_MP3_DECODER_PORT_MIN_OUTPUT_BUF_SIZE (12 * 8192)
#define ARATELIA_MP3_DECODER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_MP3_DECODER_PORT_ALIGNMENT 0
#define ARATELIA_MP3_DECODER_PORT_SUPPLIERPREF OMX_BufferSupplyInput

#ifdef __cplusplus
}
#endif

#endif /* MP3D_H */

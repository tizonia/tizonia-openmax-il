/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   vp8d.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Vp8 decoder component constants
 *
 *
 */
#ifndef VP8D_H
#define VP8D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_VP8_DECODER_DEFAULT_FRAME_WIDTH 176
#define ARATELIA_VP8_DECODER_DEFAULT_FRAME_HEIGHT 144
#define ARATELIA_VP8_DECODER_DEFAULT_ROLE "video_decoder.vp8"
#define ARATELIA_VP8_DECODER_COMPONENT_NAME "OMX.Aratelia.video_decoder.vp8"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_VP8_DECODER_INPUT_PORT_INDEX 0
#define ARATELIA_VP8_DECODER_OUTPUT_PORT_INDEX 1
#define ARATELIA_VP8_DECODER_PORT_MIN_BUF_COUNT 2
/* 38016 = (width * height) + ((width * height)/2) */
#define ARATELIA_VP8_DECODER_PORT_MIN_INPUT_BUF_SIZE 38016
#define ARATELIA_VP8_DECODER_PORT_MIN_OUTPUT_BUF_SIZE 345600
#define ARATELIA_VP8_DECODER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_VP8_DECODER_PORT_ALIGNMENT 0
#define ARATELIA_VP8_DECODER_PORT_SUPPLIERPREF OMX_BufferSupplyInput

#ifdef __cplusplus
}
#endif

#endif /* VP8D_H */

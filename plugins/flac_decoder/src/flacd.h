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
 * @file   flacd.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Flac decoder component constants
 *
 *
 */
#ifndef FLACD_H
#define FLACD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>
#include <OMX_TizoniaExt.h>

#define ARATELIA_FLAC_DECODER_DEFAULT_ROLE OMX_ROLE_AUDIO_DECODER_FLAC
#define ARATELIA_FLAC_DECODER_COMPONENT_NAME "OMX.Aratelia.audio_decoder.flac"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_FLAC_DECODER_INPUT_PORT_INDEX 0
#define ARATELIA_FLAC_DECODER_OUTPUT_PORT_INDEX 1
#define ARATELIA_FLAC_DECODER_PORT_MIN_BUF_COUNT 10
#define ARATELIA_FLAC_DECODER_PORT_MIN_INPUT_BUF_SIZE 150 * 1024
#define ARATELIA_FLAC_DECODER_PORT_MIN_OUTPUT_BUF_SIZE 8192 * 20
#define ARATELIA_FLAC_DECODER_BUFFER_THRESHOLD     \
  ARATELIA_FLAC_DECODER_PORT_MIN_INPUT_BUF_SIZE *( \
    ARATELIA_FLAC_DECODER_PORT_MIN_BUF_COUNT - 1)
#define ARATELIA_FLAC_DECODER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_FLAC_DECODER_PORT_ALIGNMENT 0
#define ARATELIA_FLAC_DECODER_PORT_SUPPLIERPREF OMX_BufferSupplyInput

#ifdef __cplusplus
}
#endif

#endif /* FLACD_H */

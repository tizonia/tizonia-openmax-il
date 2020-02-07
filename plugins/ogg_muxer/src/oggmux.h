/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   oggmux.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Ogg Muxer component constants
 *
 *
 */
#ifndef OGGDMUX_H
#define OGGDMUX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_OGG_MUXER_COMPONENT_NAME "OMX.Aratelia.container_muxer.ogg"
#define ARATELIA_OGG_MUXER_SINK_ROLE "container_muxer.sink.ogg"
#define ARATELIA_OGG_MUXER_FILTER_ROLE "container_muxer.filter.ogg"

/* Filter role (with libtizonia, port indexes must start at index 0) */
#define ARATELIA_OGG_MUXER_SINK_PORT_0_INDEX 0 /* audio input */
#define ARATELIA_OGG_MUXER_SINK_PORT_1_INDEX 1 /* video input */

/* Filter role  (with libtizonia, port indexes must start at index 0) */
#define ARATELIA_OGG_MUXER_FILTER_PORT_0_INDEX 0 /* audio input */
#define ARATELIA_OGG_MUXER_FILTER_PORT_1_INDEX 1 /* video input */
#define ARATELIA_OGG_MUXER_FILTER_PORT_2_INDEX 2 /* Ogg output port */

/* Filter ogg output port */
#define ARATELIA_OGG_MUXER_OGG_PORT_MIN_BUF_COUNT 2
#define ARATELIA_OGG_MUXER_OGG_PORT_MIN_BUF_SIZE 8192
#define ARATELIA_OGG_MUXER_OGG_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_OGG_MUXER_OGG_PORT_ALIGNMENT 0
#define ARATELIA_OGG_MUXER_OGG_PORT_SUPPLIERPREF OMX_BufferSupplyInput

/* Sink/filter audio input port */
#define ARATELIA_OGG_MUXER_AUDIO_PORT_MIN_BUF_COUNT 2
#define ARATELIA_OGG_MUXER_AUDIO_PORT_MIN_BUF_SIZE 8192
#define ARATELIA_OGG_MUXER_AUDIO_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_OGG_MUXER_AUDIO_PORT_ALIGNMENT 0
#define ARATELIA_OGG_MUXER_AUDIO_PORT_SUPPLIERPREF OMX_BufferSupplyInput

/* Sink/filter video input port */
#define ARATELIA_OGG_MUXER_VIDEO_PORT_MIN_BUF_COUNT 2
#define ARATELIA_OGG_MUXER_VIDEO_PORT_MIN_BUF_SIZE 8192 * 4
#define ARATELIA_OGG_MUXER_VIDEO_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_OGG_MUXER_VIDEO_PORT_ALIGNMENT 0
#define ARATELIA_OGG_MUXER_VIDEO_PORT_SUPPLIERPREF OMX_BufferSupplyInput

/* Sink role - additional configs */
#define ARATELIA_OGG_MUXER_DEFAULT_RECONNECT_TIMEOUT 3.0F
#define ARATELIA_OGG_MUXER_DEFAULT_BIT_RATE_KBITS 128
#define ARATELIA_OGG_MUXER_DEFAULT_CACHE_SECONDS 10

#ifdef __cplusplus
}
#endif

#endif /* OGGDMUX_H */

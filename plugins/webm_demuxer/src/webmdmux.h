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
 * @file   webmdmux.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - WebM Demuxer component constants
 *
 *
 */
#ifndef WEBMDMUX_H
#define WEBMDMUX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_WEBM_DEMUXER_COMPONENT_NAME \
  "OMX.Aratelia.container_demuxer.webm"
#define ARATELIA_WEBM_DEMUXER_SOURCE_ROLE "container_demuxer.source.webm"
#define ARATELIA_WEBM_DEMUXER_FILTER_ROLE "container_demuxer.filter.webm"

/* Filter role (with libtizonia, port indexes must start at index 0) */
#define ARATELIA_WEBM_DEMUXER_SOURCE_PORT_0_INDEX 0 /* Audio output */
#define ARATELIA_WEBM_DEMUXER_SOURCE_PORT_1_INDEX 1 /* Video output */

/* Filter role  (with libtizonia, port indexes must start at index 0) */
#define ARATELIA_WEBM_DEMUXER_FILTER_PORT_0_INDEX 0 /* Webm input port */
#define ARATELIA_WEBM_DEMUXER_FILTER_PORT_1_INDEX 1 /* Audio output */
#define ARATELIA_WEBM_DEMUXER_FILTER_PORT_2_INDEX 2 /* Video output */

/* Filter webm input port */
#define ARATELIA_WEBM_DEMUXER_WEBM_PORT_MIN_BUF_COUNT 2
#define ARATELIA_WEBM_DEMUXER_WEBM_PORT_MIN_BUF_SIZE 8192
#define ARATELIA_WEBM_DEMUXER_WEBM_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_WEBM_DEMUXER_WEBM_PORT_ALIGNMENT 0
#define ARATELIA_WEBM_DEMUXER_WEBM_PORT_SUPPLIERPREF OMX_BufferSupplyInput

/* Source/filter audio output port */
#define ARATELIA_WEBM_DEMUXER_AUDIO_PORT_MIN_BUF_COUNT 2
#define ARATELIA_WEBM_DEMUXER_AUDIO_PORT_MIN_BUF_SIZE 8192
#define ARATELIA_WEBM_DEMUXER_AUDIO_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_WEBM_DEMUXER_AUDIO_PORT_ALIGNMENT 0
#define ARATELIA_WEBM_DEMUXER_AUDIO_PORT_SUPPLIERPREF OMX_BufferSupplyInput

/* Source/filter video output port */
#define ARATELIA_WEBM_DEMUXER_VIDEO_PORT_MIN_BUF_COUNT 2
#define ARATELIA_WEBM_DEMUXER_VIDEO_PORT_MIN_BUF_SIZE 8192 * 4
#define ARATELIA_WEBM_DEMUXER_VIDEO_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_WEBM_DEMUXER_VIDEO_PORT_ALIGNMENT 0
#define ARATELIA_WEBM_DEMUXER_VIDEO_PORT_SUPPLIERPREF OMX_BufferSupplyInput

/* Source role - additional configs */
#define ARATELIA_WEBM_DEMUXER_DEFAULT_RECONNECT_TIMEOUT 3.0F
#define ARATELIA_WEBM_DEMUXER_DEFAULT_BIT_RATE_KBITS 128
#define ARATELIA_WEBM_DEMUXER_DEFAULT_CACHE_SECONDS 10

#ifdef __cplusplus
}
#endif

#endif /* WEBMDMUX_H */

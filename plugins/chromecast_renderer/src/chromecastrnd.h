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
 * along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.
 */

/**
 * @file   chromecastrnd.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Chromecast renderer component constants
 *
 *
 */
#ifndef CHROMECASTSRC_H
#define CHROMECASTSRC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <curl/curl.h>

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_CHROMECAST_RENDERER_DEFAULT_ROLE "audio_renderer.chromecast"
#define ARATELIA_GMUSIC_SOURCE_DEFAULT_ROLE "audio_renderer.chromecast.gmusic"
#define ARATELIA_SCLOUD_SOURCE_DEFAULT_ROLE "audio_renderer.chromecast.scloud"
/* #define ARATELIA_DIRBLE_SOURCE_DEFAULT_ROLE "audio_renderer.chromecast.dirble" */
#define ARATELIA_YOUTUBE_SOURCE_DEFAULT_ROLE "audio_renderer.chromecast.youtube"
#define ARATELIA_PLEX_SOURCE_DEFAULT_ROLE "audio_renderer.chromecast.plex"
#define ARATELIA_CHROMECAST_RENDERER_COMPONENT_NAME \
  "OMX.Aratelia.audio_renderer.chromecast"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_CHROMECAST_RENDERER_PORT_INDEX 0
#define ARATELIA_CHROMECAST_RENDERER_PORT_MIN_BUF_COUNT 2
#define ARATELIA_CHROMECAST_RENDERER_PORT_MIN_BUF_SIZE CURL_MAX_WRITE_SIZE
#define ARATELIA_CHROMECAST_RENDERER_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_CHROMECAST_RENDERER_PORT_ALIGNMENT 0
#define ARATELIA_CHROMECAST_RENDERER_PORT_SUPPLIERPREF OMX_BufferSupplyInput
#define ARATELIA_CHROMECAST_RENDERER_MAX_VOLUME_VALUE 100
#define ARATELIA_CHROMECAST_RENDERER_MIN_VOLUME_VALUE 0
#define ARATELIA_CHROMECAST_RENDERER_DEFAULT_VOLUME_VALUE 20
#define ARATELIA_CHROMECAST_RENDERER_DEFAULT_RECONNECT_TIMEOUT 3.0F
#define ARATELIA_CHROMECAST_RENDERER_DEFAULT_BIT_RATE_KBITS 128
#define ARATELIA_CHROMECAST_RENDERER_DEFAULT_CACHE_SECONDS 10

#ifdef __cplusplus
}
#endif

#endif /* CHROMECASTSRC_H */

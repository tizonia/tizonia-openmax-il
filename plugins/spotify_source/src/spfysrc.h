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
 * @file   spfysrc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Spotify client component constants
 *
 *
 */
#ifndef SPFYSRC_H
#define SPFYSRC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>
#include <OMX_TizoniaExt.h>

#define ARATELIA_SPOTIFY_SOURCE_DEFAULT_ROLE OMX_ROLE_AUDIO_SOURCE_PCM_SPOTIFY
#define ARATELIA_SPOTIFY_SOURCE_COMPONENT_NAME \
  "OMX.Aratelia.audio_source.spotify.pcm"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_SPOTIFY_SOURCE_PORT_INDEX 0
#define ARATELIA_SPOTIFY_SOURCE_PORT_MIN_BUF_COUNT 3
#define ARATELIA_SPOTIFY_SOURCE_PORT_MIN_BUF_SIZE 8192 * 12
#define ARATELIA_SPOTIFY_SOURCE_PORT_NONCONTIGUOUS OMX_FALSE
#define ARATELIA_SPOTIFY_SOURCE_PORT_ALIGNMENT 0
#define ARATELIA_SPOTIFY_SOURCE_PORT_SUPPLIERPREF OMX_BufferSupplyInput
#define ARATELIA_SPOTIFY_SOURCE_DEFAULT_VOLUME_VALUE 75
#define ARATELIA_SPOTIFY_SOURCE_MAX_VOLUME_VALUE 100
#define ARATELIA_SPOTIFY_SOURCE_MIN_VOLUME_VALUE 0
#define ARATELIA_SPOTIFY_SOURCE_DEFAULT_BIT_RATE_KBITS 320
#define ARATELIA_SPOTIFY_SOURCE_DEFAULT_CACHE_SECONDS 6
#define ARATELIA_SPOTIFY_SOURCE_MIN_CACHE_SECONDS 7
#define ARATELIA_SPOTIFY_SOURCE_MAX_CACHE_SECONDS 12

#ifdef __cplusplus
}
#endif

#endif /* SPFYSRC_H */

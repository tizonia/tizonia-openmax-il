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
 * @file   httpsrc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - HTTP streaming client component constants
 *
 *
 */
#ifndef HTTPSRC_H
#define HTTPSRC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <curl/curl.h>

#include <OMX_Core.h>
#include <OMX_Types.h>

#define ARATELIA_HTTP_SOURCE_DEFAULT_ROLE              "audio_source.http"
#define ARATELIA_GMUSIC_SOURCE_DEFAULT_ROLE            "audio_source.http.gmusic"
#define ARATELIA_SCLOUD_SOURCE_DEFAULT_ROLE            "audio_source.http.scloud"
#define ARATELIA_DIRBLE_SOURCE_DEFAULT_ROLE            "audio_source.http.dirble"
#define ARATELIA_HTTP_SOURCE_COMPONENT_NAME            "OMX.Aratelia.audio_source.http"
/* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_HTTP_SOURCE_PORT_INDEX                0
#define ARATELIA_HTTP_SOURCE_PORT_MIN_BUF_COUNT        2
#define ARATELIA_HTTP_SOURCE_PORT_MIN_BUF_SIZE         CURL_MAX_WRITE_SIZE
#define ARATELIA_HTTP_SOURCE_PORT_NONCONTIGUOUS        OMX_FALSE
#define ARATELIA_HTTP_SOURCE_PORT_ALIGNMENT            0
#define ARATELIA_HTTP_SOURCE_PORT_SUPPLIERPREF         OMX_BufferSupplyInput
#define ARATELIA_HTTP_SOURCE_MAX_VOLUME_VALUE          100
#define ARATELIA_HTTP_SOURCE_MIN_VOLUME_VALUE          0
#define ARATELIA_HTTP_SOURCE_DEFAULT_VOLUME_VALUE      75
#define ARATELIA_HTTP_SOURCE_DEFAULT_RECONNECT_TIMEOUT 3.0F
#define ARATELIA_HTTP_SOURCE_DEFAULT_BIT_RATE_KBITS    128
#define ARATELIA_HTTP_SOURCE_DEFAULT_CACHE_SECONDS     10

#ifdef __cplusplus
}
#endif

#endif                          /* HTTPSRC_H */

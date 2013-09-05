/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   oggdmux.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Ogg demuxer component constants
 *
 *
 */
#ifndef OGGDMUX_H
#define OGGDMUX_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OMX_Core.h"
#include "OMX_Types.h"
#include "OMX_TizoniaExt.h"

#define ARATELIA_OGG_DEMUXER_DEFAULT_ROLE                   OMX_ROLE_CONTAINER_DEMUXER_OGG
#define ARATELIA_OGG_DEMUXER_COMPONENT_NAME                 "OMX.Aratelia.container_demuxer.ogg"
  /* With libtizonia, port indexes must start at index 0 */
#define ARATELIA_OGG_DEMUXER_AUDIO_PORT_INDEX               0
#define ARATELIA_OGG_DEMUXER_VIDEO_PORT_INDEX               1
#define ARATELIA_OGG_DEMUXER_PORT_MIN_BUF_COUNT             2
#define ARATELIA_OGG_DEMUXER_PORT_MIN_AUDIO_OUTPUT_BUF_SIZE 8192
#define ARATELIA_OGG_DEMUXER_PORT_MIN_VIDEO_OUTPUT_BUF_SIZE 8192
#define ARATELIA_OGG_DEMUXER_PORT_NONCONTIGUOUS             OMX_FALSE
#define ARATELIA_OGG_DEMUXER_PORT_ALIGNMENT                 0
#define ARATELIA_OGG_DEMUXER_PORT_SUPPLIERPREF              OMX_BufferSupplyInput

#ifdef __cplusplus
}
#endif

#endif                          /* OGGDMUX_H */

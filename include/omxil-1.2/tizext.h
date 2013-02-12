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
 * @file   tizext.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - OpenMAX IL Extensions
 *
 *
 */

#ifndef TIZEXT_H
#define TIZEXT_H

#include "OMX_Types.h"

/**
 * Extension index used to select or deselect the buffer pre-announcements
 * feature on a particular port.
 */
#define OMX_TizoniaIndexParamBufferPreAnnouncementsMode 0x7F000001

/** 
 * The name of the pre-announcements mode extension.
 */
#define OMX_TIZONIA_INDEX_PARAM_BUFFER_PREANNOUNCEMENTSMODE     \
  "OMX.Tizonia.index.param.preannouncementsmode"

typedef struct OMX_TIZONIA_PARAM_BUFFER_PREANNOUNCEMENTSMODETYPE
{
  OMX_U32 nSize;
  OMX_VERSIONTYPE nVersion;
  OMX_U32 nPortIndex;
  OMX_BOOL bEnabled;
} OMX_TIZONIA_PARAM_BUFFER_PREANNOUNCEMENTSMODETYPE;


/* Shoutcast Renderer */

#define OMX_ROLE_AUDIO_RENDERER_SHOUTCAST_MP3   "audio_renderer.shoutcast.mp3"
#define OMX_ROLE_AUDIO_RENDERER_SHOUTCAST_VORBIS   "audio_renderer.shoutcast.vorbis"

typedef struct OMX_AUDIO_PARAM_SHOUTCASTHTTPINFOTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nServerPort;
    OMX_U32 nMaxListeners;
} OMX_AUDIO_PARAM_SHOUTCASTHTTPINFOTYPE;

typedef struct OMX_AUDIO_CONFIG_SHOUTCASTMETADATATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIcyMetadataPeriod;
    OMX_U32 nMetadataSize;
    OMX_U8 nMetadata[1];
} OMX_AUDIO_CONFIG_SHOUTCASTMETADATATYPE;


#endif /* TIZEXT_H */

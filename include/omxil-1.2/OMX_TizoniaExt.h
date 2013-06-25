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
 * @file   OMX_TizoniaExt.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - OpenMAX IL Extensions
 *
 *
 */

#ifndef OMX_TizoniaExt_h
#define OMX_TizoniaExt_h

#include "OMX_Core.h"
#include "OMX_Types.h"
#include "OMX_Audio.h"

#define OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN   0x00000004


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

/* Definitions for icecast-like source and sink components */
#define OMX_ROLE_AUDIO_RENDERER_ICECAST_MP3   "audio_renderer.icecast.mp3"
#define OMX_ROLE_AUDIO_RENDERER_ICECAST_VORBIS   "audio_renderer.icecast.vorbis"

#define OMX_TizoniaIndexParamHttpServer 0x7F000002           /**< reference: OMX_TIZONIA_HTTPSERVERTYPE */
#define OMX_TizoniaIndexParamIcecastMountpoint 0x7F000003    /**< reference: OMX_TIZONIA_ICECASTMOUNTPOINTTYPE */
#define OMX_TizoniaIndexConfigIcecastMetadata 0x7F000004     /**< reference: OMX_TIZONIA_ICECASTMETADATATYPE */

typedef struct OMX_TIZONIA_HTTPSERVERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_STRING cBindAddress[OMX_MAX_STRINGNAME_SIZE];
    OMX_U32 nListeningPort;
    OMX_U32 nMaxClients;
} OMX_TIZONIA_HTTPSERVERTYPE;

typedef struct OMX_TIZONIA_ICECASTMOUNTPOINTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_STRING cMountName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U32 nIcyMetadataPeriod;
    OMX_BOOL bBurstOnConnect;
    OMX_U32 nBurstSize;
    OMX_U32 nMaxClients;
    OMX_AUDIO_CODINGTYPE eEncoding;
    OMX_STRING cStationName[OMX_MAX_STRINGNAME_SIZE];
    OMX_STRING cStationDescription[OMX_MAX_STRINGNAME_SIZE];
    OMX_STRING cStationGenre[OMX_MAX_STRINGNAME_SIZE];
    OMX_STRING cStationUrl[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_ICECASTMOUNTPOINTTYPE;

typedef struct OMX_TIZONIA_ICECASTMETADATATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_STRING cStreamTitle[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_ICECASTMETADATATYPE;

#endif /* OMX_TizoniaExt_h */

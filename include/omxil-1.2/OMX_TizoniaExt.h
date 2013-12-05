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

#define OMX_ROLE_AUDIO_ENCODER_OPUS            "audio_encoder.opus"
#define OMX_ROLE_AUDIO_DECODER_OPUS            "audio_decoder.opus"
#define OMX_ROLE_AUDIO_ENCODER_FLAC            "audio_encoder.flac"
#define OMX_ROLE_AUDIO_DECODER_FLAC            "audio_decoder.flac"
#define OMX_ROLE_CONTAINER_DEMUXER_OGG         "container_demuxer.ogg"
#define OMX_ROLE_AUDIO_RENDERER_ICECAST_MP3    "audio_renderer.icecast.mp3"
#define OMX_ROLE_AUDIO_RENDERER_ICECAST_VORBIS "audio_renderer.icecast.vorbis"
#define OMX_ROLE_AUDIO_RENDERER_ICECAST_OPUS   "audio_renderer.icecast.opus"


#define OMX_TIZONIA_PORTSTATUS_AWAITBUFFERSRETURN   0x00000004

/**
 * OMX_TizoniaIndexParamBufferPreAnnouncementsMode
 *
 * Extension index used to select or deselect the buffer pre-announcements
 * feature on a particular port.
 */
#define OMX_TizoniaIndexParamBufferPreAnnouncementsMode OMX_IndexVendorStartUnused + 1

#define OMX_TizoniaIndexParamHttpServer        OMX_IndexVendorStartUnused + 2 /**< reference: OMX_TIZONIA_HTTPSERVERTYPE */
#define OMX_TizoniaIndexParamIcecastMountpoint OMX_IndexVendorStartUnused + 3 /**< reference: OMX_TIZONIA_ICECASTMOUNTPOINTTYPE */
#define OMX_TizoniaIndexConfigIcecastMetadata  OMX_IndexVendorStartUnused + 4 /**< reference: OMX_TIZONIA_ICECASTMETADATATYPE */
#define OMX_TizoniaIndexParamAudioOpus         OMX_IndexVendorStartUnused + 5 /**< reference: OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE */
#define OMX_TizoniaIndexParamAudioFlac         OMX_IndexVendorStartUnused + 6 /**< reference: OMX_TIZONIA_AUDIO_PARAM_FLACTYPE */


/**
 * OMX_AUDIO_CODINGTYPE extensions
 */

#define OMX_AUDIO_CodingOPUS OMX_AUDIO_CodingVendorStartUnused + 1
#define OMX_AUDIO_CodingFLAC OMX_AUDIO_CodingVendorStartUnused + 2

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

/**
 * Icecast-like audio renderer components
 */

typedef struct OMX_TIZONIA_HTTPSERVERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_STRING cBindAddress;
    OMX_U32 nListeningPort;
    OMX_U32 nMaxClients;
} OMX_TIZONIA_HTTPSERVERTYPE;

typedef struct OMX_TIZONIA_ICECASTMOUNTPOINTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U8 cMountName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cStationName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cStationDescription[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cStationGenre[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cStationUrl[OMX_MAX_STRINGNAME_SIZE];
    OMX_AUDIO_CODINGTYPE eEncoding;
    OMX_U32 nIcyMetadataPeriod;
    OMX_BOOL bBurstOnConnect;
    OMX_U32 nInitialBurstSize;
    OMX_U32 nMaxClients;
} OMX_TIZONIA_ICECASTMOUNTPOINTTYPE;

#define OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE OMX_MAX_STRINGNAME_SIZE

typedef struct OMX_TIZONIA_ICECASTMETADATATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U8 cStreamTitle[1];     /* Max length is OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE */
} OMX_TIZONIA_ICECASTMETADATATYPE;

/**
 * Opus encoder/decoder components
 * References:
 * - http://tools.ietf.org/html/rfc6716
 * - http://www.opus-codec.org/
 *
 * Description: (from opus-codec.org)
 *     Bit-rates from 6 kb/s to 510 kb/s
 *     Sampling rates from 8 kHz (narrowband) to 48 kHz (fullband)
 *     Frame sizes from 2.5 ms to 60 ms
 *     Support for both constant bit-rate (CBR) and variable bit-rate (VBR)
 *     Audio bandwidth from narrowband to fullband
 *     Support for speech and music
 *     Support for mono and stereo
 *     Support for up to 255 channels (multistream frames)
 *     Dynamically adjustable bitrate, audio bandwidth, and frame size
 *     Good loss robustness and packet loss concealment (PLC)
 *     Floating point and fixed-point implementation
 */

typedef enum OMX_TIZONIA_AUDIO_OPUSSTREAMFORMATTYPE {
    OMX_AUDIO_OPUSStreamFormatVBR = 0,
    OMX_AUDIO_OPUSStreamFormatCBR,
    OMX_AUDIO_OPUSStreamFormatConstrainedVBR,
    OMX_AUDIO_OPUSStreamFormatHardCBR,
    OMX_AUDIO_OPUSStreamFormatUnknown           = 0x6EFFFFFF,
    OMX_AUDIO_OPUSStreamFormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_OPUSStreamFormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_OPUSStreamFormatMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_OPUSSTREAMFORMATTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nBitRate; /** 6-256 per channel */
    OMX_U32 nSampleRate; /** 8 kHz (narrowband) to 48 kHz (fullband) */
    OMX_S32 nFrameDuration; /** 2.5, 5, 10, 20, 40, or 60 ms. */
    OMX_U32 nEncoderComplexity; /** From 0 to 10, default 0 */
    OMX_BOOL bPacketLossResilience; /** default, false  */
    OMX_BOOL bForwardErrorCorrection; /** default, false  */
    OMX_BOOL bDtx; /** default, false  */
    OMX_AUDIO_CHANNELMODETYPE eChannelMode;
    OMX_TIZONIA_AUDIO_OPUSSTREAMFORMATTYPE eFormat;
} OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE;

/**
 * FLAC encoder/decoder components
 * References:
 * - https://xiph.org/flac/
 */

/* TODO: Add a struct to configure the following encoding parameters:
 * - do_mid_side_stereo
 * - loose_mid_side_stereo
 * - apodization
 * - max_lpc_order
 * - qlp_coeff_precision
 * - do_qlp_coeff_prec_search
 * - do_escape_coding
 * - do_exhaustive_model_search
 * - min_residual_partition_order
 * - max_residual_partition_order
 * - rice_parameter_search_dist
 *
 * NOTE: nCompressionLevel: is a convenience parameter that sets all of the above.
 *
 */

/*
 * TODO: Add a struct to set FLAC metadata
 */

typedef struct OMX_TIZONIA_AUDIO_PARAM_FLACTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels; /** Up to 8 channels. Default 2.  */
    OMX_U32 nBitsPerSample; /** 4-32 bits per sample. Default 16. */
    OMX_U32 nSampleRate; /** From 1Hz to 655350Hz. Default 44100Hz */
    OMX_U32 nCompressionLevel; /** From 0 (fastest, least compression) to 8
                                   (slowest, most compression). Default 5. For
                                   more info see
                                   https://xiph.org/flac/api/group__flac__stream__encoder.html#ga20*/
    OMX_U32 nBlockSize; /** Block size in samples. From 16 to 65535. Default 0
                            (let encoder estimate blocksize). */
    OMX_U64 nTotalSamplesEstimate; /** An estimate of the total samples that
                                       will be encoded. Set to 0 if unknown. */
    OMX_AUDIO_CHANNELMODETYPE eChannelMode;
} OMX_TIZONIA_AUDIO_PARAM_FLACTYPE;

#endif /* OMX_TizoniaExt_h */

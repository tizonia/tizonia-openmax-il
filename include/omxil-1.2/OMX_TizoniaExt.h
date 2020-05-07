/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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

#define OMX_ROLE_AUDIO_SOURCE_PCM_SPOTIFY      "audio_source.pcm.spotify"
#define OMX_ROLE_AUDIO_ENCODER_OPUS            "audio_encoder.opus"
#define OMX_ROLE_AUDIO_DECODER_OPUS            "audio_decoder.opus"
#define OMX_ROLE_AUDIO_ENCODER_FLAC            "audio_encoder.flac"
#define OMX_ROLE_AUDIO_DECODER_FLAC            "audio_decoder.flac"
#define OMX_ROLE_SOURCE_CONTAINER_DEMUXER_OGG  "source.container_demuxer.ogg"
#define OMX_ROLE_FILTER_CONTAINER_DEMUXER_OGG  "filter.container_demuxer.ogg"
#define OMX_ROLE_AUDIO_RENDERER_ICECAST_MP3    "audio_renderer.icecast.mp3"
#define OMX_ROLE_AUDIO_RENDERER_ICECAST_AAC    "audio_renderer.icecast.aac"
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

#define OMX_TizoniaIndexParamHttpServer              OMX_IndexVendorStartUnused + 2 /**< reference: OMX_TIZONIA_HTTPSERVERTYPE */
#define OMX_TizoniaIndexParamIcecastMountpoint       OMX_IndexVendorStartUnused + 3 /**< reference: OMX_TIZONIA_ICECASTMOUNTPOINTTYPE */
#define OMX_TizoniaIndexConfigIcecastMetadata        OMX_IndexVendorStartUnused + 4 /**< reference: OMX_TIZONIA_ICECASTMETADATATYPE */
#define OMX_TizoniaIndexParamAudioOpus               OMX_IndexVendorStartUnused + 5 /**< reference: OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE */
#define OMX_TizoniaIndexParamAudioFlac               OMX_IndexVendorStartUnused + 6 /**< reference: OMX_TIZONIA_AUDIO_PARAM_FLACTYPE */
#define OMX_TizoniaIndexParamAudioMp2                OMX_IndexVendorStartUnused + 7 /**< reference: OMX_TIZONIA_AUDIO_PARAM_MP2TYPE */
#define OMX_TizoniaIndexParamAudioSpotifySession     OMX_IndexVendorStartUnused + 8 /**< reference: OMX_TIZONIA_AUDIO_PARAM_SPOTIFYSESSIONTYPE */
#define OMX_TizoniaIndexParamAudioSpotifyPlaylist    OMX_IndexVendorStartUnused + 9 /**< reference: OMX_TIZONIA_AUDIO_PARAM_SPOTIFYPLAYLISTTYPE */
#define OMX_TizoniaIndexParamAudioGmusicSession      OMX_IndexVendorStartUnused + 10 /**< reference: OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE */
#define OMX_TizoniaIndexParamAudioGmusicPlaylist     OMX_IndexVendorStartUnused + 11 /**< reference: OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE */
#define OMX_TizoniaIndexConfigPlaylistSkip           OMX_IndexVendorStartUnused + 12 /**< reference: OMX_TIZONIA_PLAYLISTSKIPTYPE */
#define OMX_TizoniaIndexParamAudioSoundCloudSession  OMX_IndexVendorStartUnused + 13 /**< reference: OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDSESSIONTYPE */
#define OMX_TizoniaIndexParamAudioSoundCloudPlaylist OMX_IndexVendorStartUnused + 14 /**< reference: OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDPLAYLISTTYPE */
#define OMX_TizoniaIndexParamAudioTuneinSession      OMX_IndexVendorStartUnused + 15 /**< reference: OMX_TIZONIA_AUDIO_PARAM_TUNEINSESSIONTYPE */
#define OMX_TizoniaIndexParamAudioTuneinPlaylist     OMX_IndexVendorStartUnused + 16 /**< reference: OMX_TIZONIA_AUDIO_PARAM_TUNEINPLAYLISTTYPE */
#define OMX_TizoniaIndexParamAudioYoutubeSession     OMX_IndexVendorStartUnused + 17 /**< reference: OMX_TIZONIA_AUDIO_PARAM_YOUTUBESESSIONTYPE */
#define OMX_TizoniaIndexParamAudioYoutubePlaylist    OMX_IndexVendorStartUnused + 18 /**< reference: OMX_TIZONIA_AUDIO_PARAM_YOUTUBEPLAYLISTTYPE */
#define OMX_TizoniaIndexParamAudioDeezerSession      OMX_IndexVendorStartUnused + 19 /**< reference: OMX_TIZONIA_AUDIO_PARAM_DEEZERSESSIONTYPE */
#define OMX_TizoniaIndexParamAudioDeezerPlaylist     OMX_IndexVendorStartUnused + 20 /**< reference: OMX_TIZONIA_AUDIO_PARAM_DEEZERPLAYLISTTYPE */
#define OMX_TizoniaIndexParamChromecastSession       OMX_IndexVendorStartUnused + 21 /**< reference: OMX_TIZONIA_PARAM_CHROMECASTSESSIONTYPE */
#define OMX_TizoniaIndexParamAudioPlexSession        OMX_IndexVendorStartUnused + 22 /**< reference: OMX_TIZONIA_AUDIO_PARAM_PLEXSESSIONTYPE */
#define OMX_TizoniaIndexParamAudioPlexPlaylist       OMX_IndexVendorStartUnused + 23 /**< reference: OMX_TIZONIA_AUDIO_PARAM_PLEXPLAYLISTTYPE */
#define OMX_TizoniaIndexParamStreamingBuffer         OMX_IndexVendorStartUnused + 24 /**< reference: OMX_TIZONIA_STREAMINGBUFFERTYPE */
#define OMX_TizoniaIndexParamAudioIheartSession      OMX_IndexVendorStartUnused + 25 /**< reference: OMX_TIZONIA_AUDIO_PARAM_IHEARTSESSIONTYPE */
#define OMX_TizoniaIndexParamAudioIheartPlaylist     OMX_IndexVendorStartUnused + 26 /**< reference: OMX_TIZONIA_AUDIO_PARAM_IHEARTPLAYLISTTYPE */
#define OMX_TizoniaIndexConfigPlaylistPosition       OMX_IndexVendorStartUnused + 27 /**< reference: OMX_TIZONIA_PLAYLISTPOSITIONTYPE */

/**
 * OMX_AUDIO_CODINGTYPE extensions
 */

#define OMX_AUDIO_CodingOPUS  OMX_AUDIO_CodingVendorStartUnused + 1
#define OMX_AUDIO_CodingFLAC  OMX_AUDIO_CodingVendorStartUnused + 2
#define OMX_AUDIO_CodingSPEEX OMX_AUDIO_CodingVendorStartUnused + 3
#define OMX_AUDIO_CodingOGA   OMX_AUDIO_CodingVendorStartUnused + 4 /**< this for audio in an ogg container */
#define OMX_AUDIO_CodingMP2   OMX_AUDIO_CodingVendorStartUnused + 5
#define OMX_AUDIO_CodingMP4   OMX_AUDIO_CodingVendorStartUnused + 6 /**< this for audio in a mp4 container */
#define OMX_AUDIO_CodingWEBM  OMX_AUDIO_CodingVendorStartUnused + 7 /**< this for audio in a webm container */

/**
 * OMX_VIDEO_CODINGTYPE extensions
 */
#define OMX_VIDEO_CodingVP9  OMX_VIDEO_CodingVendorStartUnused + 1

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
 * Extension to jump to another track in a playlist,
 * an absolute position relative to the beginning of
 * the playlist.
 */

typedef struct OMX_TIZONIA_PLAYLISTPOSITIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_S32 nPosition; /**< A position inth the playlist. Valid values are  */
                       /**< in the range [1, len(playlist)]. */
} OMX_TIZONIA_PLAYLISTPOSITIONTYPE;

/**
 * Extension to jump to another track in a playlist,
 * relative to the current position.
 */

typedef struct OMX_TIZONIA_PLAYLISTSKIPTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_S32 nValue;              /**< Can be a positive or a negative value,
                                      relative to the current position.
                                      Wrap-around use cases are allowed. */
} OMX_TIZONIA_PLAYLISTSKIPTYPE;

/**
 * Media streaming buffer.
 */
typedef struct OMX_TIZONIA_STREAMINGBUFFERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nCapacity;           /**< Total capacity of the buffer, in seconds. */
    OMX_U32 nLowWaterMark;       /**< A percentage of the total capacity, in the range 0-100. */
    OMX_U32 nHighWaterMark;      /**< A percentage of the total capacity, in the range 0-100. */
} OMX_TIZONIA_STREAMINGBUFFERTYPE;

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
    OMX_U8 cStreamTitle[1];     /**< Max length is OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE */
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
    OMX_U32 nBitRate; /**< 6-256 per channel */
    OMX_U32 nSampleRate; /**< 8 kHz (narrowband) to 48 kHz (fullband) */
    OMX_S32 nFrameDuration; /**< 2.5, 5, 10, 20, 40, or 60 ms. */
    OMX_U32 nEncoderComplexity; /**< From 0 to 10, default 0 */
    OMX_BOOL bPacketLossResilience; /**< default, false  */
    OMX_BOOL bForwardErrorCorrection; /**< default, false  */
    OMX_BOOL bDtx; /**< default, false  */
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
    OMX_U32 nChannels; /**< Up to 8 channels. Default 2.  */
    OMX_U32 nBitsPerSample; /**< 4-32 bits per sample. Default 16. */
    OMX_U32 nSampleRate; /**< From 1Hz to 655350Hz. Default 44100Hz */
    OMX_U32 nCompressionLevel; /**< From 0 (fastest, least compression) to 8
                                   (slowest, most compression). Default 5. For
                                   more info see
                                   https://xiph.org/flac/api/group__flac__stream__encoder.html#ga20*/
    OMX_U32 nBlockSize; /**< Block size in samples. From 16 to 65535. Default 0
                            (let encoder estimate blocksize). */
    OMX_U64 nTotalSamplesEstimate; /**< An estimate of the total samples that
                                       will be encoded. Set to 0 if unknown. */
    OMX_AUDIO_CHANNELMODETYPE eChannelMode;
} OMX_TIZONIA_AUDIO_PARAM_FLACTYPE;

/**
 * MP2 encoder/decoder components
 * References:
 * - http://en.wikipedia.org/wiki/MPEG-1_Audio_Layer_II
 *
 */

typedef enum OMX_TIZONIA_AUDIO_MP2STREAMFORMATTYPE {
    OMX_AUDIO_MP2StreamFormatMP1Layer2 = 0, /**< MP2 Audio MPEG 1 Layer 2 Streaxm format */
    OMX_AUDIO_MP2StreamFormatMP2Layer2,     /**< MP2 Audio MPEG 2 Layer 2 Stream format */
    OMX_AUDIO_MP2StreamFormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_MP2StreamFormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_MP2StreamFormatMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_MP2STREAMFORMATTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_MP2TYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nBitRate;           /**< 8, 16, 24, 40 and 144 kbit/s (MPEG-2 only), 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 and 384 kbit/s (MPEG-1 and MPEG-2) */
    OMX_U32 nSampleRate;        /**< 16, 22.05 and 24 (MPEG-2 only), 32, 44.1 and 48 kHz (MPEG-1 and MPEG-2)*/
    OMX_AUDIO_CHANNELMODETYPE eChannelMode;
    OMX_TIZONIA_AUDIO_MP2STREAMFORMATTYPE eFormat;
} OMX_TIZONIA_AUDIO_PARAM_MP2TYPE;

/**
 * Spotify source component
 * References:
 *
 */

typedef enum OMX_TIZONIA_AUDIO_SPOTIFYPLAYLISTTYPE {
    OMX_AUDIO_SpotifyPlaylistTypeUnknown = 0, /**< Playlist type unknown (Default). */
    OMX_AUDIO_SpotifyPlaylistTypeTracks, /**< Regular tracks search. */
    OMX_AUDIO_SpotifyPlaylistTypeArtist, /**< Artist search. */
    OMX_AUDIO_SpotifyPlaylistTypeAlbum, /**< Album search. */
    OMX_AUDIO_SpotifyPlaylistTypePlaylist, /**< Playlist search. */
    OMX_AUDIO_SpotifyPlaylistTypeTrackId, /**< Track search by Spotify ID, URI or URL. */
    OMX_AUDIO_SpotifyPlaylistTypeArtistId, /**< Artist search by Spotify ID, URI or URL. */
    OMX_AUDIO_SpotifyPlaylistTypeAlbumId, /**< Album search by Spotify ID, URI or URL. */
    OMX_AUDIO_SpotifyPlaylistTypePlaylistId, /**< Playlist search by Spotify ID, URI or URL. */
    OMX_AUDIO_SpotifyPlaylistTypeRelatedArtists, /**< Related artists search. */
    OMX_AUDIO_SpotifyPlaylistTypeFeaturedPlaylist, /**< Featured playlist search. */
    OMX_AUDIO_SpotifyPlaylistTypeNewReleases, /**< New releases album search. */
    OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByTrackId, /**< Find recommendations by track ID, URI or URL. */
    OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByArtistId, /**< Find recommendations by artist ID, URI or URL. */
    OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByTrack, /**< Find recommendations by track name. */
    OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByArtist, /**< Find recommendations by artist name. */
    OMX_AUDIO_SpotifyPlaylistTypeRecommendationsByGenre, /**< Find recommendations by genre name. */
    OMX_AUDIO_SpotifyPlaylistTypeUserLikedTracks, /**< The current user's liked tracks. */
    OMX_AUDIO_SpotifyPlaylistTypeUserRecentTracks, /**< The current user's most recent tracks. */
    OMX_AUDIO_SpotifyPlaylistTypeUserTopTracks, /**< The current user's top tracks. */
    OMX_AUDIO_SpotifyPlaylistTypeUserTopArtists, /**< The current user's top artists. */
    OMX_AUDIO_SpotifyPlaylistTypeUserPlaylist, /**< Playlist search in the user's library. */
    OMX_AUDIO_SpotifyPlaylistTypeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_SpotifyPlaylistTypeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_SpotifyPlaylistTypeMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_SPOTIFYPLAYLISTTYPE;

typedef enum OMX_TIZONIA_AUDIO_SPOTIFYCONNECTIONTYPE {
    OMX_AUDIO_SpotifyConnectionUnknown = 0, /**< Connection type unknown (Default) */
    OMX_AUDIO_SpotifyConnectionNone, /**< No connection */
    OMX_AUDIO_SpotifyConnectionMobile, /**< Mobile data (EDGE, 3G, etc). */
    OMX_AUDIO_SpotifyConnectionMobileRoaming, /**< Roamed mobile data (EDGE, 3G, etc). */
    OMX_AUDIO_SpotifyConnectionMobileWifi, /**< Wireless connection. */
    OMX_AUDIO_SpotifyConnectionMobileWired, /**< Wireless connection. */
    OMX_AUDIO_SpotifyConnectionKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_SpotifyConnectionVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_SpotifyConnectionMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_SPOTIFYCONNECTIONTYPE;

typedef enum OMX_TIZONIA_AUDIO_SPOTIFYBITRATETYPE {
    OMX_AUDIO_SpotifyBitrate160Kbps = 0,
    OMX_AUDIO_SpotifyBitrate320Kbps,
    OMX_AUDIO_SpotifyBitrate96Kbps,
    OMX_AUDIO_SpotifyBitrateKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_SpotifyBitrateVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_SpotifyBitrateMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_SPOTIFYBITRATETYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_SPOTIFYSESSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cUserName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cUserPassword[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cProxyServer[OMX_MAX_STRINGNAME_SIZE]; /**< Url to the proxy server that should be used. */
                                                   /**< The format is protocol://<host>:port (where protocal
                                                        is http/https/socks4/socks5)  */
    OMX_U8 cProxyUserName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cProxyPassword[OMX_MAX_STRINGNAME_SIZE];
    OMX_BOOL bRememberCredentials; /**< default: OMX_TRUE */
    OMX_BOOL bRecoverLostToken; /**< default: OMX_FALSE */
    OMX_BOOL bAllowExplicitTracks; /**< default: OMX_FALSE */
    OMX_TIZONIA_AUDIO_SPOTIFYBITRATETYPE ePreferredBitRate;  /**< 96, 160, or 320 kbps; default: 320 */
    OMX_TIZONIA_AUDIO_SPOTIFYCONNECTIONTYPE eConnectionType; /**< Default: Connection type unknown */
} OMX_TIZONIA_AUDIO_PARAM_SPOTIFYSESSIONTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_SPOTIFYPLAYLISTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cPlaylistName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 bShuffle;            /**< Default: OMX_FALSE */
    OMX_TIZONIA_AUDIO_SPOTIFYPLAYLISTTYPE ePlaylistType;
    OMX_U8 cPlaylistOwner[OMX_MAX_STRINGNAME_SIZE]; /**< Spotify user that owns this list. If empty, the
                                                       user of the currently active session is assumed. */
} OMX_TIZONIA_AUDIO_PARAM_SPOTIFYPLAYLISTTYPE;

/**
 * Google Play Music source component
 * References:
 *
 */

typedef enum OMX_TIZONIA_AUDIO_GMUSICPLAYLISTTYPE {
    OMX_AUDIO_GmusicPlaylistTypeUnknown = 0, /**< Playlist type unknown (Default). */
    OMX_AUDIO_GmusicPlaylistTypeUser, /**< User-defined playlist search. */
    OMX_AUDIO_GmusicPlaylistTypeArtist, /**< Artist search. */
    OMX_AUDIO_GmusicPlaylistTypeAlbum, /**< Album search. */
    OMX_AUDIO_GmusicPlaylistTypeStation, /**< Station search (unlimited). */
    OMX_AUDIO_GmusicPlaylistTypeGenre, /**< Genre search (unlimited). */
    OMX_AUDIO_GmusicPlaylistTypeSituation, /**< Situation search (unlimited). */
    OMX_AUDIO_GmusicPlaylistTypePromotedTracks, /**< Promoted tracks playlist. */
    OMX_AUDIO_GmusicPlaylistTypeTracks, /**< Regular tracks search. */
    OMX_AUDIO_GmusicPlaylistTypePodcast, /**< Podcast search. */
    OMX_AUDIO_GmusicPlaylistTypeLibrary, /**< A playlist containing all elements in the user's library. */
    OMX_AUDIO_GmusicPlaylistTypeFreeStation, /**< A station search (free tier). */
    OMX_AUDIO_GmusicPlaylistTypeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_GmusicPlaylistTypeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_GmusicPlaylistTypeMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_GMUSICPLAYLISTTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cUserName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cUserPassword[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cDeviceId[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_TIZONIA_AUDIO_GMUSICPLAYLISTTYPE ePlaylistType;
    OMX_BOOL bShuffle;            /**< Default: OMX_FALSE */
    OMX_BOOL bUnlimitedSearch;    /**< Default: OMX_FALSE */
    OMX_U8 cPlaylistName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cAdditionalKeywords[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE;


/**
 * SoundCloud source component
 *
 */

typedef enum OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE {
    OMX_AUDIO_SoundCloudPlaylistTypeUnknown = 0, /**< Playlist type unknown (Default). */
    OMX_AUDIO_SoundCloudPlaylistTypeUserStream, /**< The user's stream playlist. */
    OMX_AUDIO_SoundCloudPlaylistTypeUserLikes, /**< The user's likes playlist. */
    OMX_AUDIO_SoundCloudPlaylistTypeUserPlaylist, /**< A playlist from the user's collection. */
    OMX_AUDIO_SoundCloudPlaylistTypeCreator, /**< A creator's top tracks. */
    OMX_AUDIO_SoundCloudPlaylistTypeTracks, /**< Track search. */
    OMX_AUDIO_SoundCloudPlaylistTypePlaylists, /**< Playlist search. */
    OMX_AUDIO_SoundCloudPlaylistTypeGenres, /**< Genres search. */
    OMX_AUDIO_SoundCloudPlaylistTypeTags, /**< Tags search. */
    OMX_AUDIO_SoundCloudPlaylistTypeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_SoundCloudPlaylistTypeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_SoundCloudPlaylistTypeMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDSESSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cUserName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cUserPassword[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cUserOauthToken[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDSESSIONTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDPLAYLISTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE ePlaylistType;
    OMX_BOOL bShuffle;            /**< Default: OMX_FALSE */
    OMX_U8 cPlaylistName[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDPLAYLISTTYPE;

/**
 * Tunein source component
 *
 */

typedef enum OMX_TIZONIA_AUDIO_TUNEINPLAYLISTTYPE {
    OMX_AUDIO_TuneinPlaylistTypeUnknown = 0, /**< Playlist type unknown (Default). */
    OMX_AUDIO_TuneinPlaylistTypeRadios, /**< General search (may be restricted down to stations, shows or all, see OMX_TIZONIA_AUDIO_TUNEINSEARCHTYPE) */
    OMX_AUDIO_TuneinPlaylistTypeCategory, /**< Category search. */
    OMX_AUDIO_TuneinPlaylistTypeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_TuneinPlaylistTypeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_TuneinPlaylistTypeMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_TUNEINPLAYLISTTYPE;

typedef enum OMX_TIZONIA_AUDIO_TUNEINSEARCHTYPE {
    OMX_AUDIO_TuneinSearchTypeAll = 0, /**< Search type all (Default). */
    OMX_AUDIO_TuneinSearchTypeStations, /**< Station search. */
    OMX_AUDIO_TuneinSearchTypeShows, /**< Shows search. */
    OMX_AUDIO_TuneinSearchTypeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_TuneinSearchTypeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_TuneinSearchTypeMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_TUNEINSEARCHTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_TUNEINSESSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cApiKey[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_TUNEINSESSIONTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_TUNEINPLAYLISTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_TIZONIA_AUDIO_TUNEINPLAYLISTTYPE ePlaylistType;
    OMX_TIZONIA_AUDIO_TUNEINSEARCHTYPE eSearchType; /**< Default: OMX_AUDIO_TuneinSearchTypeAll */
    OMX_BOOL bShuffle;            /**< Default: OMX_FALSE */
    OMX_U8 cPlaylistName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cAdditionalKeywords1[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cAdditionalKeywords2[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cAdditionalKeywords3[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_TUNEINPLAYLISTTYPE;

/**
 * Youtube source component
 *
 */
typedef enum OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE {
    OMX_AUDIO_YoutubePlaylistTypeUnknown = 0, /**< Playlist type unknown (Default). */
    OMX_AUDIO_YoutubePlaylistTypeAudioStream, /**< Audio playback from a youtube video url or video id. */
    OMX_AUDIO_YoutubePlaylistTypeAudioPlaylist, /**< Audio playback from a youtube playlist url or playlist id. */
    OMX_AUDIO_YoutubePlaylistTypeAudioMix, /**< Audio playback from a youtube mix associated to a url or video id. */
    OMX_AUDIO_YoutubePlaylistTypeAudioSearch, /**< Audio playback from a youtube search. */
    OMX_AUDIO_YoutubePlaylistTypeAudioMixSearch, /**< Audio playback from a youtube mix associated to a search term. */
    OMX_AUDIO_YoutubePlaylistTypeAudioChannelUploads, /**< Audio playback from a youtube channel url or name. */
    OMX_AUDIO_YoutubePlaylistTypeAudioChannelPlaylist, /**< Audio playback from a youtube channel url or name and playlist name. */
    OMX_AUDIO_YoutubePlaylistTypeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_YoutubePlaylistTypeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_YoutubePlaylistTypeMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_YOUTUBESESSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cApiKey[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_YOUTUBESESSIONTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_YOUTUBEPLAYLISTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE ePlaylistType;
    OMX_BOOL bShuffle;            /**< Default: OMX_FALSE */
    OMX_U8 cPlaylistName[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_YOUTUBEPLAYLISTTYPE;

/**
 * Deezer source component
 * References:
 *
 */

typedef enum OMX_TIZONIA_AUDIO_DEEZERPLAYLISTTYPE {
    OMX_AUDIO_DeezerPlaylistTypeUnknown = 0, /**< Playlist type unknown (Default). */
    OMX_AUDIO_DeezerPlaylistTypeTrack, /**< Regular tracks search. */
    OMX_AUDIO_DeezerPlaylistTypeArtist, /**< Artist search. */
    OMX_AUDIO_DeezerPlaylistTypeAlbum, /**< Album search. */
    OMX_AUDIO_DeezerPlaylistTypeMix, /**< Mixes search. */
    OMX_AUDIO_DeezerPlaylistTypePlaylist, /**< Playlist search. */
    OMX_AUDIO_DeezerPlaylistTypeTopPlaylists, /**< Top playlists search. */
    OMX_AUDIO_DeezerPlaylistTypePodcast, /**< Podcast search. */
    OMX_AUDIO_DeezerPlaylistTypeUserFlow, /**< User radio station. */
    OMX_AUDIO_DeezerPlaylistTypeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_DeezerPlaylistTypeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_DeezerPlaylistTypeMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_DEEZERPLAYLISTTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_DEEZERSESSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cUserId[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_DEEZERSESSIONTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_DEEZERPLAYLISTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_TIZONIA_AUDIO_DEEZERPLAYLISTTYPE ePlaylistType;
    OMX_BOOL bShuffle;            /**< Default: OMX_FALSE */
    OMX_U8 cPlaylistName[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_DEEZERPLAYLISTTYPE;

/**
 * Chromecast renderer component
 */

typedef struct OMX_TIZONIA_PARAM_CHROMECASTSESSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cNameOrIpAddr[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_PARAM_CHROMECASTSESSIONTYPE;

/**
 * Plex source component
 *
 */
typedef enum OMX_TIZONIA_AUDIO_PLEXPLAYLISTTYPE {
    OMX_AUDIO_PlexPlaylistTypeUnknown = 0, /**< Playlist type unknown (Default). */
    OMX_AUDIO_PlexPlaylistTypeAudioTracks, /**< Audio search and playback from a plex track search. */
    OMX_AUDIO_PlexPlaylistTypeAudioArtist, /**< Audio search and playback from a plex artist search. */
    OMX_AUDIO_PlexPlaylistTypeAudioAlbum, /**< Audio search and playback from a plex album search. */
    OMX_AUDIO_PlexPlaylistTypeAudioPlaylist, /**< Audio playback from a plex playlist. */
    OMX_AUDIO_PlexPlaylistTypeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_PlexPlaylistTypeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_PlexPlaylistTypeMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_PLEXPLAYLISTTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_PLEXSESSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cBaseUrl[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cAuthToken[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cMusicSectionName[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_PLEXSESSIONTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_PLEXPLAYLISTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_TIZONIA_AUDIO_PLEXPLAYLISTTYPE ePlaylistType;
    OMX_BOOL bShuffle;            /**< Default: OMX_FALSE */
    OMX_U8 cPlaylistName[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_PLEXPLAYLISTTYPE;

/**
 * Iheart source component
 *
 */

typedef enum OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE {
    OMX_AUDIO_IheartPlaylistTypeUnknown = 0, /**< Playlist type unknown (Default). */
    OMX_AUDIO_IheartPlaylistTypeRadios, /**< General search */
    OMX_AUDIO_IheartPlaylistTypeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_AUDIO_IheartPlaylistTypeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_IheartPlaylistTypeMax = 0x7FFFFFFF
} OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_IHEARTSESSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 cApiKey[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_IHEARTSESSIONTYPE;

typedef struct OMX_TIZONIA_AUDIO_PARAM_IHEARTPLAYLISTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE ePlaylistType;
    OMX_BOOL bShuffle;            /**< Default: OMX_FALSE */
    OMX_U8 cPlaylistName[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cAdditionalKeywords1[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cAdditionalKeywords2[OMX_MAX_STRINGNAME_SIZE];
    OMX_U8 cAdditionalKeywords3[OMX_MAX_STRINGNAME_SIZE];
} OMX_TIZONIA_AUDIO_PARAM_IHEARTPLAYLISTTYPE;


#endif /* OMX_TizoniaExt_h */

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
 * @file   tizprobe.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Stream probing utilities
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <ZenLib/Ztring.h>
#include <MediaInfo/MediaInfo.h>
#include <MediaInfo/MediaInfo_Const.h>

#include <tizplatform.h>

#include "tizprobe.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.probe"
#endif

namespace  // unnamed
{
  std::string mi_stream_general_info_to_std_string (MediaInfoLib::MediaInfo &mi,
                                                    const std::wstring info)
  {
    std::wstring wide (
        mi.Get (MediaInfoLib::Stream_General,  // Stream type: General
                0,                             // Stream number
                info, MediaInfoLib::Info_Text, MediaInfoLib::Info_Name));
    return std::string (wide.begin (), wide.end ());
  }

  std::string mi_stream_audio_info_to_std_string (MediaInfoLib::MediaInfo &mi,
                                                  const std::wstring info)
  {
    std::wstring wide (
        mi.Get (MediaInfoLib::Stream_Audio,  // Stream type: audio
                0,                           // Stream number
                info, MediaInfoLib::Info_Text, MediaInfoLib::Info_Name));
    return std::string (wide.begin (), wide.end ());
  }

  void mi_stream_audio_info_to_unsigned (MediaInfoLib::MediaInfo &mi,
                                         const std::wstring info,
                                         OMX_U32 &result)
  {
    std::string str (mi_stream_audio_info_to_std_string (mi, info));
    if (!str.empty ())
    {
      result = boost::lexical_cast< OMX_U32 >(str);
    }
  }

  bool is_pcm_codec (const OMX_AUDIO_CODINGTYPE a_codec_id)
  {
    bool rc = false;
    if (a_codec_id == OMX_AUDIO_CodingPCM || a_codec_id
                                             == OMX_AUDIO_CodingADPCM)
    {
      rc = true;
    }
    TIZ_LOG (TIZ_PRIORITY_TRACE, "outcome ? %s", (rc ? "PCM" : "OTHER"));
    return rc;
  }

  bool open_media (const std::string &uri, MediaInfoLib::MediaInfo &mi)
  {
    ZenLib::Ztring file_uri = uri.c_str ();
    return (mi.Open (file_uri) > 0);
  }

  void obtain_stream_title_and_genre (MediaInfoLib::MediaInfo &mi,
                                      const bool quiet,
                                      std::string &stream_title,
                                      std::string &stream_genre)
  {
    std::string artist (
        mi_stream_general_info_to_std_string (mi, L"Performer"));
    std::string title (mi_stream_general_info_to_std_string (mi, L"Track"));
    std::string album (mi_stream_general_info_to_std_string (mi, L"Album"));
    std::string genre (mi_stream_general_info_to_std_string (mi, L"Genre"));
    std::string full_track_name (
        mi_stream_general_info_to_std_string (mi, L"CompleteName"));

    stream_title.assign (artist);
    if (!album.empty ())
    {
      stream_title.append (" - ");
      stream_title.append (album);
    }

    if (!title.empty ())
    {
      stream_title.append (" - ");
      stream_title.append (title);
    }
    stream_genre.assign (genre);

    if (!quiet)
    {
      if (stream_title.empty ())
      {
        stream_title.assign (full_track_name);
      }
      boost::replace_all (stream_title, "_", " ");
    }
  }

  OMX_AUDIO_CODINGTYPE obtain_codec_id (MediaInfoLib::MediaInfo &mi)
  {
    OMX_AUDIO_CODINGTYPE codec = OMX_AUDIO_CodingMP3;
    std::string format (mi_stream_audio_info_to_std_string (mi, L"Format"));
    std::string version (
        mi_stream_audio_info_to_std_string (mi, L"Format version"));
    std::string profile (
        mi_stream_audio_info_to_std_string (mi, L"Format profile"));

    if (!format.compare ("MPEG Audio") && !version.compare ("Version 1")
        && !profile.compare ("Layer 3"))
    {
      codec = OMX_AUDIO_CodingMP3;
    }
    else if (!format.compare ("MPEG Audio") && !version.compare ("Version 1")
             && !profile.compare ("Layer 2"))

    {
      codec = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingMP2);
    }
    else if (!format.compare ("Vorbis"))

    {
      codec = OMX_AUDIO_CodingVORBIS;
    }
    else if (!format.compare ("AAC"))

    {
      codec = OMX_AUDIO_CodingAAC;
    }
    else if (!format.compare ("Opus"))

    {
      codec = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingOPUS);
    }
    else if (!format.compare ("FLAC"))

    {
      codec = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingFLAC);
    }
    else if (!format.compare ("PCM"))

    {
      codec = OMX_AUDIO_CodingPCM;
    }

    return codec;
  }

  void obtain_stream_properties (MediaInfoLib::MediaInfo &mi,
                                 OMX_U32 &samplerate, OMX_U32 &bitrate,
                                 OMX_U32 &nchannels, OMX_U32 &bitdepth,
                                 OMX_ENDIANTYPE &endianness,
                                 OMX_NUMERICALDATATYPE &sign,
                                 bool &stream_is_cbr)
  {
    mi_stream_audio_info_to_unsigned (mi, L"SamplingRate", samplerate);
    mi_stream_audio_info_to_unsigned (mi, L"BitRate", bitrate);
    mi_stream_audio_info_to_unsigned (mi, L"Channel(s)", nchannels);
    mi_stream_audio_info_to_unsigned (mi, L"BitDepth", bitdepth);

    std::string en (
        mi_stream_audio_info_to_std_string (mi, L"Format_Settings_Endianness"));
    endianness = en.empty () ? endianness
                             : (en.compare ("Little") == 0 ? OMX_EndianLittle
                                                           : OMX_EndianBig);

    std::string s (
        mi_stream_audio_info_to_std_string (mi, L"Format_Settings_Sign"));
    sign = s.empty () ? sign
                      : (s.compare ("Signed") == 0 ? OMX_NumericalDataSigned
                                                   : OMX_NumericalDataUnsigned);

    std::string cbr_or_vbr (
        mi_stream_general_info_to_std_string (mi, L"OverallBitRate_Mode"));
    stream_is_cbr = (cbr_or_vbr.compare ("CBR") == 0);
  }

  OMX_MEDIACONTAINER_FORMATTYPE obtain_container_format (
      MediaInfoLib::MediaInfo &mi)
  {
    OMX_MEDIACONTAINER_FORMATTYPE container_format = OMX_FORMATMax;
    std::string format (mi_stream_general_info_to_std_string (mi, L"Format"));
    if (format.compare ("OGG") == 0)
    {
      container_format = OMX_FORMAT_OGG;
    }
    else if (format.compare ("MPEG AUDIO") == 0)
    {
      container_format = OMX_FORMAT_MP3;
    }
    else
    {
      container_format = OMX_FORMAT_RAW;
    }
    return container_format;
  }
}

tiz::probe::probe (const std::string &uri, const bool quiet)
  : uri_ (uri),
    quiet_ (quiet),
    domain_ (OMX_PortDomainMax),
    audio_coding_type_ (OMX_AUDIO_CodingUnused),
    video_coding_type_ (OMX_VIDEO_CodingUnused),
    container_type_ (OMX_FORMATMax),
    pcmtype_ (),
    mp2type_ (),
    mp3type_ (),
    opustype_ (),
    flactype_ (),
    vorbistype_ (),
    aactype_ (),
    vp8type_ (),
    meta_file_ (uri.c_str ()),
    stream_title_ (),
    stream_genre_ (),
    stream_is_cbr_ (false)
{
  // Defaults are the same as in the standard pcm renderer
  pcmtype_.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmtype_.nVersion.nVersion = OMX_VERSION;
  pcmtype_.nPortIndex = 0;
  pcmtype_.nChannels = 2;
  pcmtype_.eNumData = OMX_NumericalDataSigned;
  pcmtype_.eEndian = OMX_EndianBig;
  pcmtype_.bInterleaved = OMX_TRUE;
  pcmtype_.nBitPerSample = 16;
  pcmtype_.nSamplingRate = 48000;
  pcmtype_.ePCMMode = OMX_AUDIO_PCMModeLinear;
  pcmtype_.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcmtype_.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

  // mp2 decoding settings
  mp2type_.nSize = sizeof(OMX_TIZONIA_AUDIO_PARAM_MP2TYPE);
  mp2type_.nVersion.nVersion = OMX_VERSION;
  mp2type_.nPortIndex = 0;
  mp2type_.nChannels = 2;
  mp2type_.nBitRate = 0;
  mp2type_.nSampleRate = 48000;
  mp2type_.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  mp2type_.eFormat = OMX_AUDIO_MP2StreamFormatMP2Layer2;

  // Defaults are the same as in the standard mp3 decoder
  mp3type_.nSize = sizeof(OMX_AUDIO_PARAM_MP3TYPE);
  mp3type_.nVersion.nVersion = OMX_VERSION;
  mp3type_.nPortIndex = 0;
  mp3type_.nChannels = 2;
  mp3type_.nBitRate = 0;
  mp3type_.nSampleRate = 48000;
  mp3type_.nAudioBandWidth = 0;
  mp3type_.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  mp3type_.eFormat = OMX_AUDIO_MP3StreamFormatMP2Layer3;

  // Defaults for the opus decoder
  opustype_.nSize = sizeof(OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE);
  opustype_.nVersion.nVersion = OMX_VERSION;
  opustype_.nPortIndex = 0;
  opustype_.nChannels = 2;
  opustype_.nBitRate = 256;
  opustype_.nSampleRate = 48000;
  opustype_.nFrameDuration = 2.5;
  opustype_.nEncoderComplexity = 0;
  opustype_.bPacketLossResilience = OMX_FALSE;
  opustype_.bForwardErrorCorrection = OMX_FALSE;
  opustype_.bDtx = OMX_FALSE;
  opustype_.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  opustype_.eFormat = OMX_AUDIO_OPUSStreamFormatVBR;

  // Defaults for the flac decoder
  flactype_.nSize = sizeof(OMX_TIZONIA_AUDIO_PARAM_FLACTYPE);
  flactype_.nVersion.nVersion = OMX_VERSION;
  flactype_.nPortIndex = 0;
  flactype_.nChannels = 2;
  flactype_.nBitsPerSample = 16;
  flactype_.nSampleRate = 48000;
  flactype_.nCompressionLevel = 5;
  flactype_.nBlockSize = 0;
  flactype_.nTotalSamplesEstimate = 0;
  flactype_.eChannelMode = OMX_AUDIO_ChannelModeStereo;

  // Defaults for the vorbis decoder
  vorbistype_.nSize = sizeof(OMX_AUDIO_PARAM_VORBISTYPE);
  vorbistype_.nVersion.nVersion = OMX_VERSION;
  vorbistype_.nPortIndex = 0;
  vorbistype_.nPortIndex = 0;
  vorbistype_.nChannels = 2;
  vorbistype_.nBitRate = 0;
  vorbistype_.nMinBitRate = 0;
  vorbistype_.nMaxBitRate = 0;
  vorbistype_.nSampleRate = 48000;
  vorbistype_.nAudioBandWidth = 0;
  vorbistype_.nQuality = 5;
  vorbistype_.bManaged = OMX_FALSE;
  vorbistype_.bDownmix = OMX_FALSE;

  // Defaults for the aac decoder
  aactype_.nSize = sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE);
  aactype_.nVersion.nVersion = OMX_VERSION;
  aactype_.nPortIndex = 0;
  aactype_.nChannels = 2;
  aactype_.nSampleRate = 48000;
  aactype_.nBitRate = 0;
  aactype_.nAudioBandWidth = 0;
  aactype_.nFrameLength = 0;
  aactype_.nAACtools = OMX_AUDIO_AACToolAll;
  aactype_.nAACERtools = OMX_AUDIO_AACERAll;
  aactype_.eAACProfile = OMX_AUDIO_AACObjectLC;
  aactype_.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP2ADTS;
  aactype_.eChannelMode = OMX_AUDIO_ChannelModeStereo;

  // Defaults for the vp8 decoder
  vp8type_.nSize = sizeof (OMX_VIDEO_PARAM_VP8TYPE);
  vp8type_.nVersion.nVersion = OMX_VERSION;
  vp8type_.nPortIndex = 0;
  vp8type_.eProfile = OMX_VIDEO_VP8ProfileMain;
  vp8type_.eLevel = OMX_VIDEO_VP8Level_Version0;
  vp8type_.nDCTPartitions = 0; /* 1 DCP partitiion */
  vp8type_.bErrorResilientMode = OMX_FALSE;
}

std::string tiz::probe::get_uri () const
{
  return uri_;
}

OMX_PORTDOMAINTYPE
tiz::probe::get_omx_domain ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }

  return domain_;
}

OMX_AUDIO_CODINGTYPE
tiz::probe::get_audio_coding_type ()
{
  if (OMX_AUDIO_CodingUnused == audio_coding_type_)
  {
    probe_stream ();
  }
  return audio_coding_type_;
}

OMX_VIDEO_CODINGTYPE
tiz::probe::get_video_coding_type ()
{
  if (OMX_VIDEO_CodingUnused == video_coding_type_)
  {
    probe_stream ();
  }
  return video_coding_type_;
}

OMX_MEDIACONTAINER_FORMATTYPE
tiz::probe::get_container_type ()
{
  if (OMX_FORMATMax == container_type_)
  {
    probe_stream ();
  }
  return container_type_;
}

void tiz::probe::probe_stream ()
{
  MediaInfoLib::MediaInfo mi;

  if (open_media (uri_, mi))
  {
    OMX_U32 samplerate = 48000;
    OMX_U32 bitrate = 0;
    OMX_U32 nchannels = 2;
    OMX_U32 bitdepth = 16;
    OMX_ENDIANTYPE endianness = OMX_EndianLittle;
    OMX_NUMERICALDATATYPE sign = OMX_NumericalDataSigned;

    // Get an idea of the container format
    container_type_ = obtain_container_format (mi);

    // Get the codec type
    const OMX_AUDIO_CODINGTYPE codec_id = obtain_codec_id (mi);

    // Get the stream title and genre
    obtain_stream_title_and_genre (mi, quiet_, stream_title_, stream_genre_);

    TIZ_PRINTF_DBG_RED ("uri [%s] codec_id [%0x]\n", uri_.c_str (), codec_id);

    // Grab the sample rate, bitrate, num channels, and sample format (when
    // available), and cbr flag
    obtain_stream_properties (mi, samplerate, bitrate, nchannels, bitdepth,
                              endianness, sign, stream_is_cbr_);

    if (codec_id == (OMX_AUDIO_CODINGTYPE)OMX_AUDIO_CodingMP2)
    {
      set_mp2_codec_info (samplerate, bitrate, nchannels, bitdepth, endianness,
                          sign);
    }
    else if (codec_id == OMX_AUDIO_CodingMP3)
    {
      set_mp3_codec_info (samplerate, bitrate, nchannels, bitdepth, endianness,
                          sign);
    }
    else if (codec_id == OMX_AUDIO_CodingAAC)
    {
      set_aac_codec_info (samplerate, bitrate, nchannels, bitdepth, endianness,
                          sign);
    }
    else if (codec_id == (OMX_AUDIO_CODINGTYPE)OMX_AUDIO_CodingFLAC)
    {
      set_flac_codec_info (samplerate, bitrate, nchannels, bitdepth, endianness,
                           sign);
    }
    else if (codec_id == OMX_AUDIO_CodingVORBIS)
    {
      set_vorbis_codec_info (samplerate, bitrate, nchannels, bitdepth,
                             endianness, sign);
    }
    else if (codec_id == (OMX_AUDIO_CODINGTYPE)OMX_AUDIO_CodingOPUS)
    {
      set_opus_codec_info (samplerate, bitrate, nchannels, bitdepth, endianness,
                           sign);
    }
    else if (is_pcm_codec (codec_id))
    {
      domain_ = OMX_PortDomainAudio;
      audio_coding_type_
          = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingPCM);
      pcmtype_.nSamplingRate = samplerate;
      pcmtype_.nChannels = nchannels;
      pcmtype_.nBitPerSample = bitdepth;
      pcmtype_.eEndian = endianness;
      pcmtype_.eNumData = sign;
    }

    mi.Close ();
  }
}

void tiz::probe::set_mp2_codec_info (const OMX_U32 samplerate,
                                     const OMX_U32 bitrate,
                                     const OMX_U32 nchannels,
                                     const OMX_U32 bitdepth,
                                     const OMX_ENDIANTYPE endianness,
                                     const OMX_NUMERICALDATATYPE sign)
{
  domain_ = OMX_PortDomainAudio;
  audio_coding_type_ = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingMP2);
  mp2type_.nSampleRate = pcmtype_.nSamplingRate = samplerate;
  mp2type_.nBitRate = bitrate;
  mp2type_.nChannels = pcmtype_.nChannels = nchannels;

  if (1 == pcmtype_.nChannels)
  {
    pcmtype_.bInterleaved = OMX_FALSE;
  }

  pcmtype_.nBitPerSample = bitdepth;
  pcmtype_.eEndian = endianness;
  pcmtype_.eNumData = sign;
}

void tiz::probe::set_mp3_codec_info (const OMX_U32 samplerate,
                                     const OMX_U32 bitrate,
                                     const OMX_U32 nchannels,
                                     const OMX_U32 bitdepth,
                                     const OMX_ENDIANTYPE endianness,
                                     const OMX_NUMERICALDATATYPE sign)
{
  domain_ = OMX_PortDomainAudio;
  audio_coding_type_ = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingMP3);
  mp3type_.nSampleRate = pcmtype_.nSamplingRate = samplerate;
  mp3type_.nBitRate = bitrate;
  mp3type_.nChannels = pcmtype_.nChannels = nchannels;

  if (1 == pcmtype_.nChannels)
  {
    pcmtype_.bInterleaved = OMX_FALSE;
  }

  pcmtype_.nBitPerSample = bitdepth;
  pcmtype_.eEndian = endianness;
  pcmtype_.eNumData = sign;
}

void tiz::probe::set_aac_codec_info (const OMX_U32 samplerate,
                                     const OMX_U32 bitrate,
                                     const OMX_U32 nchannels,
                                     const OMX_U32 bitdepth,
                                     const OMX_ENDIANTYPE endianness,
                                     const OMX_NUMERICALDATATYPE sign)
{
  domain_ = OMX_PortDomainAudio;
  audio_coding_type_ = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingAAC);
  aactype_.nSampleRate = pcmtype_.nSamplingRate = samplerate;
  aactype_.nBitRate = bitrate;
  aactype_.nChannels = pcmtype_.nChannels = nchannels;
  pcmtype_.nBitPerSample = bitdepth;
  pcmtype_.eEndian = endianness;
  pcmtype_.eNumData = sign;

  aactype_.nAACtools = OMX_AUDIO_AACToolAll;
  aactype_.nAACERtools = OMX_AUDIO_AACERAll;
  aactype_.eAACProfile = OMX_AUDIO_AACObjectLC;
  aactype_.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP2ADTS;
  aactype_.eChannelMode = OMX_AUDIO_ChannelModeStereo;

  if (1 == pcmtype_.nChannels)
  {
    pcmtype_.bInterleaved = OMX_FALSE;
    aactype_.eChannelMode = OMX_AUDIO_ChannelModeMono;
  }
}

void tiz::probe::set_opus_codec_info (const OMX_U32 samplerate,
                                      const OMX_U32 bitrate,
                                      const OMX_U32 nchannels,
                                      const OMX_U32 bitdepth,
                                      const OMX_ENDIANTYPE endianness,
                                      const OMX_NUMERICALDATATYPE sign)
{
  domain_ = OMX_PortDomainAudio;
  audio_coding_type_
      = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingOPUS);
  opustype_.nSampleRate = pcmtype_.nSamplingRate = samplerate;
  opustype_.nChannels = pcmtype_.nChannels = nchannels;

  pcmtype_.bInterleaved = OMX_TRUE;
  pcmtype_.nBitPerSample = bitdepth;
  pcmtype_.eEndian = endianness;
  pcmtype_.eNumData = sign;
}

void tiz::probe::set_flac_codec_info (const OMX_U32 samplerate,
                                      const OMX_U32 bitrate,
                                      const OMX_U32 nchannels,
                                      const OMX_U32 bitdepth,
                                      const OMX_ENDIANTYPE endianness,
                                      const OMX_NUMERICALDATATYPE sign)
{
  domain_ = OMX_PortDomainAudio;
  audio_coding_type_
      = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingFLAC);
  flactype_.nSampleRate = pcmtype_.nSamplingRate = samplerate;
  flactype_.nChannels = pcmtype_.nChannels = nchannels;

  if (1 == pcmtype_.nChannels)
  {
    pcmtype_.bInterleaved = OMX_FALSE;
  }

  pcmtype_.nBitPerSample = bitdepth;
  pcmtype_.eEndian = endianness;
  pcmtype_.eNumData = sign;
}

void tiz::probe::set_vorbis_codec_info (const OMX_U32 samplerate,
                                        const OMX_U32 bitrate,
                                        const OMX_U32 nchannels,
                                        const OMX_U32 bitdepth,
                                        const OMX_ENDIANTYPE endianness,
                                        const OMX_NUMERICALDATATYPE sign)
{
  domain_ = OMX_PortDomainAudio;
  audio_coding_type_
      = static_cast< OMX_AUDIO_CODINGTYPE >(OMX_AUDIO_CodingVORBIS);
  vorbistype_.nSampleRate = pcmtype_.nSamplingRate = samplerate;
  vorbistype_.nChannels = pcmtype_.nChannels = nchannels;

  if (1 == pcmtype_.nChannels)
  {
    pcmtype_.bInterleaved = OMX_FALSE;
  }

  // This is a hack, not sure why libav says this is a 16 bit format
  //   pcmtype_.nBitPerSample = 32;
  pcmtype_.nBitPerSample = bitdepth;
  pcmtype_.eEndian = endianness;
  pcmtype_.eNumData = sign;
}

void tiz::probe::get_pcm_codec_info (OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype)
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }

  pcmtype = pcmtype_;
  pcmtype.eChannelMapping[0] = pcmtype_.eChannelMapping[0];
  pcmtype.eChannelMapping[1] = pcmtype_.eChannelMapping[1];

  return;
}

void tiz::probe::set_pcm_codec_info (const OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype)
{
  pcmtype_ = pcmtype;
  return;
}

void tiz::probe::get_mp2_codec_info (OMX_TIZONIA_AUDIO_PARAM_MP2TYPE &mp2type)
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  mp2type = mp2type_;
  return;
}

void tiz::probe::get_mp3_codec_info (OMX_AUDIO_PARAM_MP3TYPE &mp3type)
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  mp3type = mp3type_;
  return;
}

void tiz::probe::get_aac_codec_info (OMX_AUDIO_PARAM_AACPROFILETYPE &aactype)
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  aactype = aactype_;
  return;
}

void tiz::probe::get_opus_codec_info (
    OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE &opustype)
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  opustype = opustype_;

  return;
}

void tiz::probe::get_flac_codec_info (
    OMX_TIZONIA_AUDIO_PARAM_FLACTYPE &flactype)
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  flactype = flactype_;
  return;
}

void tiz::probe::get_vorbis_codec_info (OMX_AUDIO_PARAM_VORBISTYPE &vorbistype)
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  vorbistype = vorbistype_;
  return;
}

void tiz::probe::get_vp8_codec_info (OMX_VIDEO_PARAM_VP8TYPE &vp8type)
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  vp8type = vp8type_;
  return;
}

std::string tiz::probe::get_stream_title ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  if (stream_title_.empty ())
  {
    stream_title_.assign (uri_.c_str ());
    boost::replace_all (stream_title_, "_", " ");
  }
  return stream_title_;
}

std::string tiz::probe::get_stream_genre ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  return stream_genre_;
}

bool tiz::probe::is_cbr_stream ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }
  return stream_is_cbr_;
}

std::string tiz::probe::retrieve_meta_data_str (
    TagLib::String (TagLib::Tag::*TagFunction)() const) const
{
  assert (TagFunction);
  if (!meta_file_.isNull () && meta_file_.tag ())
  {
    TagLib::Tag *tag = meta_file_.tag ();
    return (tag->*TagFunction)().stripWhiteSpace ().to8Bit ();
  }
  return std::string ();
}

unsigned int tiz::probe::retrieve_meta_data_uint (
    TagLib::uint (TagLib::Tag::*TagFunction)() const) const
{
  assert (TagFunction);
  if (!meta_file_.isNull () && meta_file_.tag ())
  {
    TagLib::Tag *tag = meta_file_.tag ();
    return (tag->*TagFunction)();
  }
  return 0;
}

std::string tiz::probe::title () const
{
  return retrieve_meta_data_str (&TagLib::Tag::title);
}

std::string tiz::probe::artist () const
{
  return retrieve_meta_data_str (&TagLib::Tag::artist);
}

std::string tiz::probe::album () const
{
  return retrieve_meta_data_str (&TagLib::Tag::album);
}

std::string tiz::probe::year () const
{
  return boost::lexical_cast< std::string >(
      retrieve_meta_data_uint (&TagLib::Tag::year));
}

std::string tiz::probe::comment () const
{
  return retrieve_meta_data_str (&TagLib::Tag::comment);
}

std::string tiz::probe::track () const
{
  return boost::lexical_cast< std::string >(
      retrieve_meta_data_uint (&TagLib::Tag::track));
}

std::string tiz::probe::genre () const
{
  return retrieve_meta_data_str (&TagLib::Tag::genre);
}

std::string tiz::probe::stream_length () const
{
  std::string length_str;

  if (!meta_file_.isNull () && meta_file_.audioProperties ())
  {
    TagLib::AudioProperties *properties = meta_file_.audioProperties ();
    int seconds = properties->length () % 60;
    int minutes = (properties->length () - seconds) / 60;
    int hours = 0;
    if (minutes >= 60)
    {
      int total_minutes = minutes;
      minutes = total_minutes % 60;
      hours = (total_minutes - minutes) / 60;
    }

    if (hours > 0)
    {
      length_str.append (boost::lexical_cast< std::string >(hours));
      length_str.append ("h:");
    }

    if (minutes > 0)
    {
      length_str.append (boost::lexical_cast< std::string >(minutes));
      length_str.append ("m:");
    }

    char seconds_str[3];
    sprintf (seconds_str, "%02i", seconds);
    length_str.append (seconds_str);
    length_str.append ("s");
  }

  return length_str;
}

void tiz::probe::dump_pcm_info ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }

  TIZ_PRINTF_MAG ("     %ld Ch, %g KHz, %lu:%s:%s\n", pcmtype_.nChannels,
                  ((float)pcmtype_.nSamplingRate) / 1000,
                  pcmtype_.nBitPerSample,
                  pcmtype_.eNumData == OMX_NumericalDataSigned ? "s" : "u",
                  pcmtype_.eEndian == OMX_EndianBig ? "b" : "l");
}

void tiz::probe::dump_mp3_info ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }

  TIZ_PRINTF_MAG ("     %ld Ch, %g KHz, %lu Kbps\n", mp3type_.nChannels,
                  ((float)mp3type_.nSampleRate) / 1000,
                  mp3type_.nBitRate / 1000);
}

void tiz::probe::dump_mp2_and_pcm_info ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }

  TIZ_PRINTF_MAG ("     %ld Ch, %g KHz, %lu Kbps, %lu:%s:%s\n",
                  mp2type_.nChannels, ((float)mp2type_.nSampleRate) / 1000,
                  mp2type_.nBitRate / 1000, pcmtype_.nBitPerSample,
                  pcmtype_.eNumData == OMX_NumericalDataSigned ? "s" : "u",
                  pcmtype_.eEndian == OMX_EndianBig ? "b" : "l");
}

void tiz::probe::dump_mp3_and_pcm_info ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }

  TIZ_PRINTF_MAG ("     %ld Ch, %g KHz, %lu Kbps, %lu:%s:%s\n",
                  mp3type_.nChannels, ((float)mp3type_.nSampleRate) / 1000,
                  mp3type_.nBitRate / 1000, pcmtype_.nBitPerSample,
                  pcmtype_.eNumData == OMX_NumericalDataSigned ? "s" : "u",
                  pcmtype_.eEndian == OMX_EndianBig ? "b" : "l");
}

void tiz::probe::dump_aac_and_pcm_info ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }

  TIZ_PRINTF_MAG ("     %ld Ch, %g KHz, %lu Kbps, %lu:%s:%s\n",
                  aactype_.nChannels, ((float)aactype_.nSampleRate) / 1000,
                  aactype_.nBitRate / 1000, pcmtype_.nBitPerSample,
                  pcmtype_.eNumData == OMX_NumericalDataSigned ? "s" : "u",
                  pcmtype_.eEndian == OMX_EndianBig ? "b" : "l");
}

void tiz::probe::dump_stream_metadata ()
{
  if (OMX_PortDomainMax == domain_)
  {
    probe_stream ();
  }

  std::string the_title = title ().empty () ? get_stream_title () : title ();
  std::string the_artist = artist ().empty () ? get_stream_genre () : artist ();

  TIZ_PRINTF_YEL ("   %s, %s\n", the_title.c_str (), the_artist.c_str ());
  TIZ_PRINTF_CYN ("     Duration : %s\n", stream_length ().c_str ());
  TIZ_PRINTF_CYN ("     Size : %.2g MiB\n",
      ((float)boost::filesystem::file_size (uri_.c_str ()) / (1024 * 1024)));
  if (!album ().empty ())
  {
    TIZ_PRINTF_CYN ("     Album : %s\n", album ().c_str ());
  }
  if (!year ().empty () && year ().compare ("0") != 0)
  {
    TIZ_PRINTF_CYN ("     Year : %s\n", year ().c_str ());
  }
  if (!track ().empty () && track ().compare ("0") != 0)
  {
    TIZ_PRINTF_CYN ("     Track # : %s\n", track ().c_str ());
  }
  if (!genre ().empty ())
  {
    TIZ_PRINTF_CYN ("     Genre : %s\n", genre ().c_str ());
  }
  if (!comment ().empty ())
  {
    TIZ_PRINTF_CYN ("     Comment : %s\n", comment ().c_str ());
  }
}

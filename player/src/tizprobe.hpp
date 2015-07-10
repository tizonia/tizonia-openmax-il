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
 * @file   tizprobe.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Stream probing utilities
 *
 *
 */

#ifndef TIZPROBE_HPP
#define TIZPROBE_HPP

#include <string>
#include <boost/shared_ptr.hpp>

#include <fileref.h>
#include <tag.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <OMX_TizoniaExt.h>

namespace tiz
{
  class probe
  {

  public:
    probe (const std::string &uri, const bool quiet = false);

    std::string get_uri () const;
    OMX_PORTDOMAINTYPE get_omx_domain ();
    OMX_AUDIO_CODINGTYPE get_audio_coding_type ();
    OMX_VIDEO_CODINGTYPE get_video_coding_type ();
    OMX_MEDIACONTAINER_FORMATTYPE get_container_type ();

    void get_pcm_codec_info (OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype);
    void set_pcm_codec_info (const OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype);
    void get_mp2_codec_info (OMX_TIZONIA_AUDIO_PARAM_MP2TYPE &mp2type);
    void get_mp3_codec_info (OMX_AUDIO_PARAM_MP3TYPE &mp3type);
    void get_aac_codec_info (OMX_AUDIO_PARAM_AACPROFILETYPE &aactype);
    void get_opus_codec_info (OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE &opustype);
    void get_flac_codec_info (OMX_TIZONIA_AUDIO_PARAM_FLACTYPE &flactype);
    void get_vorbis_codec_info (OMX_AUDIO_PARAM_VORBISTYPE &vorbistype);
    void get_vp8_codec_info (OMX_VIDEO_PARAM_VP8TYPE &vp8type);

    /* Meta-data information */
    std::string title () const;
    std::string artist () const;
    std::string album () const;
    std::string year () const;
    std::string comment () const;
    std::string track () const;
    std::string genre () const;

    /* Meta-data information. These methods are currently used by the http
       streaming use case. */
    std::string get_stream_title ();
    std::string get_stream_genre ();
    bool is_cbr_stream ();

    /* Duration */
    std::string stream_length () const;

    void dump_pcm_info ();
    void dump_mp3_info ();
    void dump_mp2_and_pcm_info ();
    void dump_mp3_and_pcm_info ();
    void dump_aac_and_pcm_info ();
    void dump_stream_metadata ();

  private:
    void probe_stream ();
    void set_mp2_codec_info (const OMX_U32 samplerate, const OMX_U32 bitrate,
                             const OMX_U32 nchannels, const OMX_U32 bitdepth,
                             const OMX_ENDIANTYPE endianness,
                             const OMX_NUMERICALDATATYPE sign);
    void set_mp3_codec_info (const OMX_U32 samplerate, const OMX_U32 bitrate,
                             const OMX_U32 nchannels, const OMX_U32 bitdepth,
                             const OMX_ENDIANTYPE endianness,
                             const OMX_NUMERICALDATATYPE sign);
    void set_aac_codec_info (const OMX_U32 samplerate, const OMX_U32 bitrate,
                             const OMX_U32 nchannels, const OMX_U32 bitdepth,
                             const OMX_ENDIANTYPE endianness,
                             const OMX_NUMERICALDATATYPE sign);
    void set_opus_codec_info (const OMX_U32 samplerate, const OMX_U32 bitrate,
                              const OMX_U32 nchannels, const OMX_U32 bitdepth,
                              const OMX_ENDIANTYPE endianness,
                              const OMX_NUMERICALDATATYPE sign);
    void set_flac_codec_info (const OMX_U32 samplerate, const OMX_U32 bitrate,
                              const OMX_U32 nchannels, const OMX_U32 bitdepth,
                              const OMX_ENDIANTYPE endianness,
                              const OMX_NUMERICALDATATYPE sign);
    void set_vorbis_codec_info (const OMX_U32 samplerate, const OMX_U32 bitrate,
                                const OMX_U32 nchannels, const OMX_U32 bitdepth,
                                const OMX_ENDIANTYPE endianness,
                                const OMX_NUMERICALDATATYPE sign);
    std::string retrieve_meta_data_str (
        TagLib::String (TagLib::Tag::*TagFunction)() const) const;
    unsigned int retrieve_meta_data_uint (
        TagLib::uint (TagLib::Tag::*TagFunction)() const) const;

  private:
    std::string uri_;
    bool quiet_;  // this is to control whether the probe object should dump any
    // format info to the stdout
    OMX_PORTDOMAINTYPE domain_;
    OMX_AUDIO_CODINGTYPE audio_coding_type_;
    OMX_VIDEO_CODINGTYPE video_coding_type_;
    OMX_MEDIACONTAINER_FORMATTYPE container_type_;
    OMX_AUDIO_PARAM_PCMMODETYPE pcmtype_;
    OMX_TIZONIA_AUDIO_PARAM_MP2TYPE mp2type_;
    OMX_AUDIO_PARAM_MP3TYPE mp3type_;
    OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype_;
    OMX_TIZONIA_AUDIO_PARAM_FLACTYPE flactype_;
    OMX_AUDIO_PARAM_VORBISTYPE vorbistype_;
    OMX_AUDIO_PARAM_AACPROFILETYPE aactype_;
    OMX_VIDEO_PARAM_VP8TYPE vp8type_;
    TagLib::FileRef meta_file_;
    std::string stream_title_;
    std::string stream_genre_;
    bool stream_is_cbr_;
  };
}  // namespace tiz

#endif  // TIZPROBE_HPP

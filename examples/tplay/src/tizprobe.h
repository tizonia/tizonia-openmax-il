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
 * @file   tizprobe.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  File probing utility
 *
 *
 */

#ifndef TIZPROBE_H
#define TIZPROBE_H

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <OMX_TizoniaExt.h>

#include <string>
#include <boost/shared_ptr.hpp>

class tizprobe;
class AVCodecContext;
typedef boost::shared_ptr<tizprobe> tizprobe_ptr_t;

class tizprobe
{

public:

  tizprobe(const std::string &uri, const bool quiet = false);

  std::string get_uri ()
  { return uri_; }

  OMX_PORTDOMAINTYPE get_omx_domain ();

  OMX_AUDIO_CODINGTYPE get_audio_coding_type () const
  { return audio_coding_type_; }

  OMX_VIDEO_CODINGTYPE get_video_coding_type () const
  { return video_coding_type_; }

  void get_pcm_codec_info(OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype);
  void get_mp3_codec_info(OMX_AUDIO_PARAM_MP3TYPE &mp3type);
  void get_opus_codec_info(OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE &opustype);
  void get_flac_codec_info(OMX_TIZONIA_AUDIO_PARAM_FLACTYPE &flactype);
  void get_vp8_codec_info(OMX_VIDEO_PARAM_VP8TYPE &vp8type);
  std::string get_stream_title ();
  std::string get_stream_genre ();

private:

  int probe_file();
  void set_mp3_codec_info (const AVCodecContext *cc);
  void set_opus_codec_info ();
  void set_flac_codec_info (const AVCodecContext *cc);

private:

  std::string uri_;
  bool quiet_; // this is to control whether the probe object should dump any
               // format info to the stdout
  OMX_PORTDOMAINTYPE domain_;
  OMX_AUDIO_CODINGTYPE audio_coding_type_;
  OMX_VIDEO_CODINGTYPE video_coding_type_;
  OMX_AUDIO_PARAM_PCMMODETYPE pcmtype_;
  OMX_AUDIO_PARAM_MP3TYPE mp3type_;
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype_;
  OMX_TIZONIA_AUDIO_PARAM_FLACTYPE flactype_;
  OMX_VIDEO_PARAM_VP8TYPE vp8type_;
  std::string stream_title_;
  std::string stream_genre_;
};

#endif // TIZPROBE_H

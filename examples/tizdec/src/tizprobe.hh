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
 * @file   tizprobe.hh
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  File probing utility
 *
 *
 */

#ifndef TIZPROBE_HH
#define TIZPROBE_HH

#include <string>
#include <boost/shared_ptr.hpp>

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Audio.h"
#include "OMX_Video.h"

class tizprobe;
typedef boost::shared_ptr<tizprobe> tizprobe_ptr_t;

class tizprobe
{

public:

  tizprobe(const std::string &uri);

  std::string get_uri ()
  { return uri_; }

  OMX_PORTDOMAINTYPE get_omx_domain ();

  OMX_AUDIO_CODINGTYPE get_audio_coding_type () const
  { return audio_coding_type_; }

  OMX_VIDEO_CODINGTYPE get_video_coding_type () const
  { return video_coding_type_; }

  void get_mp3_codec_info(OMX_AUDIO_PARAM_MP3TYPE &mp3type);
  void get_vp8_codec_info(OMX_VIDEO_PARAM_VP8TYPE &vp8type);

private:

  int probe_file();

private:

  std::string uri_;
  OMX_PORTDOMAINTYPE domain_;
  OMX_AUDIO_CODINGTYPE audio_coding_type_;
  OMX_VIDEO_CODINGTYPE video_coding_type_;
  OMX_AUDIO_PARAM_MP3TYPE mp3type_;
  OMX_VIDEO_PARAM_VP8TYPE vp8type_;
};

#endif // TIZPROBE_HH

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

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Audio.h"

class tizprobe
{

public:

  tizprobe(OMX_STRING file_uri);

  ~tizprobe();

  OMX_PORTDOMAINTYPE get_domain () const
  { return domain_; };

  OMX_AUDIO_CODINGTYPE get_audio_coding_type () const
  { return audio_coding_type_; }

  void get_mp3_codec_info(OMX_AUDIO_PARAM_MP3TYPE &mp3type) const;

private:

  OMX_STRING file_uri_;
  OMX_PORTDOMAINTYPE domain_;
  OMX_AUDIO_CODINGTYPE audio_coding_type_;
  OMX_AUDIO_PARAM_MP3TYPE mp3type_;

};

#endif // TIZPROBE_HH

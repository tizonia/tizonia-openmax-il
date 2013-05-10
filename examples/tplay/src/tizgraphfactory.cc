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
 * @file   tizgraphfactory.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph factory impl
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizgraphfactory.h"
#include "tizprobe.h"
#include "tizmp3graph.h"

#include <assert.h>
#include <boost/make_shared.hpp>


#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.graph.factory"
#endif

tizgraph_ptr_t
tizgraphfactory::create_graph (const std::string & uri)
{
  tizprobe_ptr_t p = boost::make_shared < tizprobe > (uri.c_str (),
                                                      /* quiet = */ true);
  tizgraph_ptr_t null_ptr;
  if (p->get_omx_domain () == OMX_PortDomainAudio
      && p->get_audio_coding_type () == OMX_AUDIO_CodingMP3)
    {
      return boost::make_shared < tizmp3graph > (p);
    }
  //   else if (p->get_omx_domain () == OMX_PortDomainVideo
  //            && p->get_video_coding_type () == OMX_VIDEO_CodingVP8)
  //     {
  //       return boost::make_shared < tizmp3graph > (p);
  //     }
  return null_ptr;
}

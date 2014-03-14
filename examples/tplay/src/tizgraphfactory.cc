/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @brief  OpenMAX IL graph factory implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include "tizprobe.h"
#include "tizmp3graph.h"
#include "tizopusgraph.h"
#include "tizvorbisgraph.h"
#include "tizflacgraph.h"
#include "tizoggflacgraph.h"
#include "tizgraphfactory.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.graph.factory"
#endif

namespace graph = tiz::graph;

tizgraph_ptr_t graph::factory::create_graph (const std::string &uri)
{
  tizprobe_ptr_t p = boost::make_shared< tiz::probe >(uri,
                                                      /* quiet = */ true);
  tizgraph_ptr_t null_ptr;
  if (p->get_omx_domain () == OMX_PortDomainAudio && p->get_audio_coding_type ()
                                                     == OMX_AUDIO_CodingMP3)
  {
    return boost::make_shared< tiz::graph::mp3decoder >();
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingOPUS)
  {
    return boost::make_shared< tiz::graph::opusdecoder >();
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingFLAC)
  {
    std::string extension (
        boost::filesystem::path (uri).extension ().string ());
    if (extension.compare (".oga") == 0 || extension.compare (".ogg") == 0)
    {
      return boost::make_shared< tiz::graph::oggflacdecoder >();
    }
    else
    {
      return boost::make_shared< tiz::graph::flacdecoder >();
    }
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingVORBIS)
  {
    return boost::make_shared< tiz::graph::vorbisdecoder >();
  }
  return null_ptr;
}

std::string graph::factory::coding_type (const std::string &uri)
{
  tizprobe_ptr_t p = boost::make_shared< tiz::probe >(uri,
                                                      /* quiet = */ true);
  tizgraph_ptr_t null_ptr;
  if (p->get_omx_domain () == OMX_PortDomainAudio && p->get_audio_coding_type ()
                                                     == OMX_AUDIO_CodingMP3)
  {
    return std::string ("mp3");
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingOPUS)
  {
    return std::string ("opus");
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingFLAC)
  {
    std::string extension (
        boost::filesystem::path (uri).extension ().string ());
    if (extension.compare (".oga") == 0 || extension.compare (".ogg") == 0)
    {
      return std::string ("oggflac");
    }
    else
    {
      return std::string ("flac");
    }
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingVORBIS)
  {
    return std::string ("vorbis");
  }
  return std::string ();
}

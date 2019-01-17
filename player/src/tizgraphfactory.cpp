/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphfactory.cpp
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

#include "decoders/tizmp3graph.hpp"
#include "decoders/tizaacgraph.hpp"
#include "decoders/tizopusgraph.hpp"
#include "decoders/tizoggopusgraph.hpp"
#include "decoders/tizvorbisgraph.hpp"
#include "decoders/tizflacgraph.hpp"
#include "decoders/tizoggflacgraph.hpp"
#include "decoders/tizpcmgraph.hpp"
#include "decoders/tizmpeggraph.hpp"
#include "tizprobe.hpp"
#include "tizgraphfactory.hpp"

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
  TIZ_LOG (TIZ_PRIORITY_DEBUG, "uri : %s", uri.c_str ());
  TIZ_LOG (TIZ_PRIORITY_DEBUG, "domain : %s",
           tiz_domain_to_str (p->get_omx_domain ()));
  TIZ_LOG (TIZ_PRIORITY_DEBUG, "coding type : %s",
           tiz_audio_coding_to_str (p->get_audio_coding_type ()));
  if (p->get_omx_domain () == OMX_PortDomainAudio
      && p->get_audio_coding_type () == OMX_AUDIO_CodingMP2)
  {
    return boost::make_shared< tiz::graph::mpegdecoder >();
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
      && p->get_audio_coding_type () == OMX_AUDIO_CodingMP3)
  {
    return boost::make_shared< tiz::graph::mp3decoder >();
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingAAC)
  {
    return boost::make_shared< tiz::graph::aacdecoder >();
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingOPUS)
  {
    if (p->get_container_type () == OMX_FORMAT_RAW)
      {
        return boost::make_shared< tiz::graph::oggopusdecoder >();
      }
    else if (p->get_container_type () == OMX_FORMAT_OGG)
      {
        return boost::make_shared< tiz::graph::oggopusdecoder >();
      }
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
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingPCM)
  {
    return boost::make_shared< tiz::graph::pcmdecoder >();
  }
  return null_ptr;
}

std::string graph::factory::coding_type (const std::string &uri)
{
  tizprobe_ptr_t p = boost::make_shared< tiz::probe >(uri,
                                                      /* quiet = */ true);
  tizgraph_ptr_t null_ptr;
  TIZ_LOG (TIZ_PRIORITY_DEBUG, "uri : %s", uri.c_str ());
  TIZ_LOG (TIZ_PRIORITY_DEBUG, "domain : %s",
           tiz_domain_to_str (p->get_omx_domain ()));
  TIZ_LOG (TIZ_PRIORITY_DEBUG, "coding type : %s",
           tiz_audio_coding_to_str (p->get_audio_coding_type ()));
  if (p->get_omx_domain () == OMX_PortDomainAudio
      && p->get_audio_coding_type () == OMX_AUDIO_CodingMP2)
  {
    return std::string ("mp2");
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio && p->get_audio_coding_type ()
           == OMX_AUDIO_CodingMP3)
  {
    return std::string ("mp3");
  }
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingAAC)
  {
    return std::string ("aac");
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
  else if (p->get_omx_domain () == OMX_PortDomainAudio
           && p->get_audio_coding_type () == OMX_AUDIO_CodingPCM)
  {
    return std::string ("pcm");
  }
  return std::string ();
}

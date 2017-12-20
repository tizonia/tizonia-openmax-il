/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   tizoggflacgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL oggflac decoder graph implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_TizoniaExt.h>
#include <tizplatform.h>

#include "tizgraphutil.hpp"
#include "tizgraphconfig.hpp"
#include "tizgraphcmd.hpp"
#include "tizprobe.hpp"
#include "tizoggflacgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.oggflacdecoder"
#endif

namespace graph = tiz::graph;

//
// oggflacdecoder
//
graph::oggflacdecoder::oggflacdecoder ()
  : tiz::graph::decoder ("oggflacdecgraph")
{
}

graph::ops *graph::oggflacdecoder::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.container_demuxer.ogg");
  comp_list.push_back ("OMX.Aratelia.audio_decoder.flac");
  comp_list.push_back (tiz::graph::util::get_default_pcm_renderer ());

  omx_comp_role_lst_t role_list;
  role_list.push_back ("source.container_demuxer.ogg");
  role_list.push_back ("audio_decoder.flac");
  role_list.push_back ("audio_renderer.pcm");

  return new oggflacdecops (this, comp_list, role_list);
}

//
// oggflacdecops
//
graph::oggflacdecops::oggflacdecops (graph *p_graph,
                                     const omx_comp_name_lst_t &comp_lst,
                                     const omx_comp_role_lst_t &role_lst)
  : tiz::graph::decops (p_graph, comp_lst, role_lst),
    need_port_settings_changed_evt_ (false)
{
}

void graph::oggflacdecops::do_disable_comp_ports (const int comp_id, const int port_id)
{
  OMX_U32 demuxers_video_port = 1;
  G_OPS_BAIL_IF_ERROR (util::disable_port (handles_[0], demuxers_video_port),
                       "Unable to disable demuxer's video port.");
  clear_expected_port_transitions ();
  add_expected_port_transition (handles_[0], demuxers_video_port,
                                OMX_CommandPortDisable);
}

void graph::oggflacdecops::do_probe ()
{
  G_OPS_BAIL_IF_ERROR (
      probe_stream (OMX_PortDomainAudio, OMX_AUDIO_CodingFLAC, "OggFLAC",
                    "decode", &tiz::probe::dump_pcm_info),
      "Unable to probe the stream.");
  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_flac_type (
          handles_[1], 0,
          boost::bind (&tiz::probe::get_flac_codec_info, probe_ptr_, _1),
          need_port_settings_changed_evt_),
      "Unable to set OMX_TizoniaIndexParamAudioFlac");
}

bool graph::oggflacdecops::is_port_settings_evt_required () const
{
  return need_port_settings_changed_evt_;
}

bool graph::oggflacdecops::is_disabled_evt_required () const
{
  return true;
}

void graph::oggflacdecops::do_configure ()
{
  G_OPS_BAIL_IF_ERROR (
      util::set_content_uri (handles_[0], probe_ptr_->get_uri ()),
      "Unable to set OMX_IndexParamContentURI");
  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_pcm_mode (
          handles_[2], 0,
          boost::bind (&tiz::probe::get_pcm_codec_info, probe_ptr_, _1)),
      "Unable to set OMX_IndexParamAudioPcm");
}

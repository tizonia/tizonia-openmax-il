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
 * @file   tizflacgraph.cc
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
#include <tizosal.h>

#include "tizgraphutil.h"
#include "tizgraphconfig.h"
#include "tizprobe.h"
#include "tizoggflacgraph.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.oggflacdecoder"
#endif

namespace graph = tiz::graph;

graph::oggflacdecoder::oggflacdecoder () : graph::graph ("oggflacdecoder")
{
}

graph::ops *graph::oggflacdecoder::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.container_demuxer.ogg");
  comp_list.push_back ("OMX.Aratelia.audio_decoder.flac");
  comp_list.push_back ("OMX.Aratelia.audio_renderer_nb.pcm");

  omx_comp_role_lst_t role_list;
  role_list.push_back ("container_demuxer.ogg");
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
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    need_port_settings_changed_evt_ (false)
{
}

void graph::oggflacdecops::do_disable_ports ()
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
  TIZ_LOG (TIZ_PRIORITY_TRACE, "current_file_index_ [%d]...",
           current_file_index_);
  assert (current_file_index_ < file_list_.size ());
  G_OPS_BAIL_IF_ERROR (probe_uri (current_file_index_), "Unable to probe uri.");
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

OMX_ERRORTYPE
graph::oggflacdecops::probe_uri (const int uri_index, const bool quiet)
{
  assert (uri_index < file_list_.size ());

  const std::string &uri = file_list_[uri_index];

  if (!uri.empty ())
  {
    // Probe a new uri
    probe_ptr_.reset ();
    bool quiet_probing = true;
    probe_ptr_ = boost::make_shared<tiz::probe>(uri, quiet_probing);
    if (probe_ptr_->get_omx_domain () != OMX_PortDomainAudio
        || probe_ptr_->get_audio_coding_type () != OMX_AUDIO_CodingFLAC)
    {
      return OMX_ErrorContentURIError;
    }
    if (!quiet)
    {
      tiz::graph::util::dump_graph_info ("OggFLAC", "decode", uri);
      probe_ptr_->dump_stream_metadata ();
      probe_ptr_->dump_pcm_info ();
    }
  }
  return OMX_ErrorNone;
}

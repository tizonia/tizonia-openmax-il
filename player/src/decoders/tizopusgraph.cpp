/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizopusgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL opus decoding graph implementation
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
#include "tizopusgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.opusdecoder"
#endif

namespace graph = tiz::graph;

//
// opusdecoder
//
graph::opusdecoder::opusdecoder ()
  : tiz::graph::decoder ("opusdecgraph")
{
}

graph::ops *graph::opusdecoder::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.container_demuxer.ogg");
  comp_list.push_back ("OMX.Aratelia.audio_decoder.opus");
  comp_list.push_back (tiz::graph::util::get_default_pcm_renderer ());

  omx_comp_role_lst_t role_list;
  role_list.push_back ("source.container_demuxer.ogg");
  role_list.push_back ("audio_decoder.opus");
  role_list.push_back ("audio_renderer.pcm");

  return new opusdecops (this, comp_list, role_list);
}

//
// opusdecops
//
graph::opusdecops::opusdecops (graph *p_graph,
                               const omx_comp_name_lst_t &comp_lst,
                               const omx_comp_role_lst_t &role_lst)
  : tiz::graph::decops (p_graph, comp_lst, role_lst),
    need_port_settings_changed_evt_ (false)
{
}

void graph::opusdecops::do_disable_comp_ports (const int comp_id, const int port_id)
{
  OMX_U32 demuxers_video_port = 1;
  G_OPS_BAIL_IF_ERROR (util::disable_port (handles_[0], demuxers_video_port),
                       "Unable to disable demuxer's video port.");
  clear_expected_port_transitions ();
  add_expected_port_transition (handles_[0], demuxers_video_port,
                                OMX_CommandPortDisable);
}

void graph::opusdecops::do_probe ()
{
  G_OPS_BAIL_IF_ERROR (
      probe_stream (OMX_PortDomainAudio, OMX_AUDIO_CodingOPUS, "opus", "decode",
                    &tiz::probe::dump_pcm_info),
      "Unable to probe the stream.");
  G_OPS_BAIL_IF_ERROR (set_opus_settings (),
                       "Unable to set OMX_TizoniaIndexParamAudioOpus");
}

bool graph::opusdecops::is_port_settings_evt_required () const
{
  return need_port_settings_changed_evt_;
}

bool graph::opusdecops::is_disabled_evt_required () const
{
  return true;
}

void graph::opusdecops::do_configure ()
{
  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_content_uri (handles_[0], probe_ptr_->get_uri ()),
      "Unable to set OMX_IndexParamContentURI");
  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_pcm_mode (
          handles_[2], 0,
          boost::bind (&tiz::probe::get_pcm_codec_info, probe_ptr_, _1)),
      "Unable to set OMX_IndexParamAudioPcm");
}

OMX_ERRORTYPE
graph::opusdecops::set_opus_settings ()
{
  // Retrieve the current opus settings from the decoder's port #0
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype_orig;
  TIZ_INIT_OMX_PORT_STRUCT (opustype_orig, 0 /* port id */);

  tiz_check_omx (OMX_GetParameter (
      handles_[1], static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioOpus),
      &opustype_orig));

  // Set the opus settings on decoder's port #0
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype;
  TIZ_INIT_OMX_PORT_STRUCT (opustype, 0 /* port id */);

  probe_ptr_->get_opus_codec_info (opustype);
  opustype.nPortIndex = 0;
  tiz_check_omx (OMX_SetParameter (
      handles_[1], static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioOpus),
      &opustype));

  // Record whether we need to wait for a port settings change event or not
  // (the decoder output port implements the "slaving" behaviour)
  need_port_settings_changed_evt_
      = ((opustype_orig.nSampleRate != opustype.nSampleRate)
         || (opustype_orig.nChannels != opustype.nChannels));

  return OMX_ErrorNone;
}

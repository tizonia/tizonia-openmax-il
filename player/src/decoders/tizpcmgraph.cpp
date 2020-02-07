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
 * @file   tizpcmgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL pcm decoder graph implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <tizplatform.h>

#include "tizgraphutil.hpp"
#include "tizgraphconfig.hpp"
#include "tizgraphcmd.hpp"
#include "tizprobe.hpp"
#include "tizpcmgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.pcmdecoder"
#endif

namespace graph = tiz::graph;

//
// pcmdecoder
//
graph::pcmdecoder::pcmdecoder ()
  : tiz::graph::decoder ("pcmdecgraph")
{
}

graph::ops *graph::pcmdecoder::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.file_reader.binary");
  comp_list.push_back ("OMX.Aratelia.audio_decoder.pcm");
  comp_list.push_back (tiz::graph::util::get_default_pcm_renderer ());

  omx_comp_role_lst_t role_list;
  role_list.push_back ("audio_reader.binary");
  role_list.push_back ("audio_decoder.pcm");
  role_list.push_back ("audio_renderer.pcm");

  return new pcmdecops (this, comp_list, role_list);
}

//
// pcmdecops
//
graph::pcmdecops::pcmdecops (graph *p_graph,
                             const omx_comp_name_lst_t &comp_lst,
                             const omx_comp_role_lst_t &role_lst)
  : tiz::graph::decops (p_graph, comp_lst, role_lst),
    need_port_settings_changed_evt_ (false)
{
}

void graph::pcmdecops::do_probe ()
{
  G_OPS_BAIL_IF_ERROR (
      probe_stream (OMX_PortDomainAudio, OMX_AUDIO_CodingPCM, "pcm", "decode",
                    &tiz::probe::dump_pcm_info),
      "Unable to probe the stream.");
}

bool graph::pcmdecops::is_port_settings_evt_required () const
{
  return need_port_settings_changed_evt_;
}

void graph::pcmdecops::do_configure ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (
        util::set_content_uri (handles_[0], probe_ptr_->get_uri ()),
        "Unable to set OMX_IndexParamContentURI");
    OMX_ERRORTYPE rc = tiz::graph::util::
        normalize_tunnel_settings< OMX_AUDIO_PARAM_PCMMODETYPE,
                                   OMX_IndexParamAudioPcm >(
            handles_, 1,  // tunneld id, i.e. this is decoder <-> renderer),
            1,            // decoder's output port
            0);           // renderer's input port
    G_OPS_BAIL_IF_ERROR (rc, "Unable to transfer OMX_IndexParamAudioPcm");
    G_OPS_BAIL_IF_ERROR (
        tiz::graph::util::set_pcm_mode (
            handles_[2], 0,
            boost::bind (&tiz::graph::pcmdecops::get_pcm_codec_info, this, _1)),
        "Unable to set OMX_IndexParamAudioPcm");
  }
}

void graph::pcmdecops::get_pcm_codec_info (OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype)
{
  OMX_U32 dec_port_id = 1;
  OMX_AUDIO_PARAM_PCMMODETYPE dec_pcmtype;
  TIZ_INIT_OMX_PORT_STRUCT (dec_pcmtype, dec_port_id);

  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[1], OMX_IndexParamAudioPcm, &dec_pcmtype),
      "Unable to get OMX_IndexParamAudioPcm from decoder");

  assert (probe_ptr_);
  probe_ptr_->get_pcm_codec_info (pcmtype);

  // Ammend the endianness, sign, and interleave config as per the decoder values
  pcmtype.eEndian = dec_pcmtype.eEndian;
  pcmtype.eNumData = dec_pcmtype.eNumData;
  pcmtype.bInterleaved = dec_pcmtype.bInterleaved;
}

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
 * @file   tizmp3graph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL mp3 decoder graph implementation
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
#include "tizmp3graph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.mp3decoder"
#endif

namespace graph = tiz::graph;

//
// mp3decoder
//
graph::mp3decoder::mp3decoder ()
  : tiz::graph::decoder ("mp3decgraph")
{
}

graph::ops *graph::mp3decoder::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.file_reader.binary");
  comp_list.push_back ("OMX.Aratelia.audio_decoder.mp3");
  comp_list.push_back (tiz::graph::util::get_default_pcm_renderer ());

  omx_comp_role_lst_t role_list;
  role_list.push_back ("audio_reader.binary");
  role_list.push_back ("audio_decoder.mp3");
  role_list.push_back ("audio_renderer.pcm");

  return new mp3decops (this, comp_list, role_list);
}

//
// mp3decops
//
graph::mp3decops::mp3decops (graph *p_graph,
                             const omx_comp_name_lst_t &comp_lst,
                             const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    need_port_settings_changed_evt_ (false)
{
}

void graph::mp3decops::do_probe ()
{
  G_OPS_BAIL_IF_ERROR (
      probe_stream (OMX_PortDomainAudio, OMX_AUDIO_CodingMP3, "mp3", "decode",
                    &tiz::probe::dump_mp3_and_pcm_info),
      "Unable to probe the stream.");
  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_mp3_type (
          handles_[1], 0,
          boost::bind (&tiz::probe::get_mp3_codec_info, probe_ptr_, _1),
          need_port_settings_changed_evt_),
      "Unable to set OMX_IndexParamAudioMp3");
}

bool graph::mp3decops::is_port_settings_evt_required () const
{
  return need_port_settings_changed_evt_;
}

bool graph::mp3decops::is_disabled_evt_required () const
{
  return false;
}

void graph::mp3decops::do_configure ()
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
            boost::bind (&tiz::graph::mp3decops::get_pcm_codec_info, this, _1)),
        "Unable to set OMX_IndexParamAudioPcm");
  }
}

void graph::mp3decops::get_pcm_codec_info (OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype)
{
  OMX_U32 dec_port_id = 1;
  OMX_AUDIO_PARAM_PCMMODETYPE dec_pcmtype;
  TIZ_INIT_OMX_PORT_STRUCT (dec_pcmtype, dec_port_id);

  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[1], OMX_IndexParamAudioPcm, &dec_pcmtype),
      "Unable to get OMX_IndexParamAudioPcm from decoder");

  assert (probe_ptr_);
  probe_ptr_->get_pcm_codec_info (pcmtype);

  // Ammend the endianness, sign, and interleave cofig as per the decoder values
  pcmtype.eEndian = dec_pcmtype.eEndian;
  pcmtype.eNumData = dec_pcmtype.eNumData;
  pcmtype.bInterleaved = dec_pcmtype.bInterleaved;
}

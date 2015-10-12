/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizflacgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL flac decoder graph implementation
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
#include "tizflacgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.flacdecoder"
#endif

namespace graph = tiz::graph;

//
// flacdecoder
//
graph::flacdecoder::flacdecoder ()
  : tiz::graph::decoder ("flacdecgraph")
{
}

graph::ops *graph::flacdecoder::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.file_reader.binary");
  comp_list.push_back ("OMX.Aratelia.audio_decoder.flac");
  comp_list.push_back (tiz::graph::util::get_default_pcm_renderer ());

  omx_comp_role_lst_t role_list;
  role_list.push_back ("audio_reader.binary");
  role_list.push_back ("audio_decoder.flac");
  role_list.push_back ("audio_renderer.pcm");

  return new flacdecops (this, comp_list, role_list);
}

//
// flacdecops
//
graph::flacdecops::flacdecops (graph *p_graph,
                               const omx_comp_name_lst_t &comp_lst,
                               const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    need_port_settings_changed_evt_ (false)
{
}

void graph::flacdecops::do_probe ()
{
  G_OPS_BAIL_IF_ERROR (
      probe_stream (OMX_PortDomainAudio, OMX_AUDIO_CodingFLAC, "flac", "decode",
                    &tiz::probe::dump_pcm_info),
      "Unable to probe the stream.");
  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_flac_type (
          handles_[1], 0,
          boost::bind (&tiz::probe::get_flac_codec_info, probe_ptr_, _1),
          need_port_settings_changed_evt_),
      "Unable to set OMX_TizoniaIndexParamAudioFlac");
}

bool graph::flacdecops::is_port_settings_evt_required () const
{
  return need_port_settings_changed_evt_;
}

bool graph::flacdecops::is_disabled_evt_required () const
{
  return false;
}

void graph::flacdecops::do_configure ()
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

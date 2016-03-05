/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizoggopusgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL OGG Opus decoding graph implementation
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
#include "tizoggopusgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.oggopusdecoder"
#endif

namespace graph = tiz::graph;

//
// oggopusdecoder
//
graph::oggopusdecoder::oggopusdecoder ()
  : tiz::graph::decoder ("oggopusdecgraph")
{
}

graph::ops *graph::oggopusdecoder::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.file_reader.binary");
  comp_list.push_back ("OMX.Aratelia.audio_decoder.opusfile.opus");
  comp_list.push_back (tiz::graph::util::get_default_pcm_renderer ());

  omx_comp_role_lst_t role_list;
  role_list.push_back ("audio_reader.binary");
  role_list.push_back ("audio_decoder.opus");
  role_list.push_back ("audio_renderer.pcm");

  return new oggopusdecops (this, comp_list, role_list);
}

//
// oggopusdecops
//
graph::oggopusdecops::oggopusdecops (graph *p_graph,
                               const omx_comp_name_lst_t &comp_lst,
                               const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    need_port_settings_changed_evt_ (false)
{
}

void graph::oggopusdecops::do_probe ()
{
  G_OPS_BAIL_IF_ERROR (
      probe_stream (OMX_PortDomainAudio, OMX_AUDIO_CodingOPUS, "OggOpus", "decode",
                    &tiz::probe::dump_pcm_info),
      "Unable to probe the stream.");
  G_OPS_BAIL_IF_ERROR (set_opus_settings (),
                       "Unable to set OMX_TizoniaIndexParamAudioOpus");
}

bool graph::oggopusdecops::is_port_settings_evt_required () const
{
  return need_port_settings_changed_evt_;
}

bool graph::oggopusdecops::is_disabled_evt_required () const
{
  return false;
}

void graph::oggopusdecops::do_configure ()
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

void graph::oggopusdecops::get_pcm_codec_info (OMX_AUDIO_PARAM_PCMMODETYPE &pcmtype) const
{
  probe_ptr_->get_pcm_codec_info (pcmtype);

  // Ammend the bits per sample value, as the ogg opus decoder produces 32 bit
  // per sample output
  pcmtype.nBitPerSample = 32;
}

OMX_ERRORTYPE
graph::oggopusdecops::set_opus_settings ()
{
  // Retrieve the current opus settings from the decoder's port #0
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype_orig;
  TIZ_INIT_OMX_PORT_STRUCT (opustype_orig, 0 /* port id */);

  tiz_check_omx_err (OMX_GetParameter (
      handles_[1], static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioOpus),
      &opustype_orig));

  // Set the opus settings on decoder's port #0
  OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype;
  TIZ_INIT_OMX_PORT_STRUCT (opustype, 0 /* port id */);

  probe_ptr_->get_opus_codec_info (opustype);
  opustype.nPortIndex = 0;
  tiz_check_omx_err (OMX_SetParameter (
      handles_[1], static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioOpus),
      &opustype));

  // Record whether we need to wait for a port settings change event or not
  // (the decoder output port implements the "slaving" behaviour)
  //
  // TODO: Add bitrate to the list of things that might change. And then remove
  // the probe_stream_hook hack (see function below)
  need_port_settings_changed_evt_
      = ((opustype_orig.nSampleRate != opustype.nSampleRate)
         || (opustype_orig.nChannels != opustype.nChannels));

  return OMX_ErrorNone;
}

bool
graph::oggopusdecops::probe_stream_hook ()
{
  if (probe_ptr_)
    {
      OMX_AUDIO_PARAM_PCMMODETYPE pcmtype;
      probe_ptr_->get_pcm_codec_info (pcmtype);
      // Ammend the bits per sample value, as the ogg opus decoder produces 32 bit
      // per sample output
      // TODO: Remove this hack
      pcmtype.nBitPerSample = 32;
      probe_ptr_->set_pcm_codec_info (pcmtype);
    }
  return true;
}

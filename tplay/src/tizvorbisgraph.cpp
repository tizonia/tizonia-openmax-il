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
 * @file   tizvorbisgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL vorbis decoder graph implementation
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
#include "tizvorbisgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.vorbisdecoder"
#endif

namespace graph = tiz::graph;

graph::vorbisdecoder::vorbisdecoder ()
  : graph::graph ("vorbisdecgraph"),
    fsm_ (boost::msm::back::states_ << tiz::graph::fsm::configuring (&p_ops_)
                                    << tiz::graph::fsm::skipping (&p_ops_),
          &p_ops_)
{
}

graph::ops *graph::vorbisdecoder::do_init ()
{
  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.container_demuxer.ogg");
  comp_list.push_back ("OMX.Aratelia.audio_decoder.vorbis");
  comp_list.push_back ("OMX.Aratelia.audio_renderer.alsa.pcm");

  omx_comp_role_lst_t role_list;
  role_list.push_back ("source.container_demuxer.ogg");
  role_list.push_back ("audio_decoder.vorbis");
  role_list.push_back ("audio_renderer.pcm");

  return new vorbisdecops (this, comp_list, role_list);
}

bool graph::vorbisdecoder::dispatch_cmd (const tiz::graph::cmd *p_cmd)
{
  assert (NULL != p_cmd);

  if (!p_cmd->kill_thread ())
  {
    if (p_cmd->evt ().type () == typeid(tiz::graph::load_evt))
    {
      // Time to start the FSM
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "Starting [%s] fsm...",
               get_graph_name ().c_str ());
      fsm_.start ();
    }

    p_cmd->inject< fsm >(fsm_, tiz::graph::pstate);

    // Check for internal errors produced during the processing of the last
    // event. If any, inject an "internal" error event. This is fatal and shall
    // terminate the state machine.
    if (OMX_ErrorNone != p_ops_->internal_error ())
    {
      fsm_.process_event (tiz::graph::err_evt (p_ops_->internal_error (),
                                               p_ops_->internal_error_msg ()));
    }

    if (fsm_.terminated_)
    {
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "[%s] fsm terminated...",
               get_graph_name ().c_str ());
    }
  }

  return p_cmd->kill_thread ();
}

//
// vorbisdecops
//
graph::vorbisdecops::vorbisdecops (graph *p_graph,
                                   const omx_comp_name_lst_t &comp_lst,
                                   const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    need_port_settings_changed_evt_ (false)
{
}

void graph::vorbisdecops::do_disable_ports ()
{
  OMX_U32 demuxers_video_port = 1;
  G_OPS_BAIL_IF_ERROR (util::disable_port (handles_[0], demuxers_video_port),
                       "Unable to disable demuxer's video port.");
  clear_expected_port_transitions ();
  add_expected_port_transition (handles_[0], demuxers_video_port,
                                OMX_CommandPortDisable);
}

void graph::vorbisdecops::do_probe ()
{
  G_OPS_BAIL_IF_ERROR (
      probe_stream (OMX_PortDomainAudio, OMX_AUDIO_CodingVORBIS, "vorbis",
                    "decode", &tiz::probe::dump_pcm_info),
      "Unable to probe the stream.");
  G_OPS_BAIL_IF_ERROR (set_vorbis_settings (),
                       "Unable to set OMX_IndexParamAudioVorbis");
}

bool graph::vorbisdecops::is_port_settings_evt_required () const
{
  return need_port_settings_changed_evt_;
}

bool graph::vorbisdecops::is_disabled_evt_required () const
{
  return true;
}

void graph::vorbisdecops::do_configure ()
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
graph::vorbisdecops::set_vorbis_settings ()
{
  // Retrieve the current vorbis settings from the decoder's port #0
  OMX_AUDIO_PARAM_VORBISTYPE vorbistype_orig;
  TIZ_INIT_OMX_PORT_STRUCT (vorbistype_orig, 0 /* port id */);

  tiz_check_omx_err (OMX_GetParameter (handles_[1], OMX_IndexParamAudioVorbis,
                                       &vorbistype_orig));

  // Set the vorbis settings on decoder's port #0
  OMX_AUDIO_PARAM_VORBISTYPE vorbistype;
  TIZ_INIT_OMX_PORT_STRUCT (vorbistype, 0 /* port id */);

  probe_ptr_->get_vorbis_codec_info (vorbistype);
  vorbistype.nPortIndex = 0;
  tiz_check_omx_err (
      OMX_SetParameter (handles_[1], OMX_IndexParamAudioVorbis, &vorbistype));

  // Record whether we need to wait for a port settings change event or not
  // (the decoder output port implements the "slaving" behaviour)
  need_port_settings_changed_evt_
      = ((vorbistype_orig.nSampleRate != vorbistype.nSampleRate)
         || (vorbistype_orig.nChannels != vorbistype.nChannels));

  return OMX_ErrorNone;
}

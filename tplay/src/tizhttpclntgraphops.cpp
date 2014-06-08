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
 * @file   tizhttpclntgraphops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Client graph implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_TizoniaExt.h>
#include <tizplatform.h>

#include "tizgraphutil.hpp"
#include "tizprobe.hpp"
#include "tizgraph.hpp"
// #include "tizhttpclntconfig.hpp"
#include "tizhttpclntgraphops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.httpclientops"
#endif

namespace graph = tiz::graph;

//
// httpclntops
//
graph::httpclntops::httpclntops (graph *p_graph,
                                 const omx_comp_name_lst_t &comp_lst,
                                 const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst)
{
}

void graph::httpclntops::do_load_source ()
{
  assert (!comp_lst_.empty ());
  assert (!role_lst_.empty ());
  // At this point we are instantiating a graph with a single component, the
  // http source component.
  assert (comp_lst_.size () == 1);
  tiz::graph::ops::do_load ();
}

void graph::httpclntops::do_enable_auto_detection ()
{
  const int http_source_index = 0;
  assert (handles_.size () == 1);
  G_OPS_BAIL_IF_ERROR (tiz::graph::util::enable_port_format_auto_detection (handles_[0],
                                                                            http_source_index,
                                                                            OMX_PortDomainAudio),
                       "Unable to set OMX_IndexParamPortDefinition (port auto detection)");
}

void graph::httpclntops::do_disable_ports ()
{
  OMX_U32 http_source_port = 0;
  G_OPS_BAIL_IF_ERROR (util::disable_port (handles_[0], http_source_port),
                       "Unable to disable http source's output port.");
  clear_expected_port_transitions ();
  add_expected_port_transition (handles_[0], http_source_port,
                                OMX_CommandPortDisable);
}

void graph::httpclntops::do_configure_source ()
{
  G_OPS_BAIL_IF_ERROR (
      util::set_content_uri (handles_[0], playlist_->get_current_uri ()),
      "Unable to set OMX_IndexParamContentURI");
}

void graph::httpclntops::do_load ()
{
  assert (!comp_lst_.empty ());
  assert (!role_lst_.empty ());

  // At this point we are going to instantiate the two remaining components in
  // the graph, the audio decoder and the pcm renderer. The http source is
  // already instantiated and in Executing state.

  assert (comp_lst_.size () == 1);

  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.audio_decoder.mp3");
  comp_list.push_back ("OMX.Aratelia.audio_renderer.pcm");

  omx_comp_role_lst_t role_list;
  role_list.push_back ("audio_decoder.mp3");
  role_list.push_back ("audio_renderer.pcm");

  tiz::graph::cbackhandler &cbacks = get_cback_handler ();
  G_OPS_BAIL_IF_ERROR (
      util::instantiate_comp_list (comp_list, handles_, h2n_, &(cbacks),
                                   cbacks.get_omx_cbacks ()),
      "Unable to instantiate the component list.");

  // Now add the the new components to the base class lists
  comp_lst_.insert (comp_lst_.begin (), comp_list.begin (), comp_list.end ());
  role_lst_.insert (role_lst_.begin (), role_list.begin (), role_list.end ());
}

void graph::httpclntops::do_configure ()
{
  G_OPS_BAIL_IF_ERROR (
      apply_pcm_codec_info_from_http_source (),
      "Unable to set OMX_IndexParamAudioPcm");
}

void graph::httpclntops::do_omx_exe2pause ()
{
  // No-op. This is to disable pause in this graph
}

void graph::httpclntops::do_omx_pause2exe ()
{
  // No-op. This is to disable pause in this graph
}

void graph::httpclntops::do_volume (const int step)
{
  // No-op. This is to disable volume in this graph
}

void graph::httpclntops::do_mute ()
{
  // No-op. This is to disable mute in this graph
}

void graph::httpclntops::do_disable_tunnel ()
{
  if (last_op_succeeded ())
  {
    const int tunnel_id = 0;  // this is the tunnel source <=> decoder
    G_OPS_BAIL_IF_ERROR (transition_tunnel (tunnel_id, OMX_CommandPortDisable),
                         "Unable to disable tunnel source <=> decoder");
  }
}

void graph::httpclntops::do_omx_loaded2idle ()
{
  if (last_op_succeeded ())
  {
    // Transition the decoder and the renderer components to Idle
    omx_comp_handle_lst_t decoder_and_renderer_handles;
    decoder_and_renderer_handles.push_back (handles_[1]); // the decoder
    decoder_and_renderer_handles.push_back (handles_[2]); // the renderer
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (decoder_and_renderer_handles, OMX_StateIdle, OMX_StateLoaded),
        "Unable to transition decoder and renderer from Loaded->Idle");
    clear_expected_transitions ();
    add_expected_transition (handles_[1], OMX_StateIdle);
    add_expected_transition (handles_[2], OMX_StateIdle);
  }
}

void graph::httpclntops::do_omx_idle2exe ()
{
  if (last_op_succeeded ())
  {
    // Transition the decoder and the renderer components to Exe
    omx_comp_handle_lst_t decoder_and_renderer_handles;
    decoder_and_renderer_handles.push_back (handles_[1]); // the decoder
    decoder_and_renderer_handles.push_back (handles_[2]); // the renderer
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (decoder_and_renderer_handles, OMX_StateExecuting, OMX_StateIdle),
        "Unable to transition decoder and renderer from Idle->Exe");
    clear_expected_transitions ();
    add_expected_transition (handles_[1], OMX_StateExecuting);
    add_expected_transition (handles_[2], OMX_StateExecuting);
  }
}

void graph::httpclntops::do_enable_tunnel ()
{
  if (last_op_succeeded ())
  {
    const int tunnel_id = 0;  // this is the tunnel source <=> decoder
    G_OPS_BAIL_IF_ERROR (transition_tunnel (tunnel_id, OMX_CommandPortEnable),
                         "Unable to enable tunnel source <=> decoder");
  }
}

// TODO: Move this implementation to the base class (and remove also from httpservops)
OMX_ERRORTYPE
graph::httpclntops::transition_source (const OMX_STATETYPE to_state)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const int http_source_index = 0;
  rc = tiz::graph::util::transition_one (handles_, http_source_index, to_state);
  if (OMX_ErrorNone == rc)
  {
    clear_expected_transitions ();
    add_expected_transition (handles_[http_source_index], to_state);
  }
  return rc;
}

// TODO: Move this implementation to the base class (and remove also from httpservops)
OMX_ERRORTYPE
graph::httpclntops::transition_tunnel (const int tunnel_id,
    const OMX_COMMANDTYPE to_disabled_or_enabled)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (to_disabled_or_enabled == OMX_CommandPortDisable
          || to_disabled_or_enabled == OMX_CommandPortEnable);

  if (to_disabled_or_enabled == OMX_CommandPortDisable)
  {
    rc = tiz::graph::util::disable_tunnel (handles_, tunnel_id);
  }
  else
  {
    rc = tiz::graph::util::enable_tunnel (handles_, tunnel_id);
  }

  if (OMX_ErrorNone == rc)
  {
    clear_expected_port_transitions ();
    const int http_source_index = 0;
    const int http_source_input_port = 0;
    add_expected_port_transition (handles_[http_source_index],
                                  http_source_input_port,
                                  to_disabled_or_enabled);
    const int decoder_index = 1;
    const int decoder_input_port = 0;
    add_expected_port_transition (handles_[decoder_index],
                                  decoder_input_port,
                                  to_disabled_or_enabled);
  }
  return rc;
}

bool graph::httpclntops::probe_stream_hook ()
{
  return true;
}

OMX_ERRORTYPE
graph::httpclntops::apply_pcm_codec_info_from_http_source ()
{
  // Retrieve the current mp3 settings from the http source component input port
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  const OMX_U32 port_id = 0;
  TIZ_INIT_OMX_PORT_STRUCT (mp3type, port_id);
  tiz_check_omx_err (OMX_GetParameter (handles_[0], OMX_IndexParamAudioMp3, &mp3type));

  // Retrieve the pcm settings from the renderer component
  OMX_AUDIO_PARAM_PCMMODETYPE renderer_pcmtype;
  TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype, port_id);
  tiz_check_omx_err (
      OMX_GetParameter (handles_[2], OMX_IndexParamAudioPcm, &renderer_pcmtype));

  // Now assign the actual settings to the pcmtype structure
  renderer_pcmtype.nChannels = mp3type.nChannels;
  renderer_pcmtype.nSamplingRate = mp3type.nSampleRate;

  // Set the new pcm settings
  tiz_check_omx_err (
      OMX_SetParameter (handles_[2], OMX_IndexParamAudioPcm, &renderer_pcmtype));
}

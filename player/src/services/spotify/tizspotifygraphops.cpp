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
 * @file   tizspotifygraphops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Spotify client graph implementation
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
#include "tizspotifyconfig.hpp"
#include "tizspotifygraphops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.spotifyops"
#endif

namespace graph = tiz::graph;

//
// spotifyops
//
graph::spotifyops::spotifyops (graph *p_graph,
                               const omx_comp_name_lst_t &comp_lst,
                               const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    encoding_ (OMX_AUDIO_CodingAutoDetect)
{
  TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype_, 0);
}

void graph::spotifyops::do_enable_auto_detection (const int handle_id,
                                                  const int port_id)
{
  tizspotifyconfig_ptr_t spotify_config
      = boost::dynamic_pointer_cast< spotifyconfig >(config_);
  assert (spotify_config);
  tiz::graph::ops::do_enable_auto_detection (handle_id, port_id);
  tiz::graph::util::dump_graph_info ("Spotify", "Connecting",
                                     spotify_config->get_user_name ().c_str ());
}

void graph::spotifyops::do_disable_comp_ports (const int comp_id, const int port_id)
{
  OMX_U32 spotify_source_port = port_id;
  G_OPS_BAIL_IF_ERROR (util::disable_port (handles_[comp_id], spotify_source_port),
                       "Unable to disable spotify source's output port.");
  clear_expected_port_transitions ();
  add_expected_port_transition (handles_[comp_id], spotify_source_port,
                                OMX_CommandPortDisable);
}

void graph::spotifyops::do_configure_comp (const int comp_id)
{
  if (comp_id == 0)
  {
    tizspotifyconfig_ptr_t spotify_config
        = boost::dynamic_pointer_cast< spotifyconfig > (config_);
    assert (spotify_config);

    G_OPS_BAIL_IF_ERROR (
        set_spotify_user_and_pass (handles_[0],
                                   spotify_config->get_user_name (),
                                   spotify_config->get_user_pass ()),
        "Unable to set OMX_TizoniaIndexParamAudioSpotifySession");

    G_OPS_BAIL_IF_ERROR (
        set_spotify_playlist (handles_[0], playlist_->get_current_uri ()),
        "Unable to set OMX_TizoniaIndexParamAudioSpotifyPlaylist");
  }
}

void graph::spotifyops::do_load ()
{
  assert (!comp_lst_.empty ());
  assert (!role_lst_.empty ());

  // At this point we are going to instantiate the remaining component in the
  // graph, the pcm renderer. The spotify source is already instantiated and in
  // Executing state.

  assert (comp_lst_.size () == 1);
  assert (handles_.size () == 1);

  G_OPS_BAIL_IF_ERROR (
      get_encoding_type_from_spotify_source (),
      "Unable to retrieve the audio encoding from the spotify source.");

  omx_comp_name_lst_t comp_list;
  omx_comp_role_lst_t role_list;

  comp_list.push_back (tiz::graph::util::get_default_pcm_renderer ());
  role_list.push_back ("audio_renderer.pcm");

  tiz::graph::cbackhandler &cbacks = get_cback_handler ();
  G_OPS_BAIL_IF_ERROR (
      util::instantiate_comp_list (comp_list, handles_, h2n_, &(cbacks),
                                   cbacks.get_omx_cbacks ()),
      "Unable to instantiate the component list.");

  // Now add the new components to the base class lists
  comp_lst_.insert (comp_lst_.begin (), comp_list.begin (), comp_list.end ());
  role_lst_.insert (role_lst_.begin (), role_list.begin (), role_list.end ());
}

void graph::spotifyops::do_configure ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (apply_pcm_codec_info_from_spotify_source (),
                         "Unable to set OMX_IndexParamAudioPcm");
  }
}

void graph::spotifyops::do_loaded2idle ()
{
  if (last_op_succeeded ())
  {
    // Transition the renderer component to Idle
    omx_comp_handle_lst_t renderer_handle;
    renderer_handle.push_back (handles_[1]);  // the renderer
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (renderer_handle, OMX_StateIdle, OMX_StateLoaded),
        "Unable to transition renderer from Loaded->Idle");
    clear_expected_transitions ();
    add_expected_transition (handles_[1], OMX_StateIdle);
  }
}

void graph::spotifyops::do_idle2exe ()
{
  if (last_op_succeeded ())
  {
    // Transition the renderer component to Exe
    omx_comp_handle_lst_t renderer_handle;
    renderer_handle.push_back (handles_[1]);  // the renderer
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (renderer_handle, OMX_StateExecuting,
                              OMX_StateIdle),
        "Unable to transition decoder and renderer from Idle->Exe");
    clear_expected_transitions ();
    add_expected_transition (handles_[1], OMX_StateExecuting);
  }
}

void graph::spotifyops::do_reconfigure_tunnel (const int tunnel_id)
{
  if (last_op_succeeded ())
  {
    assert (0 == tunnel_id);    // Only one tunnel in this graph
    // Retrieve the pcm settings from the source component
    OMX_AUDIO_PARAM_PCMMODETYPE spotify_pcmtype;
    const OMX_U32 spotify_port_id = 0;
    TIZ_INIT_OMX_PORT_STRUCT (spotify_pcmtype, spotify_port_id);
    G_OPS_BAIL_IF_ERROR (
        OMX_GetParameter (handles_[0], OMX_IndexParamAudioPcm,
                          &spotify_pcmtype),
        "Unable to retrieve the PCM settings from the Spotify source");

    // Retrieve the pcm settings from the renderer component
    OMX_AUDIO_PARAM_PCMMODETYPE renderer_pcmtype;
    const OMX_U32 renderer_port_id = 0;
    TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype, renderer_port_id);
    G_OPS_BAIL_IF_ERROR (
        OMX_GetParameter (handles_[1], OMX_IndexParamAudioPcm,
                          &renderer_pcmtype),
        "Unable to retrieve the PCM settings from the pcm renderer");

    // Now assign the current settings to the renderer structure
    renderer_pcmtype.nChannels = spotify_pcmtype.nChannels;
    renderer_pcmtype.nSamplingRate = spotify_pcmtype.nSamplingRate;

    // Set the new pcm settings
    G_OPS_BAIL_IF_ERROR (
        OMX_SetParameter (handles_[1], OMX_IndexParamAudioPcm,
                          &renderer_pcmtype),
        "Unable to set the PCM settings on the audio renderer");

    TIZ_PRINTF_MAG (
        "     %ld Ch, %g KHz, %lu:%s:%s\n", renderer_pcmtype.nChannels,
        ((float)renderer_pcmtype.nSamplingRate) / 1000,
        renderer_pcmtype.nBitPerSample,
        renderer_pcmtype.eNumData == OMX_NumericalDataSigned ? "s" : "u",
        renderer_pcmtype.eEndian == OMX_EndianBig ? "b" : "l");
  }
}

void graph::spotifyops::do_skip ()
{
  if (last_op_succeeded () && 0 != jump_)
  {
    assert (!handles_.empty ());
    G_OPS_BAIL_IF_ERROR (util::apply_playlist_jump (handles_[0], jump_),
                         "Unable to skip in playlist");
    // Reset the jump value, to its default value
    jump_ = SKIP_DEFAULT_VALUE;
  }
}

void graph::spotifyops::do_retrieve_metadata ()
{
  OMX_U32 index = 0;
  const int spotify_source_index = 0;
  while (OMX_ErrorNone == dump_metadata_item (index++, spotify_source_index))
  {
  };

  TIZ_PRINTF_MAG (
      "     %ld Ch, %g KHz, %lu:%s:%s \n", renderer_pcmtype_.nChannels,
      ((float)renderer_pcmtype_.nSamplingRate) / 1000,
      renderer_pcmtype_.nBitPerSample,
      renderer_pcmtype_.eNumData == OMX_NumericalDataSigned ? "s" : "u",
      renderer_pcmtype_.eEndian == OMX_EndianBig ? "b" : "l");
}

// TODO: Move this implementation to the base class (and remove also from
// httpservops)
OMX_ERRORTYPE
graph::spotifyops::switch_tunnel (
    const int tunnel_id, const OMX_COMMANDTYPE to_disabled_or_enabled)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (0 == tunnel_id);
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

  if (OMX_ErrorNone == rc && 0 == tunnel_id)
  {
    clear_expected_port_transitions ();
    const int spotify_source_index = 0;
    const int spotify_source_output_port = 0;
    add_expected_port_transition (handles_[spotify_source_index],
                                  spotify_source_output_port,
                                  to_disabled_or_enabled);
    const int renderer_index = 1;
    const int renderer_input_port = 0;
    add_expected_port_transition (handles_[renderer_index], renderer_input_port,
                                  to_disabled_or_enabled);
  }
  return rc;
}

bool graph::spotifyops::probe_stream_hook ()
{
  return true;
}

OMX_ERRORTYPE graph::spotifyops::get_encoding_type_from_spotify_source ()
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  const OMX_U32 port_id = 0;
  TIZ_INIT_OMX_PORT_STRUCT (port_def, port_id);
  tiz_check_omx (
      OMX_GetParameter (handles_[0], OMX_IndexParamPortDefinition, &port_def));
  encoding_ = port_def.format.audio.eEncoding;
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::spotifyops::apply_pcm_codec_info_from_spotify_source ()
{
  OMX_U32 channels = 2;
  OMX_U32 sampling_rate = 44100;
  std::string encoding_str;

  tiz_check_omx (get_channels_and_rate_from_spotify_source (
      channels, sampling_rate, encoding_str));
  tiz_check_omx (set_channels_and_rate_on_renderer (channels, sampling_rate,
                                                        encoding_str));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::spotifyops::get_channels_and_rate_from_spotify_source (
    OMX_U32 &channels, OMX_U32 &sampling_rate, std::string &encoding_str) const
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_HANDLETYPE handle = handles_[0];  // spotify source's handle
  const OMX_U32 port_id = 0;                  // spotify source's output port

  switch (encoding_)
  {
    case OMX_AUDIO_CodingPCM:
    {
      encoding_str = "pcm";
      rc = tiz::graph::util::
          get_channels_and_rate_from_audio_port_v2< OMX_AUDIO_PARAM_PCMMODETYPE >(
              handle, port_id, OMX_IndexParamAudioPcm, channels, sampling_rate);
    }
    break;
    default:
    {
      assert (0);
    }
    break;
  };

  return rc;
}

OMX_ERRORTYPE
graph::spotifyops::set_channels_and_rate_on_renderer (
    const OMX_U32 channels, const OMX_U32 sampling_rate,
    const std::string encoding_str)
{
  const OMX_HANDLETYPE handle = handles_[1];  // renderer's handle
  const OMX_U32 port_id = 0;                  // renderer's input port

  TIZ_LOG (TIZ_PRIORITY_TRACE, "channels = [%d] sampling_rate = [%d]", channels,
           sampling_rate);

  // Retrieve the pcm settings from the renderer component
  TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype_, port_id);
  tiz_check_omx (
      OMX_GetParameter (handle, OMX_IndexParamAudioPcm, &renderer_pcmtype_));

  // Now assign the actual settings to the pcmtype structure
  renderer_pcmtype_.nChannels = channels;
  renderer_pcmtype_.nSamplingRate = sampling_rate;
  renderer_pcmtype_.eNumData = OMX_NumericalDataSigned;
  renderer_pcmtype_.eEndian
      = (encoding_ == OMX_AUDIO_CodingMP3 ? OMX_EndianBig : OMX_EndianLittle);

  // Set the new pcm settings
  tiz_check_omx (
      OMX_SetParameter (handle, OMX_IndexParamAudioPcm, &renderer_pcmtype_));

  std::string coding_type_str ("Spotify");
  tiz::graph::util::dump_graph_info (coding_type_str.c_str (),
                                     "Connected",
                                     playlist_->get_current_uri ().c_str ());
  do_retrieve_metadata ();

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::spotifyops::set_spotify_user_and_pass (const OMX_HANDLETYPE handle,
                                              const std::string &user,
                                              const std::string &pass)
{
  // Set the Spotify user and pass
  OMX_TIZONIA_AUDIO_PARAM_SPOTIFYSESSIONTYPE sessiontype;
  TIZ_INIT_OMX_STRUCT (sessiontype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioSpotifySession),
      &sessiontype));
  tiz::graph::util::copy_omx_string (sessiontype.cUserName, user);
  tiz::graph::util::copy_omx_string (sessiontype.cUserPassword, pass);
  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioSpotifySession),
      &sessiontype);
}

OMX_ERRORTYPE
graph::spotifyops::set_spotify_playlist (const OMX_HANDLETYPE handle,
                                         const std::string &playlist)
{
  // Set the Spotify playlist
  OMX_TIZONIA_AUDIO_PARAM_SPOTIFYPLAYLISTTYPE playlisttype;
  TIZ_INIT_OMX_STRUCT (playlisttype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioSpotifyPlaylist),
      &playlisttype));
  tiz::graph::util::copy_omx_string (playlisttype.cPlaylistName, playlist);
  playlisttype.bShuffle = playlist_->shuffle ();
  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioSpotifyPlaylist),
      &playlisttype);
}

bool
graph::spotifyops::is_fatal_error (const OMX_ERRORTYPE error) const
{
  bool rc = false;
  TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] ", tiz_err_to_str (error));
  if (error == error_code_)
    {
      // if this error is already being handled, then ignore it.
      rc = false;
    }
  else
    {
      rc |= tiz::graph::ops::is_fatal_error (error);
      rc |= (OMX_ErrorContentURIError == error);
    }
  return rc;
}

void graph::spotifyops::do_record_fatal_error (const OMX_HANDLETYPE handle,
                                               const OMX_ERRORTYPE error,
                                               const OMX_U32 port)
{
  tiz::graph::ops::do_record_fatal_error (handle, error, port);
  if (error == OMX_ErrorContentURIError)
    {
      error_msg_.append ("\n [Playlist not found]");
    }
}

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
 * @file   tizchromecastgraphops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Chromecast rendering graph actions / operations implementation
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
#include "tizchromecastconfig.hpp"
#include "tizchromecastgraphops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.chromecast.ops"
#endif

namespace graph = tiz::graph;

namespace
{
  void copy_omx_string (OMX_U8 *p_dest, const std::string &omx_string,
                        const size_t max_length = OMX_MAX_STRINGNAME_SIZE)
  {
    const size_t len = omx_string.length ();
    const size_t to_copy = MIN (len, max_length - 1);
    assert (p_dest);
    memcpy (p_dest, omx_string.c_str (), to_copy);
    p_dest[to_copy] = '\0';
  }
}

//
// chromecastops
//
graph::chromecastops::chromecastops (graph *p_graph,
                             const omx_comp_name_lst_t &comp_lst,
                             const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    encoding_ (OMX_AUDIO_CodingAutoDetect)
{
  TIZ_INIT_OMX_PORT_STRUCT (decoder_mp3type_, 0);
  TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype_, 0);
}

void graph::chromecastops::do_configure_comp (const int comp_id)
{
  if (comp_id == 0)
  {
    tizchromecastconfig_ptr_t chromecast_config
        = boost::dynamic_pointer_cast< chromecastconfig > (config_);
    assert (chromecast_config);

    G_OPS_BAIL_IF_ERROR (
        set_chromecast_user_and_device_id (
            handles_[0], chromecast_config->get_user_name (),
            chromecast_config->get_user_pass (), chromecast_config->get_device_id ()),
        "Unable to set OMX_TizoniaIndexParamAudioGmusicSession");

    G_OPS_BAIL_IF_ERROR (
        set_chromecast_playlist (handles_[0], playlist_->get_current_uri ()),
        "Unable to set OMX_TizoniaIndexParamAudioGmusicPlaylist");
  }
}

void graph::chromecastops::do_load ()
{
  assert (!comp_lst_.empty ());
  assert (!role_lst_.empty ());

  // At this point we are going to instantiate the remaining component in the
  // graph, the audio decoder and the pcm renderer. The chromecast source is already
  // instantiated and in
  // Executing state.

  assert (comp_lst_.size () == 1);
  assert (handles_.size () == 1);

  G_OPS_BAIL_IF_ERROR (
      get_encoding_type_from_chromecast_source (),
      "Unable to retrieve the audio encoding from the chromecast source.");

  omx_comp_name_lst_t comp_list;
  omx_comp_role_lst_t role_list;

  comp_list.push_back ("OMX.Aratelia.audio_decoder.mp3");
  role_list.push_back ("audio_decoder.mp3");

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

void graph::chromecastops::do_configure ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (override_decoder_and_renderer_sampling_rates (),
                         "Unable to override decoder/renderer sampling rates");
    G_OPS_BAIL_IF_ERROR (apply_pcm_codec_info_from_decoder (),
                         "Unable to set OMX_IndexParamAudioPcm");
  }
}

void graph::chromecastops::do_loaded2idle ()
{
  if (last_op_succeeded ())
  {
    // Transition the decoder and the renderer components to Idle
    omx_comp_handle_lst_t decoder_and_renderer_handles;
    decoder_and_renderer_handles.push_back (handles_[1]);  // the decoder
    decoder_and_renderer_handles.push_back (handles_[2]);  // the renderer
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (decoder_and_renderer_handles, OMX_StateIdle,
                              OMX_StateLoaded),
        "Unable to transition deoder and renderer from Loaded->Idle");
    clear_expected_transitions ();
    add_expected_transition (handles_[1], OMX_StateIdle);
    add_expected_transition (handles_[2], OMX_StateIdle);
  }
}

void graph::chromecastops::do_idle2exe ()
{
  if (last_op_succeeded ())
  {
    // Transition the decoder and the renderer components to Exe
    omx_comp_handle_lst_t decoder_and_renderer_handles;
    decoder_and_renderer_handles.push_back (handles_[1]);  // the decoder
    decoder_and_renderer_handles.push_back (handles_[2]);  // the renderer
    G_OPS_BAIL_IF_ERROR (
        util::transition_all (decoder_and_renderer_handles, OMX_StateExecuting,
                              OMX_StateIdle),
        "Unable to transition decoder and renderer from Idle->Exe");
    clear_expected_transitions ();
    add_expected_transition (handles_[1], OMX_StateExecuting);
    add_expected_transition (handles_[2], OMX_StateExecuting);
  }
}

void graph::chromecastops::do_reconfigure_tunnel (const int tunnel_id)
{
  if (last_op_succeeded ())
  {
    if (0 == tunnel_id)
    {
      do_reconfigure_first_tunnel ();
    }
    else if (1 == tunnel_id)
    {
      do_reconfigure_second_tunnel ();
    }
    else
    {
      assert (0);
    }
  }
}

void graph::chromecastops::do_skip ()
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

void graph::chromecastops::do_retrieve_metadata ()
{
  OMX_U32 index = 0;
  const int chromecast_index = 0;
  // Extract metadata from the chromecast source
  while (OMX_ErrorNone == dump_metadata_item (index++, chromecast_index))
  {
  };

  // Now extract metadata from the decoder
  const int decoder_index = 1;
  index = 0;
  const bool use_first_as_heading = false;
  while (OMX_ErrorNone
         == dump_metadata_item (index++, decoder_index, use_first_as_heading))
  {
  };

  OMX_GetParameter (handles_[2], OMX_IndexParamAudioPcm, &renderer_pcmtype_);

  // Now print renderer metadata
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
graph::chromecastops::switch_tunnel (
    const int tunnel_id, const OMX_COMMANDTYPE to_disabled_or_enabled)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (0 == tunnel_id || 1 == tunnel_id);
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
    const int chromecast_source_index = 0;
    const int chromecast_source_output_port = 0;
    add_expected_port_transition (handles_[chromecast_source_index],
                                  chromecast_source_output_port,
                                  to_disabled_or_enabled);
    const int decoder_index = 1;
    const int decoder_input_port = 0;
    add_expected_port_transition (handles_[decoder_index], decoder_input_port,
                                  to_disabled_or_enabled);
  }
  else if (OMX_ErrorNone == rc && 1 == tunnel_id)
  {
    const int decoder_index = 1;
    const int decoder_output_port = 1;
    add_expected_port_transition (handles_[decoder_index], decoder_output_port,
                                  to_disabled_or_enabled);
    const int renderer_index = 2;
    const int renderer_input_port = 0;
    add_expected_port_transition (handles_[renderer_index], renderer_input_port,
                                  to_disabled_or_enabled);
  }
  return rc;
}

bool graph::chromecastops::probe_stream_hook ()
{
  return true;
}

OMX_ERRORTYPE graph::chromecastops::get_encoding_type_from_chromecast_source ()
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
graph::chromecastops::override_decoder_and_renderer_sampling_rates ()
{
  OMX_U32 channels = 2;
  OMX_U32 sampling_rate = 44100;
  tiz_check_omx (
      set_channels_and_rate_on_decoder (channels, sampling_rate));
  return set_channels_and_rate_on_renderer (channels, sampling_rate);
}

OMX_ERRORTYPE
graph::chromecastops::apply_pcm_codec_info_from_decoder ()
{
  OMX_U32 channels = 2;
  OMX_U32 sampling_rate = 44100;
  std::string encoding_str;

  tiz_check_omx (get_channels_and_rate_from_decoder (
      channels, sampling_rate, encoding_str));
  return set_channels_and_rate_on_renderer (channels, sampling_rate);
}

OMX_ERRORTYPE
graph::chromecastops::get_channels_and_rate_from_decoder (
    OMX_U32 &channels, OMX_U32 &sampling_rate, std::string &encoding_str) const
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_HANDLETYPE handle = handles_[1];  // mp3 decoder's handle
  const OMX_U32 port_id = 1;                  // mp3 decoder's output port

  switch (encoding_)
  {
    case OMX_AUDIO_CodingMP3:
    {
      encoding_str = "mp3";
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

  TIZ_LOG (TIZ_PRIORITY_TRACE, "outcome = [%s]", tiz_err_to_str (rc));

  return rc;
}

OMX_ERRORTYPE
graph::chromecastops::set_channels_and_rate_on_decoder (
    const OMX_U32 channels, const OMX_U32 sampling_rate)
{
  const OMX_HANDLETYPE handle = handles_[1];  // decoder's handle
  const OMX_U32 port_id = 0;                  // decoder's input port

  TIZ_LOG (TIZ_PRIORITY_TRACE, "channels = [%d] sampling_rate = [%d]", channels,
           sampling_rate);

  // Retrieve the mp3 settings from the decoder component
  TIZ_INIT_OMX_PORT_STRUCT (decoder_mp3type_, port_id);
  tiz_check_omx (
      OMX_GetParameter (handle, OMX_IndexParamAudioMp3, &decoder_mp3type_));

  TIZ_LOG (TIZ_PRIORITY_TRACE, "channels = [%d] sampling_rate = [%d]", channels,
           sampling_rate);

  // Now assign the actual settings to the pcmtype structure
  decoder_mp3type_.nChannels = channels;
  decoder_mp3type_.nSampleRate = sampling_rate;

  // Set the new mp3 settings
  tiz_check_omx (
      OMX_SetParameter (handle, OMX_IndexParamAudioMp3, &decoder_mp3type_));

  TIZ_LOG (TIZ_PRIORITY_TRACE, "channels = [%d] sampling_rate = [%d]", channels,
           sampling_rate);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::chromecastops::set_channels_and_rate_on_renderer (
    const OMX_U32 channels, const OMX_U32 sampling_rate)
{
  const OMX_HANDLETYPE handle = handles_[2];  // renderer's handle
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

  tizchromecastconfig_ptr_t chromecast_config
    = boost::dynamic_pointer_cast< chromecastconfig >(config_);
  assert (chromecast_config);

  std::string coding_type_str ("Google Play Music");
  tiz::graph::util::dump_graph_info (coding_type_str.c_str (),
                                     "Connected",
                                     chromecast_config->get_user_name ().c_str ());

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::chromecastops::set_chromecast_user_and_device_id (const OMX_HANDLETYPE handle,
                                                 const std::string &user,
                                                 const std::string &pass,
                                                 const std::string &device_id)
{
  // Set the Google Play Music user and pass
  OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE sessiontype;
  TIZ_INIT_OMX_STRUCT (sessiontype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioGmusicSession),
      &sessiontype));
  copy_omx_string (sessiontype.cUserName, user);
  copy_omx_string (sessiontype.cUserPassword, pass);
  copy_omx_string (sessiontype.cDeviceId, device_id);
  return OMX_SetParameter (handle, static_cast< OMX_INDEXTYPE >(
                                       OMX_TizoniaIndexParamAudioGmusicSession),
                           &sessiontype);
}

OMX_ERRORTYPE
graph::chromecastops::set_chromecast_playlist (const OMX_HANDLETYPE handle,
                                       const std::string &playlist)
{
  // Set the Google Play Music playlist
  OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE playlisttype;
  TIZ_INIT_OMX_STRUCT (playlisttype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioGmusicPlaylist),
      &playlisttype));
  copy_omx_string (playlisttype.cPlaylistName, playlist);

  tizchromecastconfig_ptr_t chromecast_config
    = boost::dynamic_pointer_cast< chromecastconfig >(config_);
  assert (chromecast_config);

  playlisttype.ePlaylistType = chromecast_config->get_playlist_type ();
  playlisttype.bShuffle = playlist_->shuffle () ? OMX_TRUE : OMX_FALSE;
  playlisttype.bUnlimitedSearch = chromecast_config->is_unlimited_search () ? OMX_TRUE : OMX_FALSE;

  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioGmusicPlaylist),
      &playlisttype);
}

bool graph::chromecastops::is_fatal_error (const OMX_ERRORTYPE error) const
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

void graph::chromecastops::do_record_fatal_error (const OMX_HANDLETYPE handle,
                                              const OMX_ERRORTYPE error,
                                              const OMX_U32 port)
{
  tiz::graph::ops::do_record_fatal_error (handle, error, port);
  if (error == OMX_ErrorContentURIError)
  {
    error_msg_.append ("\n [Playlist not found]");
  }
}

void graph::chromecastops::do_reconfigure_first_tunnel ()
{
  // Retrieve the mp3 settings from the chromecast source component
  OMX_AUDIO_PARAM_MP3TYPE chromecast_mp3type;
  const OMX_U32 chromecast_port_id = 0;
  TIZ_INIT_OMX_PORT_STRUCT (chromecast_mp3type, chromecast_port_id);
  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[0], OMX_IndexParamAudioMp3, &chromecast_mp3type),
      "Unable to retrieve the MP3 settings from the chromecast source");

  // Retrieve the mp3 settings from the decoder component
  OMX_AUDIO_PARAM_MP3TYPE decoder_mp3type;
  const OMX_U32 decoder_port_id = 0;
  TIZ_INIT_OMX_PORT_STRUCT (decoder_mp3type, decoder_port_id);
  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[1], OMX_IndexParamAudioMp3, &decoder_mp3type),
      "Unable to retrieve the MP3 settings from the audio decoder");

  // Now assign the current settings to the decoder structure
  decoder_mp3type.nChannels = chromecast_mp3type.nChannels;
  decoder_mp3type.nSampleRate = chromecast_mp3type.nSampleRate;

  // Set the new mp3 settings
  G_OPS_BAIL_IF_ERROR (
      OMX_SetParameter (handles_[1], OMX_IndexParamAudioMp3, &decoder_mp3type),
      "Unable to set the MP3 settings on the audio decoder");
}

void graph::chromecastops::do_reconfigure_second_tunnel ()
{
  // Retrieve the pcm settings from the decoder component
  OMX_AUDIO_PARAM_PCMMODETYPE decoder_pcmtype;
  const OMX_U32 decoder_port_id = 1;
  TIZ_INIT_OMX_PORT_STRUCT (decoder_pcmtype, decoder_port_id);
  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[1], OMX_IndexParamAudioPcm, &decoder_pcmtype),
      "Unable to retrieve the PCM settings from the Mp3 decoder");

  // Retrieve the pcm settings from the renderer component
  OMX_AUDIO_PARAM_PCMMODETYPE renderer_pcmtype;
  const OMX_U32 renderer_port_id = 0;
  TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype, renderer_port_id);
  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[2], OMX_IndexParamAudioPcm, &renderer_pcmtype),
      "Unable to retrieve the PCM settings from the pcm renderer");

  // Now assign the current settings to the renderer structure
  renderer_pcmtype.nChannels = decoder_pcmtype.nChannels;
  renderer_pcmtype.nSamplingRate = decoder_pcmtype.nSamplingRate;

  // Set the new pcm settings
  G_OPS_BAIL_IF_ERROR (
      OMX_SetParameter (handles_[2], OMX_IndexParamAudioPcm, &renderer_pcmtype),
      "Unable to set the PCM settings on the audio renderer");

  TIZ_PRINTF_MAG (
      "     %ld Ch, %g KHz, %lu:%s:%s\n", renderer_pcmtype.nChannels,
      ((float)renderer_pcmtype.nSamplingRate) / 1000,
      renderer_pcmtype.nBitPerSample,
      renderer_pcmtype.eNumData == OMX_NumericalDataSigned ? "s" : "u",
      renderer_pcmtype.eEndian == OMX_EndianBig ? "b" : "l");
}

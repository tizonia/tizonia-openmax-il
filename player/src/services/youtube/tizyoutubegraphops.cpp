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
 * @file   tizyoutubegraphops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Youtube client graph actions / operations implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_TizoniaExt.h>
#include <tizplatform.h>

#include "tizgraph.hpp"
#include "tizgraphutil.hpp"
#include "tizprobe.hpp"
#include "tizyoutubeconfig.hpp"
#include "tizyoutubegraphops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.youtube.ops"
#endif

namespace graph = tiz::graph;

namespace
{
  void copy_omx_string (OMX_U8 *p_dest, const std::string &omx_string)
  {
    const size_t len = omx_string.length ();
    const size_t to_copy = MIN (len, OMX_MAX_STRINGNAME_SIZE - 1);
    assert (p_dest);
    memcpy (p_dest, omx_string.c_str (), to_copy);
    p_dest[to_copy] = '\0';
  }
}

//
// youtubeops
//
graph::youtubeops::youtubeops (graph *p_graph,
                               const omx_comp_name_lst_t &comp_lst,
                               const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    encoding_ (OMX_AUDIO_CodingAutoDetect),
    container_ (OMX_AUDIO_CodingAutoDetect),
    inital_graph_load_ (false)
{
  TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype_, 0);
}

void graph::youtubeops::do_enable_auto_detection (const int handle_id,
                                                  const int port_id)
{
  tiz::graph::ops::do_enable_auto_detection (handle_id, port_id);
}

void graph::youtubeops::do_disable_comp_ports (const int comp_id,
                                               const int port_id)
{
  if (comp_id == 0)
  {
    // Disable the http source's input port
    OMX_U32 youtube_source_port = 0;
    G_OPS_BAIL_IF_ERROR (
        util::disable_port (handles_[comp_id], youtube_source_port),
        "Unable to disable youtube source's output port.");
    clear_expected_port_transitions ();
    add_expected_port_transition (handles_[comp_id], youtube_source_port,
                                  OMX_CommandPortDisable);
  }
  else if (comp_id == 1)
  {
    // Disable the demuxer's input port
    OMX_U32 demuxer_input_port = 0;
    G_OPS_BAIL_IF_ERROR (
        util::disable_port (handles_[comp_id], demuxer_input_port),
        "Unable to disable demuxer's audio port.");
    // Disable the demuxer's output ports (both of them)
    OMX_U32 demuxer_audio_port = 1;
    G_OPS_BAIL_IF_ERROR (
        util::disable_port (handles_[comp_id], demuxer_audio_port),
        "Unable to disable demuxer's audio port.");
    OMX_U32 demuxer_video_port = 2;
    G_OPS_BAIL_IF_ERROR (
        util::disable_port (handles_[comp_id], demuxer_video_port),
        "Unable to disable demuxer's video port.");
    clear_expected_port_transitions ();
    add_expected_port_transition (handles_[comp_id], demuxer_input_port,
                                  OMX_CommandPortDisable);
    add_expected_port_transition (handles_[comp_id], demuxer_audio_port,
                                  OMX_CommandPortDisable);
    add_expected_port_transition (handles_[comp_id], demuxer_video_port,
                                  OMX_CommandPortDisable);
  }
}

void graph::youtubeops::do_configure_comp (const int comp_id)
{
  if (last_op_succeeded ())
  {
    if (comp_id == 0)
    {
      G_OPS_BAIL_IF_ERROR (
          set_youtube_playlist (handles_[0], playlist_->get_current_uri ()),
          "Unable to set OMX_TizoniaIndexParamAudioYoutubePlaylist");
    }
    else if (comp_id == 3)
    {
      G_OPS_BAIL_IF_ERROR (apply_pcm_codec_info_from_decoder (),
                           "Unable to set OMX_IndexParamAudioPcm");
    }
  }
}

void graph::youtubeops::do_load_comp (const int comp_id)
{
  if (comp_id == 0)
  {
    do_load_http_source ();
  }
  else if (comp_id == 1)
  {
    do_load_demuxer ();
  }
  else if (comp_id == 2)
  {
    do_load_decoder ();
  }
  else if (comp_id == 3)
  {
    do_load_renderer ();
  }
}

void graph::youtubeops::do_omx_loaded2idle ()
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

void graph::youtubeops::do_omx_idle2exe ()
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

void graph::youtubeops::do_reconfigure_tunnel (const int tunnel_id)
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
    else if (2 == tunnel_id)
    {
      do_reconfigure_third_tunnel ();
    }
    else
    {
      assert (0);
    }
  }
}

void graph::youtubeops::do_skip ()
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

void graph::youtubeops::do_retrieve_metadata ()
{
  OMX_U32 index = 0;
  const int youtube_index = 0;
  // Extract metadata from the youtube source
  while (OMX_ErrorNone == dump_metadata_item (index++, youtube_index))
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

void graph::youtubeops::do_load_http_source ()
{
  assert (comp_lst_.size () == 0);
  assert (role_lst_.size () == 0);
  assert (handles_.size () == 0);

  // The youtube source will be instantiated now.

  omx_comp_name_lst_t comp_list;
  comp_list.push_back ("OMX.Aratelia.audio_source.http");

  omx_comp_role_lst_t role_list;
  role_list.push_back ("audio_source.http.youtube");

  G_OPS_BAIL_IF_ERROR (util::verify_comp_list (comp_list),
                       "Unable to verify the component list.");

  omx_comp_role_pos_lst_t role_positions;
  G_OPS_BAIL_IF_ERROR (
      util::verify_role_list (comp_list, role_list, role_positions),
      "Unable to verify the role list.");

  tiz::graph::cbackhandler &cbacks = get_cback_handler ();
  G_OPS_BAIL_IF_ERROR (
      util::instantiate_comp_list (comp_list, handles_, h2n_, &(cbacks),
                                   cbacks.get_omx_cbacks ()),
      "Unable to instantiate the component list.");

  G_OPS_BAIL_IF_ERROR (
      util::set_role_list (handles_, role_list, role_positions),
      "Unable to instantiate the component list.");

  // Now add the new component to the base class lists
  comp_lst_.insert (comp_lst_.begin (), comp_list.begin (), comp_list.end ());
  role_lst_.insert (role_lst_.begin (), role_list.begin (), role_list.end ());

  if (inital_graph_load_)
  {
    inital_graph_load_ = false;
    tiz::graph::util::dump_graph_info ("Youtube", "Connecting", "");
  }
}

void graph::youtubeops::do_load_demuxer ()
{
  assert (comp_lst_.size () == 1);
  assert (role_lst_.size () == 1);
  assert (handles_.size () == 1);

  // The youtube source is already instantiated. The container demuxer needs to
  // be instantiated next.

  G_OPS_BAIL_IF_ERROR (
      get_container_type_from_youtube_source (),
      "Unable to retrieve the audio encoding from the container demuxer.");

  omx_comp_handle_lst_t hdl_list;
  omx_comp_name_lst_t comp_list;
  omx_comp_role_lst_t role_list;
  G_OPS_BAIL_IF_ERROR (add_demuxer_to_component_list (comp_list, role_list),
                       "Unknown/unhandled container format.");

  G_OPS_BAIL_IF_ERROR (util::verify_comp_list (comp_list),
                       "Unable to verify the component list.");

  omx_comp_role_pos_lst_t role_positions;
  G_OPS_BAIL_IF_ERROR (
      util::verify_role_list (comp_list, role_list, role_positions),
      "Unable to verify the role list.");

  tiz::graph::cbackhandler &cbacks = get_cback_handler ();
  G_OPS_BAIL_IF_ERROR (
      util::instantiate_comp_list (comp_list, handles_, h2n_, &(cbacks),
                                   cbacks.get_omx_cbacks ()),
      "Unable to instantiate the component list.");

  hdl_list.insert (hdl_list.begin (), handles_.rbegin (),
                   handles_.rbegin () + 1);
  G_OPS_BAIL_IF_ERROR (
      util::set_role_list (hdl_list, role_list, role_positions),
      "Unable to instantiate the component list.");

  // Now add the new components to the base class lists
  comp_lst_.insert (comp_lst_.begin (), comp_list.begin (), comp_list.end ());
  role_lst_.insert (role_lst_.begin (), role_list.begin (), role_list.end ());
}

void graph::youtubeops::do_load_decoder ()
{
  assert (comp_lst_.size () == 2);
  assert (role_lst_.size () == 2);
  assert (handles_.size () == 2);

  // The audio decoder needs to be instantiated next.

  G_OPS_BAIL_IF_ERROR (
      get_encoding_type_from_container_demuxer (),
      "Unable to retrieve the audio encoding from the container demuxer.");

  omx_comp_name_lst_t comp_list;
  omx_comp_role_lst_t role_list;
  G_OPS_BAIL_IF_ERROR (add_decoder_to_component_list (comp_list, role_list),
                       "Unknown/unhandled stream format.");

  tiz::graph::cbackhandler &cbacks = get_cback_handler ();
  G_OPS_BAIL_IF_ERROR (
      util::instantiate_comp_list (comp_list, handles_, h2n_, &(cbacks),
                                   cbacks.get_omx_cbacks ()),
      "Unable to instantiate the component list.");

  // Now add the new components to the base class lists
  comp_lst_.insert (comp_lst_.begin (), comp_list.begin (), comp_list.end ());
  role_lst_.insert (role_lst_.begin (), role_list.begin (), role_list.end ());
}

void graph::youtubeops::do_load_renderer ()
{
  assert (comp_lst_.size () == 3);
  assert (role_lst_.size () == 3);
  assert (handles_.size () == 4);

  // The audio renderer needs to be instantiated next.
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

OMX_ERRORTYPE
graph::youtubeops::add_demuxer_to_component_list (
    omx_comp_name_lst_t &comp_list, omx_comp_role_lst_t &role_list)
{
  OMX_ERRORTYPE rc = OMX_ErrorFormatNotDetected;
  if (OMX_AUDIO_CodingWEBM == container_)
  {
    comp_list.push_back ("OMX.Aratelia.container_demuxer.webm");
    role_list.push_back ("container_demuxer.filter.webm");
    rc = OMX_ErrorNone;
  }
  else if (OMX_AUDIO_CodingMP4 == container_)
  {
    // TODO
    TIZ_LOG (TIZ_PRIORITY_ERROR,
             "[OMX_ErrorFormatNotDetected] : Unhandled container format "
             "[OMX_AUDIO_CodingMP4].");
  }
  else if (OMX_AUDIO_CodingOGA == container_)
  {
    // TODO
    TIZ_LOG (TIZ_PRIORITY_ERROR,
             "[OMX_ErrorFormatNotDetected] : Unhandled container format "
             "[OMX_AUDIO_CodingOGA].");
  }
  else
  {
    TIZ_LOG (
        TIZ_PRIORITY_ERROR,
        "[OMX_ErrorFormatNotDetected] : Unhandled container format [%d]...",
        container_);
  }
  return rc;
}

OMX_ERRORTYPE
graph::youtubeops::add_decoder_to_component_list (
    omx_comp_name_lst_t &comp_list, omx_comp_role_lst_t &role_list)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  switch (encoding_)
  {
    case OMX_AUDIO_CodingMP3:
    {
      comp_list.push_back ("OMX.Aratelia.audio_decoder.mp3");
      role_list.push_back ("audio_decoder.mp3");
    }
    break;
    case OMX_AUDIO_CodingAAC:
    {
      comp_list.push_back ("OMX.Aratelia.audio_decoder.aac");
      role_list.push_back ("audio_decoder.aac");
    }
    break;
    case OMX_AUDIO_CodingVORBIS:
    {
      comp_list.push_back ("OMX.Aratelia.audio_decoder.vorbis");
      role_list.push_back ("audio_decoder.vorbis");
    }
    break;
    default:
    {
      if (OMX_AUDIO_CodingOPUS == encoding_)
      {
        comp_list.push_back ("OMX.Aratelia.audio_decoder.opusfile.opus");
        role_list.push_back ("audio_decoder.opus");
      }
      else if (OMX_AUDIO_CodingFLAC == encoding_)
      {
        comp_list.push_back ("OMX.Aratelia.audio_decoder.flac");
        role_list.push_back ("audio_decoder.flac");
      }
      else
      {
        TIZ_LOG (
            TIZ_PRIORITY_ERROR,
            "[OMX_ErrorFormatNotDetected] : Unhandled encoding type [%d]...",
            encoding_);
        rc = OMX_ErrorFormatNotDetected;
      }
    }
    break;
  };
  return rc;
}

// TODO: Move this implementation to the base class (and remove also from
// httpservops)
OMX_ERRORTYPE
graph::youtubeops::transition_tunnel (
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
    const int youtube_source_index = 0;
    const int youtube_source_output_port = 0;
    add_expected_port_transition (handles_[youtube_source_index],
                                  youtube_source_output_port,
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

bool graph::youtubeops::probe_stream_hook ()
{
  return true;
}

OMX_ERRORTYPE graph::youtubeops::get_container_type_from_youtube_source ()
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  const OMX_U32 port_id = 0;
  TIZ_INIT_OMX_PORT_STRUCT (port_def, port_id);
  tiz_check_omx (
      OMX_GetParameter (handles_[0], OMX_IndexParamPortDefinition, &port_def));
  container_ = port_def.format.audio.eEncoding;
  TIZ_LOG (TIZ_PRIORITY_DEBUG, "container_ = [%X]", container_);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE graph::youtubeops::get_encoding_type_from_container_demuxer ()
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  const OMX_U32 port_id = 1;
  TIZ_INIT_OMX_PORT_STRUCT (port_def, port_id);
  tiz_check_omx (
      OMX_GetParameter (handles_[1], OMX_IndexParamPortDefinition, &port_def));
  encoding_ = port_def.format.audio.eEncoding;
  TIZ_LOG (TIZ_PRIORITY_DEBUG, "encoding_ = [%X]", encoding_);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::youtubeops::apply_pcm_codec_info_from_decoder ()
{
  OMX_U32 channels = 2;
  OMX_U32 sampling_rate = 44100;
  std::string encoding_str;

  tiz_check_omx (get_channels_and_rate_from_decoder (channels, sampling_rate,
                                                     encoding_str));
  tiz_check_omx (set_channels_and_rate_on_renderer (channels, sampling_rate,
                                                    encoding_str));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::youtubeops::get_channels_and_rate_from_decoder (
    OMX_U32 &channels, OMX_U32 &sampling_rate, std::string &encoding_str) const
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_HANDLETYPE handle = handles_[1];  // decoder's handle
  const OMX_U32 port_id = 1;                  // decoder's output port

  switch (encoding_)
  {
    case OMX_AUDIO_CodingMP3:
    {
      encoding_str = "mp3";
    }
    break;
    case OMX_AUDIO_CodingAAC:
    {
      encoding_str = "aac";
    }
    break;
    case OMX_AUDIO_CodingVORBIS:
    {
      encoding_str = "vorbis";
    }
    break;
    default:
    {
      if (OMX_AUDIO_CodingOPUS == encoding_)
      {
        encoding_str = "opus";
      }
      else if (OMX_AUDIO_CodingFLAC == encoding_)
      {
        encoding_str = "flac";
      }
      else
      {
        TIZ_LOG (
            TIZ_PRIORITY_ERROR,
            "[OMX_ErrorFormatNotDetected] : Unhandled encoding type [%d]...",
            encoding_);
        rc = OMX_ErrorFormatNotDetected;
      }
    }
    break;
  };

  if (OMX_ErrorNone == rc)
  {
    rc = tiz::graph::util::
        get_channels_and_rate_from_audio_port_v2< OMX_AUDIO_PARAM_PCMMODETYPE > (
            handle, port_id, OMX_IndexParamAudioPcm, channels, sampling_rate);
  }
  TIZ_LOG (TIZ_PRIORITY_TRACE, "outcome = [%s]", tiz_err_to_str (rc));

  return rc;
}

OMX_ERRORTYPE
graph::youtubeops::set_channels_and_rate_on_renderer (
    const OMX_U32 channels, const OMX_U32 sampling_rate,
    const std::string encoding_str)
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

  if (OMX_AUDIO_CodingOPUS == encoding_ || OMX_AUDIO_CodingVORBIS == encoding_)
  {
    // Opus and Vorbis decoders output 32 bit samples (floats)
    renderer_pcmtype_.nBitPerSample = 32;
  }

  // Set the new pcm settings
  tiz_check_omx (
      OMX_SetParameter (handle, OMX_IndexParamAudioPcm, &renderer_pcmtype_));

  std::string coding_type_str ("Youtube");
  tiz::graph::util::dump_graph_info (coding_type_str.c_str (), "Connected",
                                     playlist_->get_current_uri ().c_str ());

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::youtubeops::set_youtube_playlist (const OMX_HANDLETYPE handle,
                                         const std::string &playlist)
{
  // Set the Youtube playlist
  OMX_TIZONIA_AUDIO_PARAM_YOUTUBEPLAYLISTTYPE playlisttype;
  TIZ_INIT_OMX_STRUCT (playlisttype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioYoutubePlaylist),
      &playlisttype));
  copy_omx_string (playlisttype.cPlaylistName, playlist);

  tizyoutubeconfig_ptr_t youtube_config
      = boost::dynamic_pointer_cast< youtubeconfig > (config_);
  assert (youtube_config);

  playlisttype.ePlaylistType = youtube_config->get_playlist_type ();
  playlisttype.bShuffle = playlist_->shuffle () ? OMX_TRUE : OMX_FALSE;

  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioYoutubePlaylist),
      &playlisttype);
}

bool graph::youtubeops::is_fatal_error (const OMX_ERRORTYPE error) const
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

void graph::youtubeops::do_record_fatal_error (const OMX_HANDLETYPE handle,
                                               const OMX_ERRORTYPE error,
                                               const OMX_U32 port)
{
  tiz::graph::ops::do_record_fatal_error (handle, error, port);
  if (error == OMX_ErrorContentURIError)
  {
    error_msg_.append ("\n [Playlist not found]");
  }
}

void graph::youtubeops::do_reconfigure_first_tunnel ()
{
  switch (encoding_)
  {
    case OMX_AUDIO_CodingMP3:
    {
      // Retrieve the mp3 settings from the youtube source component
      OMX_AUDIO_PARAM_MP3TYPE youtube_mp3type;
      const OMX_U32 youtube_port_id = 0;
      TIZ_INIT_OMX_PORT_STRUCT (youtube_mp3type, youtube_port_id);
      G_OPS_BAIL_IF_ERROR (
          OMX_GetParameter (handles_[0], OMX_IndexParamAudioMp3,
                            &youtube_mp3type),
          "Unable to retrieve the MP3 settings from the youtube source");

      // Retrieve the mp3 settings from the decoder component
      OMX_AUDIO_PARAM_MP3TYPE decoder_mp3type;
      const OMX_U32 decoder_port_id = 0;
      TIZ_INIT_OMX_PORT_STRUCT (decoder_mp3type, decoder_port_id);
      G_OPS_BAIL_IF_ERROR (
          OMX_GetParameter (handles_[1], OMX_IndexParamAudioMp3,
                            &decoder_mp3type),
          "Unable to retrieve the MP3 settings from the audio decoder");

      // Now assign the current settings to the decoder structure
      decoder_mp3type.nChannels = youtube_mp3type.nChannels;
      decoder_mp3type.nSampleRate = youtube_mp3type.nSampleRate;

      // Set the new mp3 settings
      G_OPS_BAIL_IF_ERROR (
          OMX_SetParameter (handles_[1], OMX_IndexParamAudioMp3,
                            &decoder_mp3type),
          "Unable to set the MP3 settings on the audio decoder");
    }
    break;
    default:
    {
      // TODO
    }
    break;
  };
}

void graph::youtubeops::do_reconfigure_second_tunnel ()
{
  // Retrieve the audio port format from the demuxer component
  OMX_AUDIO_PARAM_PORTFORMATTYPE demuxer_audio_format;
  const OMX_U32 demuxer_port_id = 1;
  TIZ_INIT_OMX_PORT_STRUCT (demuxer_audio_format, demuxer_port_id);
  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[1], OMX_IndexParamAudioPortFormat,
                        &demuxer_audio_format),
      "Unable to retrieve the audio port format from the demuxer");

  // Retrieve the pcm settings from the renderer component
  OMX_AUDIO_PARAM_PCMMODETYPE renderer_pcmtype;
  const OMX_U32 renderer_port_id = 0;
  TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype, renderer_port_id);
  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[2], OMX_IndexParamAudioPcm, &renderer_pcmtype),
      "Unable to retrieve the PCM settings from the pcm renderer");

  // Now assign the current settings to the renderer structure
  //   renderer_pcmtype.nChannels = demuxer_audio_format.nChannels;
  //   renderer_pcmtype.nSamplingRate = demuxer_audio_format.nSamplingRate;

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

void graph::youtubeops::do_reconfigure_third_tunnel ()
{
  // Retrieve the pcm settings from the decoder component
  OMX_AUDIO_PARAM_PCMMODETYPE decoder_pcmtype;
  const OMX_U32 decoder_port_id = 1;
  TIZ_INIT_OMX_PORT_STRUCT (decoder_pcmtype, decoder_port_id);
  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[2], OMX_IndexParamAudioPcm, &decoder_pcmtype),
      "Unable to retrieve the PCM settings from the decoder");

  // Retrieve the pcm settings from the renderer component
  OMX_AUDIO_PARAM_PCMMODETYPE renderer_pcmtype;
  const OMX_U32 renderer_port_id = 0;
  TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype, renderer_port_id);
  G_OPS_BAIL_IF_ERROR (
      OMX_GetParameter (handles_[3], OMX_IndexParamAudioPcm, &renderer_pcmtype),
      "Unable to retrieve the PCM settings from the pcm renderer");

  // Now assign the current settings to the renderer structure
  renderer_pcmtype.nChannels = decoder_pcmtype.nChannels;
  renderer_pcmtype.nSamplingRate = decoder_pcmtype.nSamplingRate;

  // Set the new pcm settings
  G_OPS_BAIL_IF_ERROR (
      OMX_SetParameter (handles_[3], OMX_IndexParamAudioPcm, &renderer_pcmtype),
      "Unable to set the PCM settings on the audio renderer");

  TIZ_PRINTF_MAG (
      "     %ld Ch, %g KHz, %lu:%s:%s\n", renderer_pcmtype.nChannels,
      ((float)renderer_pcmtype.nSamplingRate) / 1000,
      renderer_pcmtype.nBitPerSample,
      renderer_pcmtype.eNumData == OMX_NumericalDataSigned ? "s" : "u",
      renderer_pcmtype.eEndian == OMX_EndianBig ? "b" : "l");
}

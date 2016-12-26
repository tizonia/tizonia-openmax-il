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
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    encoding_ (OMX_AUDIO_CodingAutoDetect)
{
}

void graph::httpclntops::do_enable_auto_detection (const int handle_id, const int port_id)
{
  tiz::graph::ops::do_enable_auto_detection (handle_id, port_id);
  tiz::graph::util::dump_graph_info ("http", "Connecting to radio station",
                                     playlist_->get_current_uri ().c_str ());
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

void graph::httpclntops::do_configure_comp (const int comp_id)
{
  if (comp_id == 0)
  {
    G_OPS_BAIL_IF_ERROR (
        util::set_content_uri (handles_[0], playlist_->get_current_uri ()),
        "Unable to set OMX_IndexParamContentURI");
  }
}

void graph::httpclntops::do_load ()
{
  assert (!comp_lst_.empty ());
  assert (!role_lst_.empty ());

  // At this point we are going to instantiate the two remaining components in
  // the graph, the audio decoder and the pcm renderer. The http source is
  // already instantiated and in Executing state.

  assert (comp_lst_.size () == 1);

  dump_stream_metadata ();

  G_OPS_BAIL_IF_ERROR (
      get_encoding_type_from_http_source (),
      "Unable to retrieve the audio encoding from the http source.");

  omx_comp_name_lst_t comp_list;
  omx_comp_role_lst_t role_list;
  G_OPS_BAIL_IF_ERROR (add_decoder_to_component_list (comp_list, role_list),
                       "Unknown/unhandled stream format.");

  comp_list.push_back (tiz::graph::util::get_default_pcm_renderer ());
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
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (apply_pcm_codec_info_from_http_source (),
                         "Unable to set OMX_IndexParamAudioPcm");
  }
}

void graph::httpclntops::do_omx_exe2pause ()
{
  // No-op. This is to disable pause in this graph
}

void graph::httpclntops::do_omx_pause2exe ()
{
  // No-op. This is to disable pause in this graph
}

void graph::httpclntops::do_omx_loaded2idle ()
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

void graph::httpclntops::do_reconfigure_tunnel (const int tunnel_id)
{
  if (last_op_succeeded ())
  {
    assert (1 == tunnel_id);     // We only expect tunnel reconfiguration in tunnel 1
    // Retrieve the pcm settings from the decoder component
    OMX_AUDIO_PARAM_PCMMODETYPE decoder_pcmtype;
    const OMX_U32 decoder_port_id = 1;
    TIZ_INIT_OMX_PORT_STRUCT (decoder_pcmtype, decoder_port_id);
    G_OPS_BAIL_IF_ERROR (
        OMX_GetParameter (handles_[1], OMX_IndexParamAudioPcm,
                          &decoder_pcmtype),
        "Unable to retrieve the PCM settings from the audio decoder");

    // Retrieve the pcm settings from the renderer component
    OMX_AUDIO_PARAM_PCMMODETYPE renderer_pcmtype;
    const OMX_U32 renderer_port_id = 0;
    TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype, renderer_port_id);
    G_OPS_BAIL_IF_ERROR (
        OMX_GetParameter (handles_[2], OMX_IndexParamAudioPcm,
                          &renderer_pcmtype),
        "Unable to retrieve the PCM settings from the pcm renderer");

    // Now assign the current settings to the renderer structure
    renderer_pcmtype.nChannels = decoder_pcmtype.nChannels;
    renderer_pcmtype.nSamplingRate = decoder_pcmtype.nSamplingRate;

    // Set the new pcm settings
    G_OPS_BAIL_IF_ERROR (
        OMX_SetParameter (handles_[2], OMX_IndexParamAudioPcm,
                          &renderer_pcmtype),
        "Unable to set the PCM settings on the audio renderer");

    TIZ_PRINTF_MAG ("     %ld Ch, %g KHz, %lu:%s:%s\n",
             renderer_pcmtype.nChannels,
             ((float)renderer_pcmtype.nSamplingRate) / 1000,
             renderer_pcmtype.nBitPerSample,
             renderer_pcmtype.eNumData == OMX_NumericalDataSigned ? "s" : "u",
             renderer_pcmtype.eEndian == OMX_EndianBig ? "b" : "l");
  }
}

// TODO: Move this implementation to the base class (and remove also from
// httpservops)
OMX_ERRORTYPE
graph::httpclntops::transition_tunnel (
    const int tunnel_id, const OMX_COMMANDTYPE to_disabled_or_enabled)
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

  if (OMX_ErrorNone == rc && 0 == tunnel_id)
  {
    clear_expected_port_transitions ();
    const int http_source_index = 0;
    const int http_source_output_port = 0;
    add_expected_port_transition (handles_[http_source_index],
                                  http_source_output_port,
                                  to_disabled_or_enabled);
    const int decoder_index = 1;
    const int decoder_input_port = 0;
    add_expected_port_transition (handles_[decoder_index], decoder_input_port,
                                  to_disabled_or_enabled);
  }
  else if (OMX_ErrorNone == rc && 1 == tunnel_id)
  {
    clear_expected_port_transitions ();
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

OMX_ERRORTYPE
graph::httpclntops::add_decoder_to_component_list (
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
          TIZ_LOG (TIZ_PRIORITY_ERROR,
                   "[OMX_ErrorFormatNotDetected] : Unhandled encoding type [%d]...",
                   encoding_);
          rc = OMX_ErrorFormatNotDetected;
        }
      }
      break;
  };
  return rc;
}

bool graph::httpclntops::probe_stream_hook ()
{
  return true;
}

void graph::httpclntops::dump_stream_metadata ()
{
  OMX_U32 index = 0;
  const int http_source_index = 0;
  const bool use_first_as_heading = false;
  while (OMX_ErrorNone == dump_metadata_item (index++, http_source_index,
                                              use_first_as_heading))
  {
  };
}

OMX_ERRORTYPE graph::httpclntops::get_encoding_type_from_http_source ()
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
graph::httpclntops::apply_pcm_codec_info_from_http_source ()
{
  OMX_U32 channels = 2;
  OMX_U32 sampling_rate = 44100;
  std::string encoding_str;

  tiz_check_omx (get_channels_and_rate_from_http_source (
      channels, sampling_rate, encoding_str));
  tiz_check_omx (
      set_channels_and_rate_on_decoder (channels, sampling_rate));
  tiz_check_omx (set_channels_and_rate_on_renderer (channels, sampling_rate,
                                                        encoding_str));

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::httpclntops::get_channels_and_rate_from_http_source (
    OMX_U32 &channels, OMX_U32 &sampling_rate, std::string &encoding_str) const
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_HANDLETYPE handle = handles_[0];  // http source's handle
  const OMX_U32 port_id = 0;                  // http source's output port

  switch (encoding_)
  {
    case OMX_AUDIO_CodingMP3:
    {
      encoding_str = "mp3";
      rc = tiz::graph::util::
          get_channels_and_rate_from_audio_port< OMX_AUDIO_PARAM_MP3TYPE >(
              handle, port_id, OMX_IndexParamAudioMp3, channels, sampling_rate);
    }
    break;
    case OMX_AUDIO_CodingAAC:
    {
      encoding_str = "aac";
      rc = tiz::graph::util::
          get_channels_and_rate_from_audio_port< OMX_AUDIO_PARAM_AACPROFILETYPE >(
              handle, port_id, OMX_IndexParamAudioAac, channels, sampling_rate);
    }
    break;
    case OMX_AUDIO_CodingVORBIS:
    {
      encoding_str = "vorbis";
      rc = tiz::graph::util::
          get_channels_and_rate_from_audio_port< OMX_AUDIO_PARAM_VORBISTYPE >(
              handle, port_id, OMX_IndexParamAudioVorbis, channels,
              sampling_rate);
    }
    break;
    default:
    {
      if (OMX_AUDIO_CodingOPUS == encoding_)
      {
        encoding_str = "opus";
        rc = tiz::graph::util::
            get_channels_and_rate_from_audio_port< OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE >(
                handle, port_id,
                static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioOpus),
                channels, sampling_rate);
      }
      else if (OMX_AUDIO_CodingFLAC == encoding_)
      {
        encoding_str = "flac";
        rc = tiz::graph::util::
            get_channels_and_rate_from_audio_port< OMX_TIZONIA_AUDIO_PARAM_FLACTYPE >(
                handle, port_id,
                static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioFlac),
                channels, sampling_rate);
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

OMX_ERRORTYPE
graph::httpclntops::set_channels_and_rate_on_decoder (
    const OMX_U32 channels, const OMX_U32 sampling_rate)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_HANDLETYPE handle = handles_[1];  // decoder
  const OMX_U32 port_id = 0;                  // decoder's input port

  TIZ_LOG (TIZ_PRIORITY_TRACE, "channels = [%d] sampling_rate = [%d]", channels,
           sampling_rate);

  switch (encoding_)
  {
    case OMX_AUDIO_CodingMP3:
    {
      rc = tiz::graph::util::
          set_channels_and_rate_on_audio_port< OMX_AUDIO_PARAM_MP3TYPE >(
              handle, port_id, OMX_IndexParamAudioMp3, channels, sampling_rate);
    }
    break;
    case OMX_AUDIO_CodingAAC:
    {
      rc = tiz::graph::util::
          set_channels_and_rate_on_audio_port< OMX_AUDIO_PARAM_AACPROFILETYPE >(
              handle, port_id, OMX_IndexParamAudioAac, channels, sampling_rate);
    }
    break;
    case OMX_AUDIO_CodingVORBIS:
    {
      rc = tiz::graph::util::
          set_channels_and_rate_on_audio_port< OMX_AUDIO_PARAM_VORBISTYPE >(
              handle, port_id, OMX_IndexParamAudioVorbis, channels,
              sampling_rate);
    }
    break;
    default:
      {
        if (OMX_AUDIO_CodingOPUS == encoding_)
          {
            rc = tiz::graph::util::
              set_channels_and_rate_on_audio_port< OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE >(
              handle, port_id,
              static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioOpus),
              channels, sampling_rate);
          }
        else if (OMX_AUDIO_CodingFLAC == encoding_)
          {
            rc = tiz::graph::util::
              set_channels_and_rate_on_audio_port< OMX_TIZONIA_AUDIO_PARAM_FLACTYPE >(
              handle, port_id,
              static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamAudioFlac),
              channels, sampling_rate);
          }
        else
          {
            TIZ_LOG (TIZ_PRIORITY_ERROR,
                     "[OMX_ErrorFormatNotDetected] : Unhandled encoding type [%d]...",
                     encoding_);
            rc = OMX_ErrorFormatNotDetected;
          }
      }
      break;
  };

  return rc;
}

OMX_ERRORTYPE
graph::httpclntops::set_channels_and_rate_on_renderer (
    const OMX_U32 channels, const OMX_U32 sampling_rate,
    const std::string encoding_str)
{
  const OMX_HANDLETYPE handle = handles_[2];  // renderer's handle
  const OMX_U32 port_id = 0;                  // renderer's input port

  TIZ_LOG (TIZ_PRIORITY_TRACE, "channels = [%d] sampling_rate = [%d]", channels,
           sampling_rate);

  // Retrieve the pcm settings from the renderer component
  OMX_AUDIO_PARAM_PCMMODETYPE renderer_pcmtype;
  TIZ_INIT_OMX_PORT_STRUCT (renderer_pcmtype, port_id);
  tiz_check_omx (
      OMX_GetParameter (handle, OMX_IndexParamAudioPcm, &renderer_pcmtype));

  // Now assign the actual settings to the pcmtype structure
  renderer_pcmtype.nChannels = channels;
  renderer_pcmtype.nSamplingRate = sampling_rate;
  renderer_pcmtype.eNumData = OMX_NumericalDataSigned;
  renderer_pcmtype.eEndian
      = (encoding_ == OMX_AUDIO_CodingMP3 ? OMX_EndianBig : OMX_EndianLittle);

  if (OMX_AUDIO_CodingOPUS == encoding_ || OMX_AUDIO_CodingVORBIS == encoding_)
  {
    // Opus and Vorbis decoders output 32 bit samples (floats)
    renderer_pcmtype.nBitPerSample = 32;
  }

  // Set the new pcm settings
  tiz_check_omx (
      OMX_SetParameter (handle, OMX_IndexParamAudioPcm, &renderer_pcmtype));

  std::string coding_type_str ("http/");
  coding_type_str.append (encoding_str);
  tiz::graph::util::dump_graph_info (coding_type_str.c_str (), "Connected",
                                     playlist_->get_current_uri ().c_str ());

  TIZ_PRINTF_MAG ("     %ld Ch, %g KHz, %lu:%s:%s\n",
           renderer_pcmtype.nChannels,
           ((float)renderer_pcmtype.nSamplingRate) / 1000,
           renderer_pcmtype.nBitPerSample,
           renderer_pcmtype.eNumData == OMX_NumericalDataSigned ? "s" : "u",
           renderer_pcmtype.eEndian == OMX_EndianBig ? "b" : "l");

  return OMX_ErrorNone;
}

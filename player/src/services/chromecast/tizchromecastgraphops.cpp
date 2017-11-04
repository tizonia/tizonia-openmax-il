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
    // TODO
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

  // TODO
  // Now print renderer metadata
//   TIZ_PRINTF_MAG (
//       "     %ld Ch, %g KHz, %lu:%s:%s \n", renderer_pcmtype_.nChannels,
//       ((float)renderer_pcmtype_.nSamplingRate) / 1000,
//       renderer_pcmtype_.nBitPerSample,
//       renderer_pcmtype_.eNumData == OMX_NumericalDataSigned ? "s" : "u",
//       renderer_pcmtype_.eEndian == OMX_EndianBig ? "b" : "l");
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

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

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_TizoniaExt.h>
#include <tizplatform.h>

#include "tizchromecastconfig.hpp"
#include "tizchromecastgraphops.hpp"
#include <tizdirbleconfig.hpp>
#include <tizgmusicconfig.hpp>
#include <tizgraph.hpp>
#include <tizgraphutil.hpp>
#include <tizprobe.hpp>
#include <tizscloudconfig.hpp>
#include <tizyoutubeconfig.hpp>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.chromecast.ops"
#endif

namespace graph = tiz::graph;

//
// chromecastops
//
graph::chromecastops::chromecastops (graph* p_graph,
                                     const omx_comp_name_lst_t& comp_lst,
                                     const omx_comp_role_lst_t& role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    encoding_ (OMX_AUDIO_CodingAutoDetect),
    config_service_func_ (),
    cc_config_ ()
{
}

void graph::chromecastops::do_load ()
{
  assert (comp_lst_.size () == 1);
  assert (role_lst_.size () == 1);
  assert (config_);

  // Clear the role list. We'll override it with a new role once we know what
  // we need.
  role_lst_.clear ();

  tizchromecastconfig_ptr_t cc_config_
      = boost::dynamic_pointer_cast< chromecastconfig > (config_);
  assert (cc_config_);

  const std::type_info& ti_current
      = typeid (*(cc_config_->get_service_config ()));
  const std::type_info& ti_gmusic = typeid (tizgmusicconfig_ptr_t);
  const std::type_info& ti_scloud = typeid (tizscloudconfig_ptr_t);
  const std::type_info& ti_dirble = typeid (tizdirbleconfig_ptr_t);
  const std::type_info& ti_youtube = typeid (tizyoutubeconfig_ptr_t);

  if (ti_current == ti_gmusic)
  {
    role_lst_.push_back ("audio_renderer.chromecast.gmusic");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_gmusic, this);
  }
  else if (ti_current == ti_scloud)
  {
    role_lst_.push_back ("audio_renderer.chromecast.scloud");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_scloud, this);
  }
  else if (ti_current == ti_dirble)
  {
    role_lst_.push_back ("audio_renderer.chromecast.dirble");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_dirble, this);
  }
  else if (ti_current == ti_youtube)
  {
    role_lst_.push_back ("audio_renderer.chromecast.youtube");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_youtube, this);
  }
  else
  {
    std::string msg ("Unable to set a suitable component role");
    BOOST_ASSERT_MSG (false, msg.c_str ());
    G_OPS_BAIL_IF_ERROR (OMX_ErrorComponentNotFound, msg.c_str ());
  }
  // At this point we are instantiating a graph with a single component.
  tiz::graph::ops::do_load ();
}

void graph::chromecastops::do_configure ()
{
  if (last_op_succeeded ())
  {
    // Set the chromecast name or ip parameter
    do_configure_chromecast ();

    if (last_op_succeeded ())
    {
      // Now set the service parameters
      config_service_func_ ();
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

  // TODO
  // Now print renderer metadata
  //   TIZ_PRINTF_MAG (
  //       "     %ld Ch, %g KHz, %lu:%s:%s \n", renderer_pcmtype_.nChannels,
  //       ((float)renderer_pcmtype_.nSamplingRate) / 1000,
  //       renderer_pcmtype_.nBitPerSample,
  //       renderer_pcmtype_.eNumData == OMX_NumericalDataSigned ? "s" : "u",
  //       renderer_pcmtype_.eEndian == OMX_EndianBig ? "b" : "l");
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

void graph::chromecastops::do_configure_chromecast ()
{
  assert (cc_config_);

  G_OPS_BAIL_IF_ERROR (tiz::graph::util::set_chromecast_name_or_ip (
                           handles_[0], cc_config_->get_name_or_ip ()),
                       "Unable to set OMX_TizoniaIndexParamChromecastSession");
}

void graph::chromecastops::do_configure_gmusic ()
{
  assert (cc_config_);
  tizgmusicconfig_ptr_t gmusic_config
      = boost::dynamic_pointer_cast< gmusicconfig > (
          cc_config_->get_service_config ());
  assert (gmusic_config);

  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_gmusic_user_and_device_id (
          handles_[0], gmusic_config->get_user_name (),
          gmusic_config->get_user_pass (), gmusic_config->get_device_id ()),
      "Unable to set OMX_TizoniaIndexParamAudioGmusicSession");

  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_gmusic_playlist (
          handles_[0], playlist_->get_current_uri (),
          gmusic_config->get_playlist_type (), playlist_->shuffle (),
          gmusic_config->is_unlimited_search ()),
      "Unable to set OMX_TizoniaIndexParamAudioGmusicPlaylist");
}

void graph::chromecastops::do_configure_scloud ()
{
  assert (cc_config_);
  tizscloudconfig_ptr_t scloud_config
      = boost::dynamic_pointer_cast< scloudconfig > (
          cc_config_->get_service_config ());
  assert (scloud_config);

  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_scloud_oauth_token (
          handles_[0], scloud_config->get_oauth_token ()),
      "Unable to set OMX_TizoniaIndexParamAudioSoundCloudSession");

  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_scloud_playlist (
          handles_[0], playlist_->get_current_uri (),
          scloud_config->get_playlist_type (), playlist_->shuffle ()),
      "Unable to set OMX_TizoniaIndexParamAudioSoundCloudPlaylist");
}

void graph::chromecastops::do_configure_dirble ()
{
  assert (cc_config_);
  tizdirbleconfig_ptr_t dirble_config
      = boost::dynamic_pointer_cast< dirbleconfig > (
          cc_config_->get_service_config ());
  assert (dirble_config);

  G_OPS_BAIL_IF_ERROR (tiz::graph::util::set_dirble_api_key (
                           handles_[0], dirble_config->get_api_key ()),
                       "Unable to set OMX_TizoniaIndexParamAudioDirbleSession");

  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_dirble_playlist (
          handles_[0], playlist_->get_current_uri (),
          dirble_config->get_playlist_type (), playlist_->shuffle ()),
      "Unable to set OMX_TizoniaIndexParamAudioDirblePlaylist");
}

void graph::chromecastops::do_configure_youtube ()
{
  assert (cc_config_);
  tizyoutubeconfig_ptr_t youtube_config
      = boost::dynamic_pointer_cast< youtubeconfig > (
          cc_config_->get_service_config ());
  assert (youtube_config);

  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_youtube_playlist (
          handles_[0], playlist_->get_current_uri (),
          youtube_config->get_playlist_type (), playlist_->shuffle ()),
      "Unable to set OMX_TizoniaIndexParamAudioYoutubePlaylist");
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

bool graph::chromecastops::probe_stream_hook ()
{
  return true;
}

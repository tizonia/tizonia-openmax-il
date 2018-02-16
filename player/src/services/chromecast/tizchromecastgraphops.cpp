/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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

#include <ctype.h>

#include <algorithm>

#include <boost/assert.hpp>
#include <boost/bind.hpp>

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
#include <tizplexconfig.hpp>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.chromecast.ops"
#endif

namespace graph = tiz::graph;
typedef tiz::graph::chromecastconfig cc_cfg_t;

#define CC_OPS_DO_LOAD_ERROR_MSG "Unable to set a suitable component role"

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

  cc_config_ = boost::dynamic_pointer_cast< chromecastconfig > (config_);
  assert (cc_config_);

  const cc_cfg_t::service_config_type_t config_type
      = cc_config_->get_service_config_type ();

  if (config_type == cc_cfg_t::ConfigHttpStreaming)
  {
    role_lst_.push_back ("audio_renderer.chromecast");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_http, this);
  }
  else if (config_type == cc_cfg_t::ConfigGoogleMusic)
  {
    role_lst_.push_back ("audio_renderer.chromecast.gmusic");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_gmusic, this);
  }
  else if (config_type == cc_cfg_t::ConfigSoundCloud)
  {
    role_lst_.push_back ("audio_renderer.chromecast.scloud");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_scloud, this);
  }
  else if (config_type == cc_cfg_t::ConfigDirble)
  {
    role_lst_.push_back ("audio_renderer.chromecast.dirble");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_dirble, this);
  }
  else if (config_type == cc_cfg_t::ConfigYouTube)
  {
    role_lst_.push_back ("audio_renderer.chromecast.youtube");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_youtube, this);
  }
  else if (config_type == cc_cfg_t::ConfigPlex)
  {
    role_lst_.push_back ("audio_renderer.chromecast.plex");
    config_service_func_
        = boost::bind (&tiz::graph::chromecastops::do_configure_plex, this);
  }
  else
  {
    BOOST_ASSERT_MSG (false, CC_OPS_DO_LOAD_ERROR_MSG);
    G_OPS_BAIL_IF_ERROR (OMX_ErrorComponentNotFound, CC_OPS_DO_LOAD_ERROR_MSG);
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
  while (OMX_ErrorNone
      == dump_metadata_item (index++, chromecast_index))
  {
  };
}

OMX_ERRORTYPE
graph::chromecastops::dump_metadata_item (
    const OMX_U32 index, const int comp_index,
    const bool use_first_as_heading /* = true */)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_CONFIG_METADATAITEMTYPE *p_meta = NULL;
  size_t metadata_len = 0;
  size_t value_len = 0;

  (void)use_first_as_heading;

  value_len = OMX_MAX_STRINGNAME_SIZE;
  metadata_len = sizeof (OMX_CONFIG_METADATAITEMTYPE) + value_len;

  if (NULL
      == (p_meta
          = (OMX_CONFIG_METADATAITEMTYPE *)tiz_mem_calloc (1, metadata_len)))
  {
    rc = OMX_ErrorInsufficientResources;
  }
  else
  {
    p_meta->nSize = metadata_len;
    p_meta->nVersion.nVersion = OMX_VERSION;
    p_meta->eScopeMode = OMX_MetadataScopeAllLevels;
    p_meta->nScopeSpecifier = 0;
    p_meta->nMetadataItemIndex = index;
    p_meta->eSearchMode = OMX_MetadataSearchValueSizeByIndex;
    p_meta->eKeyCharset = OMX_MetadataCharsetASCII;
    p_meta->eValueCharset = OMX_MetadataCharsetASCII;
    p_meta->nKeySizeUsed = 0;
    p_meta->nValue[0] = '\0';
    p_meta->nValueMaxSize = OMX_MAX_STRINGNAME_SIZE;
    p_meta->nValueSizeUsed = 0;

    rc = OMX_GetConfig (handles_[comp_index], OMX_IndexConfigMetadataItem,
                        p_meta);
    if (OMX_ErrorNone == rc
        && strnlen ((const char *)p_meta->nKey, OMX_MAX_STRINGNAME_SIZE)
        && strnlen ((const char *)p_meta->nValue, OMX_MAX_STRINGNAME_SIZE))
    {
      if (isspace (p_meta->nKey[0]) && isspace (p_meta->nKey[1]))
      {
        TIZ_PRINTF_GRN ("   %s : %s\n", p_meta->nKey, p_meta->nValue);
      }
      else if (0 == index && use_first_as_heading)
      {
        TIZ_PRINTF_YEL ("   %s : %s\n", p_meta->nKey, p_meta->nValue);
      }
      else
      {
        TIZ_PRINTF_CYN ("     %s : %s\n", p_meta->nKey, p_meta->nValue);
      }
    }

    tiz_mem_free (p_meta);
    p_meta = NULL;
  }
  return rc;
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

void graph::chromecastops::do_configure_chromecast ()
{
  assert (cc_config_);
  G_OPS_BAIL_IF_ERROR (tiz::graph::util::set_chromecast_name_or_ip (
                           handles_[0], cc_config_->get_name_or_ip ()),
                       "Unable to set OMX_TizoniaIndexParamChromecastSession");
}

void graph::chromecastops::do_configure_http ()
{
  assert (cc_config_);
  tizgraphconfig_ptr_t config
      = cc_config_->get_service_config ();

  assert (config);
  tizplaylist_ptr_t playlist = config_->get_playlist ();

  G_OPS_BAIL_IF_ERROR (
      util::set_content_uri (handles_[0], playlist_->get_current_uri ()),
      "Unable to set OMX_IndexParamContentURI");
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

void graph::chromecastops::do_configure_plex ()
{
  assert (cc_config_);
  tizplexconfig_ptr_t plex_config
      = boost::dynamic_pointer_cast< plexconfig > (
          cc_config_->get_service_config ());
  assert (plex_config);

  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_plex_playlist (
          handles_[0], playlist_->get_current_uri (),
          plex_config->get_playlist_type (), playlist_->shuffle ()),
      "Unable to set OMX_TizoniaIndexParamAudioPlexPlaylist");
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

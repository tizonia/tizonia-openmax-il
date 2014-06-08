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
 * @file   tizhttpservgraphops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Server graph implementation
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
#include "tizhttpservconfig.hpp"
#include "tizhttpservgraphops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.httpserverops"
#endif

namespace graph = tiz::graph;

//
// httpservops
//
graph::httpservops::httpservops (graph *p_graph,
                                 const omx_comp_name_lst_t &comp_lst,
                                 const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst),
    is_initial_configuration_ (true)
{
}

void graph::httpservops::do_probe ()
{
  G_OPS_BAIL_IF_ERROR (
      probe_stream (OMX_PortDomainAudio, OMX_AUDIO_CodingMP3, "mp3/http",
                    "stream", &tiz::probe::dump_mp3_info),
      "Unable to probe the stream.");
}

void graph::httpservops::do_omx_exe2pause ()
{
  // No-op. This is to disable pause in this graph
}

void graph::httpservops::do_omx_pause2exe ()
{
  // No-op. This is to disable pause in this graph
}

void graph::httpservops::do_volume (const int step)
{
  // No-op. This is to disable volume in this graph
}

void graph::httpservops::do_mute ()
{
  // No-op. This is to disable mute in this graph
}

void graph::httpservops::do_configure_server ()
{
  G_OPS_BAIL_IF_ERROR (configure_server (),
                       "Unable to set OMX_TizoniaIndexParamHttpServer");
}

void graph::httpservops::do_configure_station ()
{
  G_OPS_BAIL_IF_ERROR (configure_station (),
                       "Unable to set OMX_TizoniaIndexParamIcecastMountpoint");
}

void graph::httpservops::do_configure_stream ()
{
  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_content_uri (handles_[0], probe_ptr_->get_uri ()),
      "Unable to set OMX_IndexParamContentURI");
  bool need_port_settings_changed_evt = false;  // not needed here
  G_OPS_BAIL_IF_ERROR (
      tiz::graph::util::set_mp3_type (
          handles_[1], 0,
          boost::bind (&tiz::probe::get_mp3_codec_info, probe_ptr_, _1),
          need_port_settings_changed_evt),
      "Unable to set OMX_IndexParamAudioMp3");
  G_OPS_BAIL_IF_ERROR (configure_stream_metadata (),
                       "Unable to set OMX_TizoniaIndexConfigIcecastMetadata");
}

void graph::httpservops::do_source_omx_idle2exe ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (transition_source (OMX_StateExecuting),
                         "Unable to transition file reader from Idle->Exe");
  }
}

void graph::httpservops::do_source_omx_exe2idle ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (transition_source (OMX_StateIdle),
                         "Unable to transition file reader from Exe->Idle");
  }
}

void graph::httpservops::do_source_omx_idle2loaded ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (transition_source (OMX_StateLoaded),
                         "Unable to transition file reader from Idle->Loaded");
  }
}

void graph::httpservops::do_disable_tunnel ()
{
  if (last_op_succeeded ())
  {
    // there is only one tunnel in this graph
    const int tunnel_id = 0;
    G_OPS_BAIL_IF_ERROR (transition_tunnel (tunnel_id, OMX_CommandPortDisable),
                         "Unable to disable tunnel file reader->http renderer");
  }
}

void graph::httpservops::do_enable_tunnel ()
{
  if (last_op_succeeded ())
  {
    // there is only one tunnel in this graph
    const int tunnel_id = 0;
    G_OPS_BAIL_IF_ERROR (transition_tunnel (tunnel_id, OMX_CommandPortEnable),
                         "Unable to enable tunnel file reader->http renderer");
  }
}

bool graph::httpservops::is_initial_configuration () const
{
  return is_initial_configuration_;
}

void graph::httpservops::do_flag_initial_config_done ()
{
  // At this point, both the server and station have been configured. Will
  // switch this flag, so that this won't happen again during the lifetime of
  // this graph.
  is_initial_configuration_ = false;
}

OMX_ERRORTYPE
graph::httpservops::configure_server ()
{
  OMX_TIZONIA_HTTPSERVERTYPE httpsrv;
  httpsrv.nSize = sizeof(OMX_TIZONIA_HTTPSERVERTYPE);
  httpsrv.nVersion.nVersion = OMX_VERSION;

  tiz_check_omx_err (OMX_GetParameter (
      handles_[1],
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamHttpServer), &httpsrv));

  tizhttpservconfig_ptr_t srv_config
      = boost::dynamic_pointer_cast< httpservconfig >(config_);
  assert (srv_config);
  httpsrv.nListeningPort = srv_config->get_port ();
  httpsrv.nMaxClients = 1;  // the http renderer component supports only one
  // client, for now

  return OMX_SetParameter (
      handles_[1],
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamHttpServer), &httpsrv);
}

OMX_ERRORTYPE
graph::httpservops::configure_station ()
{
  OMX_TIZONIA_ICECASTMOUNTPOINTTYPE mount;
  mount.nSize = sizeof(OMX_TIZONIA_ICECASTMOUNTPOINTTYPE);
  mount.nVersion.nVersion = OMX_VERSION;
  mount.nPortIndex = 0;

  tizhttpservconfig_ptr_t srv_config
      = boost::dynamic_pointer_cast< httpservconfig >(config_);
  assert (srv_config);

  tiz_check_omx_err (OMX_GetParameter (
      handles_[1],
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamIcecastMountpoint),
      &mount));

  snprintf ((char *)mount.cMountName, sizeof(mount.cMountName), "/");
  snprintf ((char *)mount.cStationName, sizeof(mount.cStationName),
            "%s (%s:%ld)", srv_config->get_station_name ().c_str (),
            srv_config->get_host_name ().c_str (), srv_config->get_port ());
  snprintf ((char *)mount.cStationDescription,
            sizeof(mount.cStationDescription),
            "Tizonia Audio Streaming Server");
  snprintf ((char *)mount.cStationGenre, sizeof(mount.cStationGenre), "%s",
            srv_config->get_station_genre ().c_str ());
  snprintf ((char *)mount.cStationUrl, sizeof(mount.cStationUrl),
            "http://tizonia.org");

  // TODO: Will re-enable this when github's issue #44 is closed.
  // Disable ICY metadata for now
  mount.nIcyMetadataPeriod = 0;

  mount.eEncoding = OMX_AUDIO_CodingMP3;
  mount.nMaxClients = 1;
  return OMX_SetParameter (
      handles_[1],
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamIcecastMountpoint),
      &mount);
}

OMX_ERRORTYPE
graph::httpservops::configure_stream_metadata ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  // Set the stream title on to the renderer's input port
  OMX_TIZONIA_ICECASTMETADATATYPE *p_metadata = NULL;
  if (NULL == (p_metadata = (OMX_TIZONIA_ICECASTMETADATATYPE *)tiz_mem_calloc (
                   1, sizeof(OMX_TIZONIA_ICECASTMETADATATYPE)
                      + OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE)))
  {
    rc = OMX_ErrorInsufficientResources;
  }
  else
  {
    p_metadata->nVersion.nVersion = OMX_VERSION;
    p_metadata->nPortIndex = 0;

    // Obtain the stream title
    std::string stream_title = probe_ptr_->get_stream_title ();
    snprintf ((char *)p_metadata->cStreamTitle,
              OMX_TIZONIA_MAX_SHOUTCAST_METADATA_SIZE, "StreamTitle='%s';",
              stream_title.c_str ());
    p_metadata->nSize = sizeof(OMX_TIZONIA_ICECASTMETADATATYPE)
                        + strlen ((char *)p_metadata->cStreamTitle);

    TIZ_LOG (TIZ_PRIORITY_TRACE, "p_metadata->cStreamTitle [%s]...",
             p_metadata->cStreamTitle);

    rc = OMX_SetConfig (handles_[1], static_cast< OMX_INDEXTYPE >(
                                         OMX_TizoniaIndexConfigIcecastMetadata),
                        p_metadata);

    tiz_mem_free (p_metadata);
    p_metadata = NULL;
  }

  return rc;
}

OMX_ERRORTYPE
graph::httpservops::transition_tunnel (const int tunnel_id,
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
    const int file_reader_index = 0;
    const int file_reader_input_port = 0;
    add_expected_port_transition (handles_[file_reader_index],
                                  file_reader_input_port,
                                  to_disabled_or_enabled);
    const int http_renderer_index = 1;
    const int http_renderer_input_port = 0;
    add_expected_port_transition (handles_[http_renderer_index],
                                  http_renderer_input_port,
                                  to_disabled_or_enabled);
  }
  return rc;
}

bool graph::httpservops::probe_stream_hook ()
{
  bool rc = false;
  if (probe_ptr_ && config_)
  {
    tizhttpservconfig_ptr_t srv_config
        = boost::dynamic_pointer_cast< httpservconfig >(config_);
    assert (srv_config);

    OMX_AUDIO_PARAM_MP3TYPE mp3type;
    probe_ptr_->get_mp3_codec_info (mp3type);

    // Skip streams with sampling rates different to the ones received in the
    // server configuration, or process all if the list is empty.
    const std::vector< int > &rates = srv_config->get_sampling_rates ();

    rc = true;
    if (!rates.empty ())
    {
      rc &= std::find (rates.begin (), rates.end (), mp3type.nSampleRate)
            != rates.end ();
      TIZ_LOG (TIZ_PRIORITY_TRACE, "nSampleRate [%d] found [%s]...",
               mp3type.nSampleRate, rc ? "YES" : "NOT");
    }

    // Skip streams with bitrate types different to the ones received in the
    // server configuration, or process all if the list is empty.
    TIZ_LOG (TIZ_PRIORITY_TRACE, "is_cbr_stream () [%s]...",
             probe_ptr_->is_cbr_stream () ? "YES" : "NO");
    const std::vector< std::string > &bitrate_types = srv_config->get_bitrate_modes ();
    if (!bitrate_types.empty ())
    {
      rc &= std::find (bitrate_types.begin (), bitrate_types.end (),
                       probe_ptr_->is_cbr_stream () ? "CBR" : "VBR")
        != bitrate_types.end ();
    }
  }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "return () [%s]...", rc ? "YES" : "NO");

  return rc;
}

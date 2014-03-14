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
 * @file   tizhttpservgraphops.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL HTTP Streaming Server graph implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_TizoniaExt.h>
#include <tizosal.h>

#include "tizgraphutil.h"
#include "tizprobe.h"
#include "tizgraph.h"
#include "tizhttpservconfig.h"
#include "tizhttpservgraphops.h"

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
  TIZ_LOG (TIZ_PRIORITY_TRACE, "current_file_index_ [%d]...",
           current_file_index_);
  assert (current_file_index_ < file_list_.size ());
  G_OPS_BAIL_IF_ERROR (probe_uri (current_file_index_), "Unable to probe uri.");
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

void graph::httpservops::do_source_omx_loaded2idle ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (transition_source (OMX_StateIdle),
                         "Unable to transition file reader from Loaded->Idle");
  }
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
    G_OPS_BAIL_IF_ERROR (transition_tunnel (OMX_CommandPortDisable),
                         "Unable to disable tunnel file reader->http renderer");
  }
}

void graph::httpservops::do_enable_tunnel ()
{
  if (last_op_succeeded ())
  {
    G_OPS_BAIL_IF_ERROR (transition_tunnel (OMX_CommandPortEnable),
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
graph::httpservops::probe_uri (const int uri_index, const bool quiet)
{
  assert (uri_index < file_list_.size ());

  const std::string &uri = file_list_[uri_index];

  if (!uri.empty ())
  {
    // Probe a new uri
    probe_ptr_.reset ();
    bool quiet_probing = true;
    probe_ptr_ = boost::make_shared< tiz::probe >(uri, quiet_probing);
    if (probe_ptr_->get_omx_domain () != OMX_PortDomainAudio
        || probe_ptr_->get_audio_coding_type () != OMX_AUDIO_CodingMP3)
    {
      return OMX_ErrorContentURIError;
    }
    if (!quiet)
    {
      tiz::graph::util::dump_graph_info ("mp3/http", "stream", uri);
      probe_ptr_->dump_stream_metadata ();
      probe_ptr_->dump_mp3_info ();
    }
  }
  return OMX_ErrorNone;
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

  bool quiet = true;
  tiz_check_omx_err (probe_uri (0, quiet));

  tizhttpservconfig_ptr_t srv_config
      = boost::dynamic_pointer_cast< httpservconfig >(config_);
  assert (srv_config);

  tiz_check_omx_err (OMX_GetParameter (
      handles_[1],
      static_cast< OMX_INDEXTYPE >(OMX_TizoniaIndexParamIcecastMountpoint),
      &mount));

  snprintf ((char *)mount.cMountName, sizeof(mount.cMountName), "/");
  snprintf ((char *)mount.cStationName, sizeof(mount.cStationName),
            "Tizonia Radio Station (%s:%ld)",
            srv_config->get_host_name ().c_str (), srv_config->get_port ());
  snprintf ((char *)mount.cStationDescription,
            sizeof(mount.cStationDescription),
            "Audio Streaming with OpenMAX IL");
  snprintf ((char *)mount.cStationGenre, sizeof(mount.cStationGenre), "%s",
            probe_ptr_->get_stream_genre ().c_str ());
  snprintf ((char *)mount.cStationUrl, sizeof(mount.cStationUrl),
            "http://tizonia.org");
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
graph::httpservops::transition_source (const OMX_STATETYPE to_state)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const int file_reader_index = 0;
  rc = tiz::graph::util::transition_one (handles_, file_reader_index, to_state);
  if (OMX_ErrorNone == rc)
  {
    clear_expected_transitions ();
    add_expected_transition (handles_[file_reader_index], to_state);
  }
  return rc;
}

OMX_ERRORTYPE
graph::httpservops::transition_tunnel (
    const OMX_COMMANDTYPE to_disabled_or_enabled)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const int tunnel_id = 0;  // there is only one tunnel in this graph

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

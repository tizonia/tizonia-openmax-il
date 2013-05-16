/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraph.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph base class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizstreamsrvgraph.h"
#include "tizprobe.h"
#include "tizosal.h"

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_TizoniaExt.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>


#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.streamsrv"
#endif

tizstreamsrvgraph::tizstreamsrvgraph (tizprobe_ptr_t probe_ptr)
  : tizgraph (2, probe_ptr),
    file_list_ (),
    current_file_index_ (0),
    config_ ()
{
}

OMX_ERRORTYPE
tizstreamsrvgraph::do_load ()
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;

  component_names_t comp_list;
  comp_list.push_back ("OMX.Aratelia.file_reader.binary");
  comp_list.push_back ("OMX.Aratelia.ice_renderer.http");

  if (OMX_ErrorNone != (ret = verify_existence (comp_list)))
    {
      return ret;
    }

  component_roles_t role_list;
  role_list.push_back ("audio_reader.binary");
  role_list.push_back ("ice_renderer.http");

  if (OMX_ErrorNone != (ret = verify_role_list (comp_list, role_list)))
    {
      return ret;
    }

  if (OMX_ErrorNone != (ret = instantiate_list (comp_list)))
    {
      return ret;
    }

  return ret;
}

OMX_ERRORTYPE
tizstreamsrvgraph::configure_streamsrv_graph ()
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;

  TIZ_LOG (TIZ_TRACE, "Configure current_file_index_ [%d]...",
           current_file_index_);

  if (current_file_index_ < 0 || current_file_index_ >= file_list_.size ())
    {
      current_file_index_ = 0;
    }

  const std::string &uri = file_list_[current_file_index_++];

  if (!uri.empty ())
    {
      // Probe a new uri
      probe_ptr_.reset ();
      probe_ptr_ = boost::make_shared < tizprobe > (uri);
      if (probe_ptr_->get_omx_domain () != OMX_PortDomainAudio
          || probe_ptr_->get_audio_coding_type () != OMX_AUDIO_CodingMP3)
        {
          return OMX_ErrorContentURIError;
        }
    }

  // Set the new URI
  OMX_PARAM_CONTENTURITYPE *p_uritype = NULL;

  if (NULL == (p_uritype = (OMX_PARAM_CONTENTURITYPE *) tiz_mem_calloc
               (1, sizeof (OMX_PARAM_CONTENTURITYPE)
                + OMX_MAX_STRINGNAME_SIZE)))
    {
      return OMX_ErrorInsufficientResources;
    }

  p_uritype->nSize = sizeof (OMX_PARAM_CONTENTURITYPE)
    + OMX_MAX_STRINGNAME_SIZE;
  p_uritype->nVersion.nVersion = OMX_VERSION;

  strncpy ((char *) p_uritype->contentURI, probe_ptr_->get_uri ().c_str (),
           OMX_MAX_STRINGNAME_SIZE);
  p_uritype->contentURI[strlen (probe_ptr_->get_uri ().c_str ())] = '\0';

  ret = OMX_SetParameter (handles_[0], OMX_IndexParamContentURI,
                          p_uritype);

  tiz_mem_free (p_uritype);
  p_uritype = NULL;

  if (OMX_ErrorNone != ret)
    {
      return ret;
    }

  OMX_TIZONIA_PARAM_HTTPSERVERTYPE httpsrv;
  httpsrv.nSize             = sizeof (OMX_TIZONIA_PARAM_HTTPSERVERTYPE);
  httpsrv.nVersion.nVersion = OMX_VERSION;

  tiz_check_omx_err
    (OMX_GetParameter
     (handles_[1],
      static_cast<OMX_INDEXTYPE>(OMX_TizoniaIndexParamHttpServer),
      &httpsrv));

  tizstreamsrvconfig_ptr_t srv_config
    = boost::dynamic_pointer_cast<tizstreamsrvconfig>(config_);
  assert (srv_config);
  httpsrv.nListeningPort = srv_config->get_port ();
  httpsrv.nMaxClients    = 1; // the http renderer component supports only one
                              // client, for now

  return OMX_SetParameter
    (handles_[1],
     static_cast<OMX_INDEXTYPE>(OMX_TizoniaIndexParamHttpServer),
     &httpsrv);
}

OMX_ERRORTYPE
tizstreamsrvgraph::do_configure (const tizgraphconfig_ptr_t config)
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;

  config_             = config;
  file_list_          = config->get_uris ();
  current_file_index_ = 0;

  tiz_check_omx_err (setup_suppliers ());
  tiz_check_omx_err (setup_tunnels ());

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizstreamsrvgraph::do_execute ()
{
  assert (OMX_StateLoaded == current_graph_state_);

  if (current_file_index_ < file_list_.size ())
    {
      tiz_check_omx_err (configure_streamsrv_graph ());
      tiz_check_omx_err (transition_all (OMX_StateIdle, OMX_StateLoaded));
      tiz_check_omx_err (transition_all (OMX_StateExecuting, OMX_StateIdle));
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizstreamsrvgraph::do_pause ()
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizstreamsrvgraph::do_seek ()
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizstreamsrvgraph::do_skip (const int jump)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizstreamsrvgraph::do_volume ()
{
  return OMX_ErrorNone;
}

void
tizstreamsrvgraph::do_unload ()
{
  (void) transition_all (OMX_StateIdle, OMX_StateExecuting);
  (void) transition_all (OMX_StateLoaded, OMX_StateIdle);
  tear_down_tunnels ();
  destroy_list ();
}

void
tizstreamsrvgraph::do_eos (const OMX_HANDLETYPE handle)
{
  if (handle == handles_[1])
    {
      int tunnel_id = 0; // there is only one tunnel in this graph
      int file_reader_id = 0; // here we are interested in the file reader
      (void) disable_tunnel (tunnel_id);
      (void) transition_one (file_reader_id, OMX_StateIdle);
      (void) transition_one (file_reader_id, OMX_StateLoaded);
      (void) configure_streamsrv_graph ();
      (void) transition_one (file_reader_id, OMX_StateIdle);
      (void) transition_one (file_reader_id, OMX_StateExecuting);
      (void) enable_tunnel (tunnel_id);
    }
}

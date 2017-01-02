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
 * @file   tizhttpservmgr.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Implementation of a Manager for the HTTP server graph
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <boost/assign/list_of.hpp> // for 'list_of()'
#include <boost/make_shared.hpp>

#include <tizplatform.h>

#include <tizgraphmgrcaps.hpp>
#include "tizhttpservgraph.hpp"
#include "tizhttpservmgr.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.httpservmgr"
#endif

namespace graphmgr = tiz::graphmgr;

//
// mgr
//
graphmgr::httpservmgr::httpservmgr (tizgraphconfig_ptr_t config)
  : graphmgr::mgr (), config_ (config)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

graphmgr::httpservmgr::~httpservmgr ()
{
}

graphmgr::ops *graphmgr::httpservmgr::do_init (
    const tizplaylist_ptr_t &playlist, const termination_callback_t &termination_cback,
    graphmgr_capabilities_t &graphmgr_caps)
{
  // Fill this graph manager capabilities
  graphmgr_caps.can_quit_ = false;
  graphmgr_caps.can_raise_ = false;
  graphmgr_caps.has_track_list_ = true;
  graphmgr_caps.identity_.assign ("Tizonia version ");
  graphmgr_caps.identity_.append (PACKAGE_VERSION);
  graphmgr_caps.uri_schemes_ = boost::assign::list_of ("http");
  graphmgr_caps.mime_types_ = boost::assign::list_of
    ("audio/mpeg")("audio/mpg")("audio/mp3");
  graphmgr_caps.minimum_rate_ = 1.0;
  graphmgr_caps.maximum_rate_ = 1.0;
  graphmgr_caps.can_go_next_ = false;
  graphmgr_caps.can_go_previous_ = false;
  graphmgr_caps.can_play_ = true;
  graphmgr_caps.can_pause_ = false;
  graphmgr_caps.can_seek_ = false;
  graphmgr_caps.can_control_ = false;

  return new httpservmgrops (this, playlist, termination_cback);
}

//
// decodemgrops
//
graphmgr::httpservmgrops::httpservmgrops (mgr *p_mgr,
                                          const tizplaylist_ptr_t &playlist,
                                          const termination_callback_t &termination_cback)
  : tiz::graphmgr::ops (p_mgr, playlist, termination_cback)
{
}

tizgraph_ptr_t graphmgr::httpservmgrops::get_graph (
    const std::string & /* uri */)
{
  tizgraph_ptr_t g_ptr;
  std::string encoding ("http/mp3");
  tizgraph_ptr_map_t::const_iterator it = graph_registry_.find (encoding);
  if (it == graph_registry_.end ())
  {
    g_ptr = boost::make_shared< tiz::graph::httpserver >();
    if (g_ptr)
    {
      // TODO: Check rc
      std::pair< tizgraph_ptr_map_t::iterator, bool > rc
          = graph_registry_.insert (
              std::make_pair< std::string, tizgraph_ptr_t >(encoding, g_ptr));
      if (rc.second)
      {
        // TODO: Check rc
        g_ptr->init ();
        g_ptr->set_manager (p_mgr_);
      }
      else
      {
        GMGR_OPS_RECORD_ERROR (OMX_ErrorInsufficientResources,
                               "Unable to register a new graph.");
      }
    }
    else
    {
      GMGR_OPS_RECORD_ERROR (OMX_ErrorInsufficientResources,
                             "Unable to create the http server graph.");
    }
  }
  else
  {
    g_ptr = it->second;
  }

  return g_ptr;
}

void graphmgr::httpservmgrops::do_load ()
{
  tizgraph_ptr_t g_ptr (get_graph (std::string ()));
  if (g_ptr)
  {
    GMGR_OPS_BAIL_IF_ERROR (g_ptr, g_ptr->load (), "Unable to load the graph.");
  }
  p_managed_graph_ = g_ptr;
}

void graphmgr::httpservmgrops::do_execute ()
{
  assert (playlist_);
  assert (p_mgr_);

  httpservmgr *p_servermgr = dynamic_cast< httpservmgr * >(p_mgr_);
  assert (p_servermgr);

  graph_config_.reset ();
  graph_config_ = p_servermgr->config_;
  assert (graph_config_);

  GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_,
                          p_managed_graph_->execute (graph_config_),
                          "Unable to execute the graph.");
}

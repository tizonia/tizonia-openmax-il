/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tiziheartmgr.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Manager implementation for the Iheart client graph
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <boost/assign/list_of.hpp>  // for 'list_of()'
#include <boost/make_shared.hpp>

#include <tizplatform.h>

#include "tizgraphmgrcaps.hpp"
#include "tiziheartgraph.hpp"
#include "tiziheartmgr.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.iheartmgr"
#endif

namespace graphmgr = tiz::graphmgr;

//
// mgr
//
graphmgr::iheartmgr::iheartmgr (tizgraphconfig_ptr_t config)
  : graphmgr::mgr (), config_ (config)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

graphmgr::iheartmgr::~iheartmgr ()
{
}

graphmgr::ops *graphmgr::iheartmgr::do_init (
    const tizplaylist_ptr_t &playlist,
    const termination_callback_t &termination_cback,
    graphmgr_capabilities_t &graphmgr_caps)
{
  // Fill this graph manager capabilities
  graphmgr_caps.can_quit_ = false;
  graphmgr_caps.can_raise_ = false;
  graphmgr_caps.has_track_list_ = false;
  graphmgr_caps.identity_.assign ("Tizonia version ");
  graphmgr_caps.identity_.append (PACKAGE_VERSION);
  graphmgr_caps.uri_schemes_
      = boost::assign::list_of ("iheart")
            .convert_to_container< std::vector< std::string > > ();
  graphmgr_caps.mime_types_
      = boost::assign::list_of ("audio/pcm")
            .convert_to_container< std::vector< std::string > > ();
  graphmgr_caps.minimum_rate_ = 1.0;
  graphmgr_caps.maximum_rate_ = 1.0;
  graphmgr_caps.can_go_next_ = false;
  graphmgr_caps.can_go_previous_ = false;
  graphmgr_caps.can_play_ = true;
  graphmgr_caps.can_pause_ = false;
  graphmgr_caps.can_seek_ = false;
  graphmgr_caps.can_control_ = false;

  return new iheartmgrops (this, playlist, termination_cback);
}

//
// decodemgrops
//
graphmgr::iheartmgrops::iheartmgrops (
    mgr *p_mgr, const tizplaylist_ptr_t &playlist,
    const termination_callback_t &termination_cback)
  : tiz::graphmgr::ops (p_mgr, playlist, termination_cback)
{
}

tizgraph_ptr_t graphmgr::iheartmgrops::get_graph (
    const std::string & /* uri */)
{
  tizgraph_ptr_t g_ptr;
  std::string encoding ("pcm");
  tizgraph_ptr_map_t::const_iterator it = graph_registry_.find (encoding);
  if (it == graph_registry_.end ())
  {
    g_ptr = boost::make_shared< tiz::graph::iheart >();
    if (g_ptr)
    {
      // TODO: Check rc
      std::pair< tizgraph_ptr_map_t::iterator, bool > rc
          = graph_registry_.insert (std::make_pair (encoding, g_ptr));
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
                             "Unable to create the Iheart client graph.");
    }
  }
  else
  {
    g_ptr = it->second;
  }

  return g_ptr;
}

void graphmgr::iheartmgrops::do_load ()
{
  tizgraph_ptr_t g_ptr (get_graph (std::string ()));
  if (g_ptr)
  {
    GMGR_OPS_BAIL_IF_ERROR (g_ptr, g_ptr->load (), "Unable to load the graph.");
  }
  p_managed_graph_ = g_ptr;
}

void graphmgr::iheartmgrops::do_execute ()
{
  assert (playlist_);
  assert (p_mgr_);

  iheartmgr *p_clientmgr = dynamic_cast< iheartmgr * >(p_mgr_);
  assert (p_clientmgr);

  graph_config_.reset ();
  graph_config_ = p_clientmgr->config_;
  assert (graph_config_);

  GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_,
                          p_managed_graph_->execute (graph_config_),
                          "Unable to execute the graph.");
}

bool graphmgr::iheartmgrops::is_fatal_error (const OMX_ERRORTYPE error,
                                              const std::string &msg)
{
  bool rc = false;
  TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (error),
           msg.c_str ());
  if (error == OMX_ErrorContentURIError)
  {
    // If the source component reports this error, the playlist is not avaiable.
    error_msg_.assign ("Playlist not found.");
    rc = true;
  }
  else
  {
    rc = graphmgr::ops::is_fatal_error (error, msg);
  }
  return rc;
}

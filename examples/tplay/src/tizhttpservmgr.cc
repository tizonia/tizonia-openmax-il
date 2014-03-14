/* -*-Mode: c++; -*- */
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
 * @file   tizhttpservmgr.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph manager base class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <boost/make_shared.hpp>

#include <tizosal.h>

#include "tizhttpservgraph.h"
#include "tizhttpservmgr.h"

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
    const uri_lst_t &file_list, const error_callback_t &error_cback)
{
  return new httpservmgrops (this, file_list, error_cback);
}

//
// decodemgrops
//
graphmgr::httpservmgrops::httpservmgrops (mgr *p_mgr,
                                          const uri_lst_t &file_list,
                                          const error_callback_t &error_cback)
  : tiz::graphmgr::ops (p_mgr, file_list, error_cback)
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
    ;
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
      std::string msg ("Unable to create the http server graph.");
      GMGR_OPS_RECORD_ERROR (OMX_ErrorInsufficientResources, msg);
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
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  tizgraph_ptr_t g_ptr (get_graph (std::string ()));
  if (g_ptr)
  {
    const bool continuous_play = true;
    assert (p_mgr_);

    httpservmgr *p_servermgr = dynamic_cast< httpservmgr * >(p_mgr_);
    assert (p_servermgr);
    graph_config_.reset ();
    graph_config_ = p_servermgr->config_;
    GMGR_OPS_BAIL_IF_ERROR (g_ptr, g_ptr->load (), "Unable to load the graph.");
  }

  p_managed_graph_ = g_ptr;
}

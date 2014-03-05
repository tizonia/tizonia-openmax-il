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
 * @file   tizgraphmgrops.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph manager operations impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/make_shared.hpp>

#include <tizosal.h>
#include <tizmacros.h>

#include "tizgraphfactory.h"
#include "tizgraph.h"
#include "tizgraphconfig.h"
#include "tizgraphmgrops.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graphmgr.ops"
#endif

namespace graphmgr = tiz::graphmgr;

#define GMGR_OPS_RECORD_ERROR(err, str)                                     \
  do                                                                        \
  {                                                                         \
    error_msg_.assign (str);                                                \
    error_code_ = err;                                                      \
    TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (error_code_), \
             error_msg_.c_str ());                                          \
  } while (0)

#define GMGR_OPS_CHECK_FOR_ERROR(ptr, exp, str) \
  do                                            \
  {                                             \
    if (ptr)                                    \
    {                                           \
      OMX_ERRORTYPE rc_ = OMX_ErrorNone;        \
      if (OMX_ErrorNone != (rc_ = (exp)))       \
      {                                         \
        GMGR_OPS_RECORD_ERROR (rc_, str);       \
      }                                         \
    }                                           \
  } while (0)

//
// ops
//
graphmgr::ops::ops (graphmgr::mgr *p_mgr, const uri_lst_t &file_list,
                    const error_callback_t &error_cback)
  : p_mgr_ (p_mgr),
    playlist_ (file_list),
    graph_config_ (),
    graph_registry_ (),
    p_managed_graph_ (),
    error_cback_ (error_cback),
    error_code_ (OMX_ErrorNone),
    error_msg_ ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

graphmgr::ops::~ops ()
{
}

void graphmgr::ops::deinit ()
{
  tizgraph_ptr_map_t::iterator registry_end = graph_registry_.end ();

  for (tizgraph_ptr_map_t::iterator it = graph_registry_.begin ();
       it != registry_end; ++it)
  {
    tizgraph_ptr_t p_graph = (*it).second;
    if (p_graph)
    {
      p_graph->deinit ();
    }
  }
  graph_registry_.clear ();
}

tizgraph_ptr_t graphmgr::ops::get_graph (const std::string &uri)
{
  tizgraph_ptr_t g_ptr;
  std::string encoding (tiz::graph::factory::coding_type (uri));
  tizgraph_ptr_map_t::const_iterator it = graph_registry_.find (encoding);
  if (it == graph_registry_.end ())
  {
    g_ptr = tiz::graph::factory::create_graph (uri);
    if (g_ptr)
    {
      // TODO: Check rc
      std::pair<tizgraph_ptr_map_t::iterator, bool> rc
          = graph_registry_.insert (
              std::make_pair<std::string, tizgraph_ptr_t>(encoding, g_ptr));
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
      std::string msg ("Unable to create graph for encoding [");
      msg.append (encoding.c_str ());
      msg.append ("].");
      GMGR_OPS_RECORD_ERROR (OMX_ErrorInsufficientResources, msg);
    }
  }
  else
  {
    g_ptr = it->second;
  }

  return g_ptr;
}

void graphmgr::ops::do_load ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  tizplaylist_t sub_playlist = playlist_.get_next_sub_playlist ();
  const uri_lst_t &sub_urilist = sub_playlist.get_uri_list ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "sub_urilist size %d", sub_urilist.size ());

  tizgraph_ptr_t g_ptr (get_graph (sub_urilist[0]));

  if (g_ptr)
  {
    const bool continuous_play
        = (tizplaylist (sub_urilist).is_single_format_playlist ()
           && sub_urilist.size () > 1);
    graph_config_.reset ();
    graph_config_
        = boost::make_shared<tiz::graph::config>(sub_urilist, continuous_play);

    GMGR_OPS_CHECK_FOR_ERROR (g_ptr, g_ptr->load (),
                              "Unable to load the graph.");
  }

  p_managed_graph_ = g_ptr;
}

void graphmgr::ops::do_execute ()
{
  GMGR_OPS_CHECK_FOR_ERROR (p_managed_graph_,
                            p_managed_graph_->execute (graph_config_),
                            "Unable to execute the graph.");
}

void graphmgr::ops::do_unload ()
{
  if (p_managed_graph_)
  {
    p_managed_graph_->unload ();
  }
}

void graphmgr::ops::do_next ()
{
  GMGR_OPS_CHECK_FOR_ERROR (p_managed_graph_, p_managed_graph_->skip (1),
                            "Unable to skip to next song.");
}

void graphmgr::ops::do_prev ()
{
  GMGR_OPS_CHECK_FOR_ERROR (p_managed_graph_, p_managed_graph_->skip (-1),
                            "Unable to skip to prev song.");
}

void graphmgr::ops::do_fwd ()
{
  // TODO:
}

void graphmgr::ops::do_rwd ()
{
  // TODO:
}

void graphmgr::ops::do_vol_up ()
{
  GMGR_OPS_CHECK_FOR_ERROR (p_managed_graph_, p_managed_graph_->volume (1),
                            "Unable to inc. volume.");
}

void graphmgr::ops::do_vol_down ()
{
  GMGR_OPS_CHECK_FOR_ERROR (p_managed_graph_, p_managed_graph_->volume (-1),
                            "Unable to dec. volume.");
}

void graphmgr::ops::do_mute ()
{
  GMGR_OPS_CHECK_FOR_ERROR (p_managed_graph_, p_managed_graph_->mute (),
                            "Unable to mute/unmute.");
}

void graphmgr::ops::do_pause ()
{
  GMGR_OPS_CHECK_FOR_ERROR (p_managed_graph_, p_managed_graph_->pause (),
                            "Unable to pause.");
}

void graphmgr::ops::do_report_fatal_error (const OMX_ERRORTYPE error,
                                           const std::string &msg)
{
  TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (error),
           msg.c_str ());
  error_cback_ (error, msg);
}

OMX_ERRORTYPE
graphmgr::ops::get_internal_error () const
{
  return error_code_;
}

std::string graphmgr::ops::get_internal_error_msg () const
{
  return error_msg_;
}

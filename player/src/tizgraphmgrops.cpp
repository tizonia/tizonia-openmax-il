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
 * @file   tizgraphmgrops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph manager operations
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/make_shared.hpp>

#include <tizplatform.h>
#include <tizmacros.h>

#include "tizgraphfactory.hpp"
#include "tizgraph.hpp"
#include "tizgraphmgr.hpp"
#include "tizgraphconfig.hpp"
#include "tizgraphmgrops.hpp"
#include "tizgraphutil.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graphmgr.ops"
#endif

namespace graphmgr = tiz::graphmgr;
namespace control = tiz::control;

//
// ops
//
graphmgr::ops::ops (graphmgr::mgr *p_mgr, const tizplaylist_ptr_t &playlist,
                    const termination_callback_t &termination_cback)
  : p_mgr_ (p_mgr),
    playlist_ (playlist),
    next_playlist_ (),
    graph_config_ (),
    graph_registry_ (),
    p_managed_graph_ (),
    termination_cback_ (termination_cback),
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

  termination_cback_ (OMX_ErrorNone, "");
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
      std::string msg ("Unable to create a graph for [");
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
  next_playlist_ = find_next_sub_list ();

  if (next_playlist_)
  {
    const uri_lst_t &next_urilist = next_playlist_->get_uri_list ();
    TIZ_LOG (TIZ_PRIORITY_TRACE, "next_urilist size %d", next_urilist.size ());

    tizgraph_ptr_t g_ptr (get_graph (next_urilist[0]));
    if (g_ptr)
    {
      GMGR_OPS_BAIL_IF_ERROR (g_ptr, g_ptr->load (),
                              "Unable to load the graph.");
    }
    p_managed_graph_ = g_ptr;
  }
  else
  {
    GMGR_OPS_RECORD_ERROR (OMX_ErrorInsufficientResources,
                           "Unable to allocate the next playlist.");
  }
}

void graphmgr::ops::do_execute ()
{
  assert (playlist_);
  assert (next_playlist_);

  next_playlist_->set_loop_playback (playlist_->single_format ());
  graph_config_.reset ();
  graph_config_ = boost::make_shared< tiz::graph::config >(next_playlist_);

  if (graph_config_)
  {
    GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_,
                            p_managed_graph_->execute (graph_config_),
                            "Unable to execute the graph.");
  }
  else
  {
    GMGR_OPS_RECORD_ERROR (
        OMX_ErrorInsufficientResources,
        "Unable to allocate the graph configuration object.");
  }
}

void graphmgr::ops::do_stop ()
{
  if (p_managed_graph_)
  {
    p_managed_graph_->stop ();
  }
}

void graphmgr::ops::do_unload ()
{
  if (p_managed_graph_)
  {
    p_managed_graph_->unload ();
  }
}

void graphmgr::ops::do_deinit ()
{
  if (p_managed_graph_)
  {
    p_managed_graph_->deinit ();

    tizgraph_ptr_map_t::iterator registry_end = graph_registry_.end ();
    for (tizgraph_ptr_map_t::iterator it = graph_registry_.begin ();
         it != registry_end; ++it)
      {
        const tizgraph_ptr_t p_graph = (*it).second;
        if (p_graph == p_managed_graph_)
          {
            graph_registry_.erase (it);
            break;
          }
      }
    p_managed_graph_.reset ();
  }
}

void graphmgr::ops::do_next ()
{
  GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_, p_managed_graph_->skip (1),
                          "Unable to skip to next song.");
}

void graphmgr::ops::do_prev ()
{
  GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_, p_managed_graph_->skip (-1),
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
  GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_, p_managed_graph_->volume_step (1),
                          "Unable to inc. volume.");
}

void graphmgr::ops::do_vol_down ()
{
  GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_, p_managed_graph_->volume_step (-1),
                          "Unable to dec. volume.");
}

void graphmgr::ops::do_vol (const double vol)
{
  GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_, p_managed_graph_->volume (vol),
                          "Unable to change volume.");
}

void graphmgr::ops::do_mute ()
{
  GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_, p_managed_graph_->mute (),
                          "Unable to mute/unmute.");
}

void graphmgr::ops::do_pause ()
{
  GMGR_OPS_BAIL_IF_ERROR (p_managed_graph_, p_managed_graph_->pause (),
                          "Unable to pause.");
}

void graphmgr::ops::do_report_fatal_error (const OMX_ERRORTYPE error,
                                           const std::string &msg)
{
  TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (error),
           msg.c_str ());
  termination_cback_ (error, msg);
}

void graphmgr::ops::do_end_of_play ()
{
  termination_cback_ (OMX_ErrorNone, "End of playlist.");
}

void graphmgr::ops::do_update_control_ifcs (const control::playback_status_t status)
{
  if (p_mgr_)
    {
      std::string current_stream;
      if (next_playlist_)
        {
          current_stream = next_playlist_->get_current_uri ();
        }
      p_mgr_->do_update_control_ifcs (status, current_stream);
    }
}

void graphmgr::ops::do_update_metadata (const track_metadata_map_t &metadata)
{
  if (p_mgr_)
    {
      p_mgr_->do_update_metadata (metadata);
    }
}

void graphmgr::ops::do_update_volume (const int volume)
{
  if (p_mgr_)
    {
      p_mgr_->do_update_volume (volume);
    }
}

bool graphmgr::ops::is_fatal_error (const OMX_ERRORTYPE error,
                                    const std::string &msg)
{
  TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (error),
           msg.c_str ());
  // This is a generic implementation. We use here some common understanding of
  // fatal errors. Each manager graph may decide to use its own list of fatal
  // errors.
  return tiz::graph::util::is_fatal_error (error);
}

OMX_ERRORTYPE
graphmgr::ops::internal_error () const
{
  return error_code_;
}

std::string graphmgr::ops::internal_error_msg () const
{
  return error_msg_;
}

tizplaylist_ptr_t graphmgr::ops::find_next_sub_list () const
{
  tizplaylist_ptr_t next_lst;

  assert (playlist_);

  tiz::playlist::list_direction_t dir = tiz::playlist::DirUp;
  if (next_playlist_ && next_playlist_->size () > 0)
  {
    dir = next_playlist_->past_end () ? tiz::playlist::DirUp
                                      : tiz::playlist::DirDown;
  }

  next_lst = boost::make_shared< tiz::playlist >(
      playlist_->obtain_next_sub_playlist (dir));
  if (next_playlist_ && next_playlist_->before_begin ())
  {
    if (next_lst)
    {
      next_lst->set_index (next_lst->size () - 1);
    }
  }

  return next_lst;
}

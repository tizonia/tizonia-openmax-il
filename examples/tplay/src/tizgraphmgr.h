/* -*-Mode: c++; -*- */
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
 * @file   tizgraphmgr.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OMX IL graph manager
 *
 *
 */

#ifndef TIZGRAPHMGR_H
#define TIZGRAPHMGR_H

#include "tizgraphtypes.h"
#include "tizplaylist.h"

#include <tizosal.h>
#include <OMX_Core.h>

#include <assert.h>

class tizgraphmgrcmd
{

public:

  enum cmd_type
  {
    ETIZGraphMgrCmdStart,
    ETIZGraphMgrCmdNext,
    ETIZGraphMgrCmdPrev,
    ETIZGraphMgrCmdFwd,
    ETIZGraphMgrCmdRwd,
    ETIZGraphMgrCmdVolume,
    ETIZGraphMgrCmdPause,
    ETIZGraphMgrCmdStop,
    ETIZGraphMgrCmdGraphEop,
    ETIZGraphMgrCmdGraphError,
    ETIZGraphMgrCmdMax
  };

  tizgraphmgrcmd (const cmd_type type)
    : type_ (type)
  {assert (type_ < ETIZGraphMgrCmdMax);}

  cmd_type get_type () const {return type_;}

private:

  const cmd_type type_;

};

class tizgraphmgr
{

  friend class tizgraph;
  friend       void* ::g_graphmgr_thread_func (void *);

public:
  enum mgr_state
    {
      ETIZGraphMgrStateNull,
      ETIZGraphMgrStateInited,
      ETIZGraphMgrStateStarted,
      ETIZGraphMgrStateMax,
    };

public:

  tizgraphmgr(const uri_list_t &file_list);
  virtual ~tizgraphmgr ();

  OMX_ERRORTYPE init ();
  OMX_ERRORTYPE start ();
  OMX_ERRORTYPE next ();
  OMX_ERRORTYPE prev ();
  OMX_ERRORTYPE fwd ();
  OMX_ERRORTYPE rwd ();
  OMX_ERRORTYPE volume ();
  OMX_ERRORTYPE pause ();
  OMX_ERRORTYPE stop ();
  OMX_ERRORTYPE deinit ();

protected:

  OMX_ERRORTYPE graph_end_of_play ();
  OMX_ERRORTYPE graph_error ();

  OMX_ERRORTYPE do_start ();
  OMX_ERRORTYPE do_next ();
  OMX_ERRORTYPE do_prev ();
  OMX_ERRORTYPE do_fwd ();
  OMX_ERRORTYPE do_rwd ();
  OMX_ERRORTYPE do_vol ();
  OMX_ERRORTYPE do_pause ();
  OMX_ERRORTYPE do_stop ();
  OMX_ERRORTYPE do_graph_end_of_play ();
  OMX_ERRORTYPE do_graph_error ();

  OMX_ERRORTYPE send_msg (const tizgraphmgrcmd::cmd_type type);
  static void dispatch (tizgraphmgr *p_graph_mgr, const tizgraphmgrcmd *p_cmd);
  tizgraph_ptr_t get_graph (const std::string & uri);


  bool verify_mgr_state (tizgraphmgrcmd::cmd_type cmd);

protected:

  mgr_state  mgr_state_;
  tiz_thread_t   thread_;
  tiz_mutex_t    mutex_;
  tiz_sem_t      sem_;
  tiz_queue_t   *p_queue_;
  tizplaylist_t playlist_;
  tizgraph_ptr_map_t graph_registry_;
  tizgraph_ptr_t running_graph_ptr_;
};

#endif // TIZGRAPHMGR_H

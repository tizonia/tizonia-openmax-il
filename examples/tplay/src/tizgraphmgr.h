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
 * @file   tizgraphmgr.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph manager
 *
 *
 */

#ifndef TIZGRAPHMGR_H
#define TIZGRAPHMGR_H

#include "tizgraphtypes.h"
#include "tizplaylist.h"

#include <tizosal.h>
#include <OMX_Core.h>

#include <string>
#include <boost/function.hpp>

// Forward declarations
class tizgraphmgrcmd;

/**
 *  @class tizgraphmgr
 *  @brief The graph manager class.
 *
 *  A graph manager instantiates OpenMAX IL graphs according to the media types
 *  of the elements found in a play list. The graph manager runs in its own
 *  thread and will manage the lifetime of the graphs that it creates.
 */
class tizgraphmgr
{

  friend class tizgraph;
  friend       void* ::g_graphmgr_thread_func (void *);

public:

  typedef boost::function<void (OMX_ERRORTYPE, std::string)> error_callback_t;

public:

  enum mgr_state
    {
      ETIZGraphMgrStateNull,
      ETIZGraphMgrStateInited,
      ETIZGraphMgrStateStarted,
      ETIZGraphMgrStateMax,
    };

public:

  tizgraphmgr(const uri_list_t &file_list, const error_callback_t &error_cback);
  virtual ~tizgraphmgr ();

  /**
   * Initialises the graph manager thread.
   *
   * @pre This method must be called only once, before any call is made to the
   * other APIs.
   *
   * @post The graph manager thread is ready to process requests.
   *
   * @return OMX_ErrorNone if initialisation was
   * successful. OMX_ErrorInsuficientResources otherwise.
   */
  OMX_ERRORTYPE init ();

  /**
   * Start processing the play list from the beginning.
   *
   * @pre init() has been called on this manager.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE start ();

  /**
   * Process the next item in the playlist.
   *
   * @pre init() has been called on this manager.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE next ();

  /**
   * Process the previous item in the playlist.
   *
   * @pre init() has been called on this manager.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE prev ();

  /**
   * NOT IMPLEMENTED YET
   *
   * @pre init() has been called on this manager.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE fwd ();

  /**
   * NOT IMPLEMENTED YET
   *
   * @pre init() has been called on this manager.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE rwd ();

  /**
   * Change the volume.
   *
   * @pre init() has been called on this manager.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE volume (const int step);

  /**
   * Mute/unmute toggle.
   *
   * @pre init() has been called on this manager.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE mute ();

  /**
   * Pause the processing of the current item in the playlist.
   *
   * @pre init() has been called on this manager.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE pause ();

  /**
   * Halts processing of the playlist.
   *
   * @pre init() has been called on this manager.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE stop ();

  /**
   * Destroy the manager thread and releases all resources.
   *
   * @pre stop() has been called on this manager.
   *
   * @post Only init() can be called at this point.
   *
   * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
   * success.
   */
  OMX_ERRORTYPE deinit ();

protected:

  OMX_ERRORTYPE graph_end_of_play ();
  OMX_ERRORTYPE graph_error (OMX_ERRORTYPE error, std::string msg);

protected:

  OMX_ERRORTYPE do_start ();
  OMX_ERRORTYPE do_next ();
  OMX_ERRORTYPE do_prev ();
  OMX_ERRORTYPE do_fwd ();
  OMX_ERRORTYPE do_rwd ();
  OMX_ERRORTYPE do_vol_up ();
  OMX_ERRORTYPE do_vol_down ();
  OMX_ERRORTYPE do_mute ();
  OMX_ERRORTYPE do_pause ();
  OMX_ERRORTYPE do_stop ();
  OMX_ERRORTYPE do_graph_end_of_play ();

  OMX_ERRORTYPE send_cmd (tizgraphmgrcmd *p_cmd);
  static void dispatch (tizgraphmgr *p_graph_mgr, const tizgraphmgrcmd *p_cmd);
  tizgraph_ptr_t get_graph (const std::string & uri);

protected:

  mgr_state mgr_state_;
  tiz_thread_t thread_;
  tiz_mutex_t mutex_;
  tiz_sem_t sem_;
  tiz_queue_t *p_queue_;
  tizplaylist_t playlist_;
  tizgraph_ptr_map_t graph_registry_;
  tizgraph_ptr_t running_graph_ptr_;
  error_callback_t error_cback_;

};

#endif // TIZGRAPHMGR_H

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
 * @file   tizgraphmgr.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph manager
 *
 *
 */

#ifndef TIZGRAPHMGR_HPP
#define TIZGRAPHMGR_HPP

#include <string>
#include <boost/function.hpp>

#include <tizplatform.h>
#include <OMX_Core.h>

#include "tizgraphtypes.hpp"
#include "tizgraphmgrfsm.hpp"

namespace tiz
{
  namespace graphmgr
  {

    // Forward declarations
    void *thread_func (void *p_arg);
    class cmd;

    /**
     *  @class mgr
     *  @brief The graph manager class.
     *
     *  A graph manager instantiates OpenMAX IL graphs according to the media
     *types
     *  of the elements found in a play list. The graph manager runs in its own
     *  thread and will manage the lifetime of the graphs that it creates.
     */
    class mgr
    {

      friend class tiz::graph::graph;
      friend void *thread_func (void *);

    public:
      typedef boost::function< void(OMX_ERRORTYPE, std::string) >
          error_callback_t;

    public:
      mgr ();
      virtual ~mgr ();

      /**
       * Initialise the graph manager thread.
       *
       * @pre This method must be called only once, before any call is made to
       *the
       * other APIs.
       *
       * @post The graph manager thread is ready to process requests.
       *
       * @return OMX_ErrorNone if initialisation was
       * successful. OMX_ErrorInsuficientResources otherwise.
       */
      OMX_ERRORTYPE init (const tizplaylist_ptr_t &playlist,
                          const error_callback_t &error_cback);

      /**
       * Destroy the manager thread and release all resources.
       *
       * @pre stop() has been called on this manager.
       *
       * @post Only init() can be called at this point.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      void deinit ();

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

    protected:
      virtual ops *do_init (const tizplaylist_ptr_t &playlist,
                            const error_callback_t &error_cback) = 0;

    protected:
      OMX_ERRORTYPE graph_loaded ();
      OMX_ERRORTYPE graph_execd ();
      OMX_ERRORTYPE graph_unloaded ();
      OMX_ERRORTYPE graph_end_of_play ();
      OMX_ERRORTYPE graph_error (const OMX_ERRORTYPE error,
                                 const std::string &msg);

    protected:
      ops *p_ops_;
      fsm fsm_;

    private:
      OMX_ERRORTYPE init_cmd_queue ();
      void deinit_cmd_queue ();
      OMX_ERRORTYPE post_cmd (cmd *p_cmd);
      static bool dispatch_cmd (mgr *p_mgr, const cmd *p_cmd);

    private:
      tiz_thread_t thread_;
      tiz_mutex_t mutex_;
      tiz_sem_t sem_;
      tiz_queue_t *p_queue_;
    };

    typedef boost::shared_ptr< mgr > mgr_ptr_t;

  }  // namespace graphmgr
}  // namespace tiz

#endif  // TIZGRAPHMGR_HPP

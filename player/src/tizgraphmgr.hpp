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
#include "tizplaybackstatus.hpp"
#include "mpris/tizmprismgr.hpp"
#include "tizgraphmgrfsm.hpp"

namespace tiz
{
  namespace graphmgr
  {

    // Forward declarations
    void *thread_func (void *p_arg);
    class cmd;
    class graphmgr_capabilities;

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

      friend class ops;
      friend class tiz::graph::graph;
      friend void *thread_func (void *);

    public:
      typedef boost::function< void(const OMX_ERRORTYPE error_code, const std::string error_msg) >
          termination_callback_t;

    public:
      mgr ();
      virtual ~mgr ();

      /**
       * Initialise the graph manager thread.
       *
       * @pre This method must be called only once, before any call is made to
       * the other APIs.
       *
       * @post The graph manager thread is ready to process requests.
       *
       * @return OMX_ErrorNone if initialisation was
       * successful. OMX_ErrorInsuficientResources otherwise.
       */
      OMX_ERRORTYPE init (const tizplaylist_ptr_t &playlist,
                          const termination_callback_t &termination_cback);

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
       * Process a specific item in the playlist.
       *
       * @pre init() has been called on this manager.
       *
       * @param idx The index of the item in the playlist to be processed next.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE position (const int position);

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
       * Increments or decrements the volume by steps.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE volume_step (const int step);

      /**
       * Changes the volume to the specified value. 1.0 is maximum volume and 0.0 means mute.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE volume (const double volume);

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
       * Halt processing of the playlist.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE stop ();

      /**
       * Exit the manager thread.
       *
       * @pre init() has been called on this manager.
       *
       * @return OMX_ErrorInsuficientResources if OOM. OMX_ErrorNone in case of
       * success.
       */
      OMX_ERRORTYPE quit ();

    protected:
      virtual ops *do_init (const tizplaylist_ptr_t &playlist,
                            const termination_callback_t &termination_cback,
                            graphmgr_capabilities &graphmgr_caps) = 0;

    protected:
      OMX_ERRORTYPE graph_loaded ();
      OMX_ERRORTYPE graph_execd ();
      OMX_ERRORTYPE graph_stopped ();
      OMX_ERRORTYPE graph_paused ();
      OMX_ERRORTYPE graph_resumed ();
      OMX_ERRORTYPE graph_metadata (const track_metadata_map_t &metadata);
      OMX_ERRORTYPE graph_volume (const int volume);
      OMX_ERRORTYPE graph_unloaded ();
      OMX_ERRORTYPE graph_end_of_play ();
      OMX_ERRORTYPE graph_error (const OMX_ERRORTYPE error,
                                 const std::string &msg);

      OMX_ERRORTYPE start_mpris (const graphmgr_capabilities &graphmgr_caps);
      OMX_ERRORTYPE stop_mpris ();
      OMX_ERRORTYPE do_update_control_ifcs (const control::playback_status_t status,
                                            const std::string &current_song = std::string ());
      OMX_ERRORTYPE do_update_metadata (const track_metadata_map_t &metadata);
      OMX_ERRORTYPE do_update_volume (const int volume);

    protected:
      ops *p_ops_;
      fsm fsm_;
      control::mprismgr_ptr_t mpris_ptr_;
      control::playback_events_t playback_events_;

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

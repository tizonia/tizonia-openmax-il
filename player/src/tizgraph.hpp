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
 * @file   tizgraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph base class
 *
 *
 */

#ifndef TIZGRAPH_HPP
#define TIZGRAPH_HPP

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include <OMX_Core.h>
#include <tizplatform.h>

#include "tizgraphtypes.hpp"
#include "tizplaylist.hpp"
#include "tizgraphcback.hpp"

namespace tiz
{
  namespace graphmgr
  {
    // Forward declaration
    class mgr;
  }

  namespace graph
  {

    // Forward declarations
    void *thread_func (void *p_arg);
    class cmd;
    class cbackhandler;
    class ops;
    class progress_display;

    class graph
    {

      friend class cbackhandler;
      friend class ops;
      friend void *thread_func (void *);

    public:
      explicit graph (const std::string &graph_name);
      virtual ~graph ();

      OMX_ERRORTYPE init ();
      OMX_ERRORTYPE load (const tizgraphconfig_ptr_t config
                          = tizgraphconfig_ptr_t ());
      OMX_ERRORTYPE execute (const tizgraphconfig_ptr_t config
                             = tizgraphconfig_ptr_t ());
      OMX_ERRORTYPE pause ();
      OMX_ERRORTYPE seek ();
      OMX_ERRORTYPE skip (const int jump);
      OMX_ERRORTYPE volume_step (const int step);
      OMX_ERRORTYPE volume (const double vol);
      OMX_ERRORTYPE mute ();
      OMX_ERRORTYPE stop ();
      void unload ();
      void deinit ();

      void omx_evt (const omx_event_info &evt);
      void set_manager (tiz::graphmgr::mgr *ap_mgr);

      static void timer_cback (void *ap_graph, tiz_event_timer_t *ap_ev_timer,
                               void *ap_arg1, const uint32_t a_id);

    protected:
      virtual ops *do_init () = 0;
      virtual bool dispatch_cmd (const tiz::graph::cmd *p_cmd) = 0;

    protected:
      void graph_loaded ();
      void graph_execd ();
      void graph_stopped ();
      void graph_paused ();
      void graph_resumed ();
      void graph_metadata (const track_metadata_map_t &metadata);
      void graph_volume (const int volume);
      void graph_unloaded ();
      void graph_end_of_play ();
      void graph_error (const OMX_ERRORTYPE error, const std::string &msg);

      void progress_display_start(unsigned long duration);
      void progress_display_increase();
      void progress_display_pause();
      void progress_display_resume();
      void progress_display_stop();

      std::string get_graph_name () const;

    protected:
      std::string graph_name_;
      cbackhandler cback_handler_;
      tiz::graphmgr::mgr *p_mgr_;
      ops *p_ops_;

    private:
      OMX_ERRORTYPE init_cmd_queue ();
      void deinit_cmd_queue ();
      OMX_ERRORTYPE post_cmd (tiz::graph::cmd *p_cmd);

    private:
      tiz_thread_t thread_;
      tiz_mutex_t mutex_;
      tiz_sem_t sem_;
      tiz_queue_t *p_queue_;
      tiz_event_timer *p_ev_timer_;
      progress_display *p_progress_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPH_HPP

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
 * @file   tizgraphmgr.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph manager base class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

#include <OMX_Component.h>

#include <tizplatform.h>
#include <tizmacros.h>

#include "tizgraphmgrcmd.hpp"
#include "tizgraph.hpp"
#include "tizomxutil.hpp"
#include "tizgraphutil.hpp"
#include "mpris/tizmprisprops.hpp"
#include "mpris/tizmpriscbacks.hpp"
#include "tizgraphmgrcaps.hpp"
#include "tizgraphmgr.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graphmgr"
#endif

#define TIZ_GRAPHMGR_QUEUE_MAX_ITEMS 30

namespace graphmgr = tiz::graphmgr;
namespace control = tiz::control;
namespace graph = tiz::graph;

void *graphmgr::thread_func (void *p_arg)
{
  mgr *p_mgr = static_cast< mgr * >(p_arg);
  void *p_data = NULL;
  bool done = false;

  assert (p_mgr);

  (void)tiz_thread_setname (&(p_mgr->thread_), (char *)"graphmgr");
  tiz_check_omx_ret_null (tiz_sem_post (&(p_mgr->sem_)));

  while (!done)
  {
    tiz_check_omx_ret_null (tiz_queue_receive (p_mgr->p_queue_, &p_data));

    assert (p_data);

    cmd *p_cmd = static_cast< cmd * >(p_data);
    done = mgr::dispatch_cmd (p_mgr, p_cmd);

    delete p_cmd;
  }

  tiz_check_omx_ret_null (tiz_sem_post (&(p_mgr->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Graph manager thread exiting...");

  return NULL;
}

//
// mgr
//
graphmgr::mgr::mgr ()
  : p_ops_ (NULL),
    fsm_ (boost::msm::back::states_ << graphmgr::fsm::starting (&p_ops_)
                                    << graphmgr::fsm::restarting (&p_ops_)
                                    << graphmgr::fsm::stopping (&p_ops_),
          &p_ops_),
    mpris_ptr_ (),
    playback_events_ (),
    thread_ (),
    mutex_ (),
    sem_ (),
    p_queue_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

graphmgr::mgr::~mgr ()
{
}

OMX_ERRORTYPE
graphmgr::mgr::init (const tizplaylist_ptr_t &playlist,
                     const termination_callback_t &termination_cback)
{
  // Init command queue infrastructure
  tiz_check_omx_ret_oom (init_cmd_queue ());

  // Create the manager's thread
  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (
      tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  graphmgr_capabilities_t graphmgr_caps;
  // Init this mgr's operations using the do_init template method
  tiz_check_null_ret_oom (
      (p_ops_ = do_init (playlist, termination_cback, graphmgr_caps)));

  // Let's wait until this manager's thread is ready to receive requests
  tiz_check_omx_ret_oom (tiz_sem_wait (&sem_));

  // Init the MPRIS interface and pass this manager's capabilities to it
  // (a.k.a. MPRIS properties).
  tiz_check_omx_ret_oom (start_mpris (graphmgr_caps));

  // Init OpenMAX IL
  tiz::omxutil::init ();

  // Init this manager's fsm
  fsm_.start ();

  return OMX_ErrorNone;
}

void graphmgr::mgr::deinit ()
{
  // Stop the MPRIS interface
  //
  // TODO: This is done too late here. Need do this right before the stop event
  // is processed by the thread.
  (void)stop_mpris ();

  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Waiting until stopped...");
  static_cast< void >(tiz_sem_wait (&sem_));
  void *p_result = NULL;
  static_cast< void >(tiz_thread_join (&thread_, &p_result));

  tiz::omxutil::deinit ();
  deinit_cmd_queue ();

  delete p_ops_;
  p_ops_ = NULL;
}

OMX_ERRORTYPE
graphmgr::mgr::start ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::start_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::next ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::next_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::prev ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::prev_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::position (const int pos)
{
  return post_cmd (new graphmgr::cmd (graphmgr::position_evt (pos)));
}

OMX_ERRORTYPE
graphmgr::mgr::fwd ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::fwd_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::rwd ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::rwd_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::volume_step (const int step)
{
  if (step == 0)
  {
    return OMX_ErrorNone;
  }

  if (step > 0)
  {
    return post_cmd (new graphmgr::cmd (graphmgr::vol_up_evt ()));
  }
  else
  {
    return post_cmd (new graphmgr::cmd (graphmgr::vol_down_evt ()));
  }
}

OMX_ERRORTYPE
graphmgr::mgr::volume (const double volume)
{
  return post_cmd (new graphmgr::cmd (graphmgr::vol_evt (volume)));
}

OMX_ERRORTYPE
graphmgr::mgr::mute ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::mute_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::pause ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::pause_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::stop ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::stop_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::quit ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::quit_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_loaded ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_loaded_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_execd ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_execd_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_stopped ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_stopped_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_paused ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_paused_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_resumed ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_resumed_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_metadata (const track_metadata_map_t &metadata)
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_metadata_evt (metadata)));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_volume (const int volume)
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_volume_evt (volume)));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_unloaded ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_unlded_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_end_of_play ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::graph_eop_evt ()));
}

OMX_ERRORTYPE
graphmgr::mgr::graph_error (const OMX_ERRORTYPE error, const std::string &msg)
{
  bool is_internal_error = false;
  return post_cmd (
      new graphmgr::cmd (graphmgr::err_evt (error, msg, is_internal_error)));
}

OMX_ERRORTYPE
graphmgr::mgr::start_mpris (const graphmgr_capabilities_t &graphmgr_caps)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (!mpris_ptr_ && graph::util::is_mpris_enabled ())
  {
    control::mpris_callbacks_t mpris_cbacks (
        boost::bind (&tiz::graphmgr::mgr::start, this),
        boost::bind (&tiz::graphmgr::mgr::next, this),
        boost::bind (&tiz::graphmgr::mgr::prev, this),
        boost::bind (&tiz::graphmgr::mgr::pause, this),
        boost::bind (&tiz::graphmgr::mgr::pause, this),
        boost::bind (&tiz::graphmgr::mgr::stop, this),
        boost::bind (&tiz::graphmgr::mgr::quit, this),
        boost::bind (&tiz::graphmgr::mgr::volume, this, _1));

    control::mpris_mediaplayer2_props_t props (
        graphmgr_caps.can_quit_, graphmgr_caps.can_raise_,
        graphmgr_caps.has_track_list_, graphmgr_caps.identity_,
        graphmgr_caps.uri_schemes_, graphmgr_caps.mime_types_);
    control::mpris_mediaplayer2_player_props_t player_props (
        "Stopped",                               // plaback status
        "Playlist",                              // loop status
        1.0,                                     // rate
        false,                                   // shuffle
        track_metadata_map_t(),                  // metadata
        .80,                                     // volume
        0,                                       // position
        graphmgr_caps.minimum_rate_, graphmgr_caps.maximum_rate_,
        graphmgr_caps.can_go_next_, graphmgr_caps.can_go_previous_,
        graphmgr_caps.can_play_, graphmgr_caps.can_pause_,
        graphmgr_caps.can_seek_, graphmgr_caps.can_control_);

    // NOTE: Can't use make_shared here because the playback events would be
    // passed as const & which means the slots will connected to a copy of our
    // signals, not to the original signals.
    mpris_ptr_
        = boost::shared_ptr< tiz::control::mprismgr >(new tiz::control::mprismgr (
            props, player_props, mpris_cbacks, playback_events_));
    tiz_check_null_ret_oom (mpris_ptr_);

    tiz_check_omx (mpris_ptr_->init ());
    tiz_check_omx (mpris_ptr_->start ());
  }
  return rc;
}

OMX_ERRORTYPE
graphmgr::mgr::stop_mpris ()
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  if (mpris_ptr_)
  {
    tiz_check_omx (mpris_ptr_->stop ());
    boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));
    mpris_ptr_->deinit ();
  }
  return rc;
}

OMX_ERRORTYPE
graphmgr::mgr::do_update_control_ifcs (const control::playback_status_t status,
                                       const std::string &current_song)
{
  playback_events_.playback_ (status);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graphmgr::mgr::do_update_metadata (const track_metadata_map_t &metadata)
{
  playback_events_.metadata_ (metadata);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graphmgr::mgr::do_update_volume (const int volume)
{
  playback_events_.volume_ (volume > 0 ? (double)volume / 100 : 0);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graphmgr::mgr::init_cmd_queue ()
{
  tiz_check_omx_ret_oom (tiz_mutex_init (&mutex_));
  tiz_check_omx_ret_oom (tiz_sem_init (&sem_, 0));
  tiz_check_omx_ret_oom (tiz_queue_init (&p_queue_, TIZ_GRAPHMGR_QUEUE_MAX_ITEMS));
  return OMX_ErrorNone;
}

void graphmgr::mgr::deinit_cmd_queue ()
{
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
}

OMX_ERRORTYPE
graphmgr::mgr::post_cmd (graphmgr::cmd *p_cmd)
{
  assert (p_cmd);
  assert (p_queue_);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  return OMX_ErrorNone;
}

bool graphmgr::mgr::dispatch_cmd (graphmgr::mgr *p_mgr,
                                  const graphmgr::cmd *p_cmd)
{
  assert (p_mgr);
  assert (p_mgr->p_ops_);
  assert (p_cmd);

  p_cmd->inject (p_mgr->fsm_);

  // Check for internal errors produced during the processing of the last
  // event. If any, inject an "internal" error event. This is fatal and shall
  // terminate the state machine.
  if (OMX_ErrorNone != p_mgr->p_ops_->internal_error ())
  {
    TIZ_LOG (TIZ_PRIORITY_ERROR,
             "MGR error detected. Injecting err_evt (this is fatal)");
    bool is_internal_error = true;
    p_mgr->fsm_.process_event (graphmgr::err_evt (
        p_mgr->p_ops_->internal_error (), p_mgr->p_ops_->internal_error_msg (),
        is_internal_error));
  }
  if (p_mgr->fsm_.terminated_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "MGR fsm terminated");
    p_mgr->p_ops_->deinit ();
  }

  return p_mgr->fsm_.terminated_;
}

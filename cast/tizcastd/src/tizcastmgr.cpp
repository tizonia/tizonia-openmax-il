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
 * @file   tizcastmgr.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL cast manager base class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <algorithm>
#include <boost/make_shared.hpp>

#include <OMX_Component.h>

#include <tizmacros.h>
#include <tizplatform.h>

#include "mpris/tizmpriscbacks.hpp"
#include "mpris/tizmprisprops.hpp"
#include "tizcast.hpp"
#include "tizcastmgr.hpp"
#include "tizcastmgrcaps.hpp"
#include "tizcastmgrcmd.hpp"
#include "tizcastutil.hpp"
#include "tizomxutil.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.castmgr"
#endif

#define TIZ_CASTMGR_QUEUE_MAX_ITEMS 30

namespace castmgr = tiz::castmgr;
namespace control = tiz::control;
namespace cast = tiz::cast;

void *castmgr::thread_func (void *p_arg)
{
  mgr *p_mgr = static_cast< mgr * > (p_arg);
  void *p_data = NULL;
  bool done = false;

  assert (p_mgr);

  (void)tiz_thread_setname (&(p_mgr->thread_), (char *)"castmgr");
  tiz_check_omx_ret_null (tiz_sem_post (&(p_mgr->sem_)));

  while (!done)
  {
    tiz_check_omx_ret_null (tiz_queue_receive (p_mgr->p_queue_, &p_data));

    assert (p_data);

    cmd *p_cmd = static_cast< cmd * > (p_data);
    done = mgr::dispatch_cmd (p_mgr, p_cmd);

    delete p_cmd;
  }

  tiz_check_omx_ret_null (tiz_sem_post (&(p_mgr->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Cast manager thread exiting...");

  return NULL;
}

//
// mgr
//
castmgr::mgr::mgr () : thread_ (), mutex_ (), sem_ (), p_queue_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

castmgr::mgr::~mgr ()
{
}

OMX_ERRORTYPE
castmgr::mgr::init (const tizplaylist_ptr_t &playlist,
                    const termination_callback_t &termination_cback)
{
  // Init command queue infrastructure
  tiz_check_omx_ret_oom (init_cmd_queue ());

  // Create the manager's thread
  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  castmgr_capabilities_t castmgr_caps;
  // Init this mgr's operations using the do_init template method
  tiz_check_null_ret_oom (
      (p_ops_ = do_init (playlist, termination_cback, castmgr_caps)));

  // Let's wait until this manager's thread is ready to receive requests
  tiz_check_omx_ret_oom (tiz_sem_wait (&sem_));

  return OMX_ErrorNone;
}

void castmgr::mgr::deinit ()
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Waiting until stopped...");
  static_cast< void > (tiz_sem_wait (&sem_));
  void *p_result = NULL;
  static_cast< void > (tiz_thread_join (&thread_, &p_result));
  deinit_cmd_queue ();
  delete p_ops_;
  p_ops_ = NULL;
}

OMX_ERRORTYPE
castmgr::mgr::start ()
{
  return post_cmd (new castmgr::cmd (castmgr::start_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::next ()
{
  return post_cmd (new castmgr::cmd (castmgr::next_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::prev ()
{
  return post_cmd (new castmgr::cmd (castmgr::prev_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::fwd ()
{
  return post_cmd (new castmgr::cmd (castmgr::fwd_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::rwd ()
{
  return post_cmd (new castmgr::cmd (castmgr::rwd_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::volume_step (const int step)
{
  if (step == 0)
  {
    return OMX_ErrorNone;
  }

  if (step > 0)
  {
    return post_cmd (new castmgr::cmd (castmgr::vol_up_evt ()));
  }
  else
  {
    return post_cmd (new castmgr::cmd (castmgr::vol_down_evt ()));
  }
}

OMX_ERRORTYPE
castmgr::mgr::volume (const double volume)
{
  return post_cmd (new castmgr::cmd (castmgr::vol_evt (volume)));
}

OMX_ERRORTYPE
castmgr::mgr::mute ()
{
  return post_cmd (new castmgr::cmd (castmgr::mute_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::pause ()
{
  return post_cmd (new castmgr::cmd (castmgr::pause_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::stop ()
{
  return post_cmd (new castmgr::cmd (castmgr::stop_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::quit ()
{
  return post_cmd (new castmgr::cmd (castmgr::quit_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::cast_loaded ()
{
  return post_cmd (new castmgr::cmd (castmgr::cast_loaded_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::cast_execd ()
{
  return post_cmd (new castmgr::cmd (castmgr::cast_execd_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::cast_stopped ()
{
  return post_cmd (new castmgr::cmd (castmgr::cast_stopped_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::cast_paused ()
{
  return post_cmd (new castmgr::cmd (castmgr::cast_paused_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::cast_unpaused ()
{
  return post_cmd (new castmgr::cmd (castmgr::cast_unpaused_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::cast_metadata (const track_metadata_map_t &metadata)
{
  return post_cmd (new castmgr::cmd (castmgr::cast_metadata_evt (metadata)));
}

OMX_ERRORTYPE
castmgr::mgr::cast_volume (const int volume)
{
  return post_cmd (new castmgr::cmd (castmgr::cast_volume_evt (volume)));
}

OMX_ERRORTYPE
castmgr::mgr::cast_unloaded ()
{
  return post_cmd (new castmgr::cmd (castmgr::cast_unlded_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::cast_end_of_play ()
{
  return post_cmd (new castmgr::cmd (castmgr::cast_eop_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::cast_error (const OMX_ERRORTYPE error, const std::string &msg)
{
  bool is_internal_error = false;
  return post_cmd (
      new castmgr::cmd (castmgr::err_evt (error, msg, is_internal_error)));
}

OMX_ERRORTYPE
castmgr::mgr::do_update_control_ifcs (const control::playback_status_t status,
                                      const std::string &current_song)
{
  playback_events_.playback_ (status);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
castmgr::mgr::do_update_metadata (const track_metadata_map_t &metadata)
{
  playback_events_.metadata_ (metadata);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
castmgr::mgr::do_update_volume (const int volume)
{
  playback_events_.volume_ (volume > 0 ? (double)volume / 100 : 0);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
castmgr::mgr::init_cmd_queue ()
{
  tiz_check_omx_ret_oom (tiz_mutex_init (&mutex_));
  tiz_check_omx_ret_oom (tiz_sem_init (&sem_, 0));
  tiz_check_omx_ret_oom (
      tiz_queue_init (&p_queue_, TIZ_CASTMGR_QUEUE_MAX_ITEMS));
  return OMX_ErrorNone;
}

void castmgr::mgr::deinit_cmd_queue ()
{
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
}

OMX_ERRORTYPE
castmgr::mgr::post_cmd (castmgr::cmd *p_cmd)
{
  assert (p_cmd);
  assert (p_queue_);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  return OMX_ErrorNone;
}

bool castmgr::mgr::dispatch_cmd (castmgr::mgr *p_mgr, const castmgr::cmd *p_cmd)
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
    p_mgr->fsm_.process_event (castmgr::err_evt (
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

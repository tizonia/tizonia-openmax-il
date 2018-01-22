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
 * @brief  Cast manager impl
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

#include "tizcastmgr.hpp"
#include "tizcastmgrcmd.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.mgr"
#endif

#define TIZ_CAST_MGR_QUEUE_MAX_ITEMS 30

namespace castmgr = tiz::castmgr;

void *castmgr::thread_func (void *p_arg)
{
  mgr *p_mgr = static_cast< mgr * > (p_arg);
  void *p_data = NULL;
  bool done = false;
  OMX_U32 poll_time = 0.2;  // ms

  assert (p_mgr);

  (void)tiz_thread_setname (&(p_mgr->thread_), (char *)"castmgr");
  tiz_check_omx_ret_null (tiz_sem_post (&(p_mgr->sem_)));

  while (!done)
  {
    tiz_check_omx_ret_null (
        tiz_queue_timed_receive (p_mgr->p_queue_, &p_data, poll_time));

    // Dispatch events from the command queue
    if (p_data)
    {
      cmd *p_cmd = static_cast< cmd * > (p_data);
      done = mgr::dispatch_cmd (p_mgr, p_cmd);
      delete p_cmd;
    }

    // Poll the chromecast socket
    if (!done)
    {
      cmd* p_poll_cmd = new castmgr::cmd (castmgr::poll_evt ());
      done = mgr::dispatch_cmd (p_mgr, p_poll_cmd);
      delete p_poll_cmd;
    }
  }

  tiz_check_omx_ret_null (tiz_sem_post (&(p_mgr->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Cast manager thread exiting...");

  return NULL;
}

//
// mgr
//
castmgr::mgr::mgr ()
  : p_ops_ (NULL),
    fsm_ (boost::msm::back::states_, &p_ops_),
    thread_ (),
    mutex_ (),
    sem_ (),
    p_queue_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

castmgr::mgr::~mgr ()
{
}

OMX_ERRORTYPE
castmgr::mgr::init ()
{
  // Init command queue infrastructure
  tiz_check_omx_ret_oom (init_cmd_queue ());

  // Create the manager's thread
  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  // Init this mgr's operations using the do_init template method
  tiz_check_null_ret_oom ((p_ops_ = do_init ()));

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
castmgr::mgr::quit ()
{
  return post_cmd (new castmgr::cmd (castmgr::quit_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::connect (const std::string &name_or_ip)
{
  return post_cmd (new castmgr::cmd (castmgr::connect_evt (name_or_ip)));
}

OMX_ERRORTYPE
castmgr::mgr::disconnect ()
{
  return post_cmd (new castmgr::cmd (castmgr::disconnect_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::load_url (const std::string &url,
                        const std::string &mime_type,
                        const std::string &title)
{
  return post_cmd (new castmgr::cmd (castmgr::load_url_evt (url, mime_type, title)));
}

OMX_ERRORTYPE
castmgr::mgr::play ()
{
  return post_cmd (new castmgr::cmd (castmgr::play_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::stop ()
{
  return post_cmd (new castmgr::cmd (castmgr::stop_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::pause ()
{
  return post_cmd (new castmgr::cmd (castmgr::pause_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::volume_up ()
{
  return post_cmd (new castmgr::cmd (castmgr::volume_up_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::volume_down ()
{
  return post_cmd (new castmgr::cmd (castmgr::volume_down_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::mute ()
{
  return post_cmd (new castmgr::cmd (castmgr::mute_evt ()));
}

OMX_ERRORTYPE
castmgr::mgr::unmute ()
{
  return post_cmd (new castmgr::cmd (castmgr::unmute_evt ()));
}

castmgr::ops *castmgr::mgr::do_init ()
{
  return new ops (this);
}

OMX_ERRORTYPE
castmgr::mgr::init_cmd_queue ()
{
  tiz_check_omx_ret_oom (tiz_mutex_init (&mutex_));
  tiz_check_omx_ret_oom (tiz_sem_init (&sem_, 0));
  tiz_check_omx_ret_oom (
      tiz_queue_init (&p_queue_, TIZ_CAST_MGR_QUEUE_MAX_ITEMS));
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

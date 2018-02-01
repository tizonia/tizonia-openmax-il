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
 * @file   tizcastworker.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Cast worker thread impl
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

#include "tizcastworker.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.worker"
#endif

#define TIZ_CAST_WORKER_QUEUE_MAX_ITEMS 30

namespace cast = tiz::cast;

void *cast::thread_func (void *p_arg)
{
  worker *p_worker = static_cast< worker * > (p_arg);
  void *p_data = NULL;
  bool done = false;
  int poll_time_ms = 100;  // ms
  // Pre-allocated poll command
  cmd *p_poll_cmd = new cast::cmd (cast::poll_evt (poll_time_ms));

  assert (p_worker);

  (void)tiz_thread_setname (&(p_worker->thread_), (char *)"cast");
  tiz_check_omx_ret_null (tiz_sem_post (&(p_worker->sem_)));

  while (!done)
  {
    tiz_queue_timed_receive (p_worker->p_queue_, &p_data, poll_time_ms);

    // Dispatch events from the command queue
    if (p_data)
    {
      cmd *p_cmd = static_cast< cmd * > (p_data);
      done = worker::dispatch_cmd (p_worker, p_cmd);
      delete p_cmd;
      p_data = NULL;
    }

    // This is to poll the chromecast socket periodically
    if (!done)
    {
      done = worker::dispatch_cmd (p_worker, p_poll_cmd);
    }
  }

  tiz_check_omx_ret_null (tiz_sem_post (&(p_worker->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Cast daemon worker thread exiting...");

  delete p_poll_cmd;

  return NULL;
}

//
// worker
//
cast::worker::worker (cast_status_cback_t cast_cb,
                      media_status_cback_t media_cb,
                      termination_callback_t termination_cb)
  : p_cc_ctx_(NULL),
    cast_cb_ (cast_cb),
    media_cb_ (media_cb),
    termination_cb_ (termination_cb),
    thread_ (),
    mutex_ (),
    sem_ (),
    p_queue_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
  int rc = tiz_chromecast_ctx_init (&(p_cc_ctx_));
  assert (0 == rc);
}

cast::worker::~worker ()
{
  deinit_cmd_queue ();
  BOOST_FOREACH (const devices_pair_t &device, devices_)
    {
      tiz::cast::mgr *p_cast_mgr = device.second.p_cast_mgr_;
      dispose_mgr (p_cast_mgr);
    }
  tiz_chromecast_ctx_destroy (&(p_cc_ctx_));
}

OMX_ERRORTYPE
cast::worker::init ()
{
  // Init command queue infrastructure
  tiz_check_omx_ret_oom (init_cmd_queue ());

  // Create the manager's thread
  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  // Init this worker's operations using the do_init template method
  tiz_check_null_ret_oom (
      (p_ops_ = new ops (
           this, boost::bind (&tiz::cast::worker::cast_status_received, this),
           cast_cb_, media_cb_, termination_cb_)));

  // Let's wait until this manager's thread is ready to receive requests
  tiz_check_omx_ret_oom (tiz_sem_wait (&sem_));

  // Get the fsm to start processing events
  return start_fsm ();
}

void cast::worker::deinit ()
{
  if (!fsm_.terminated_)
    {
      (void)stop_fsm ();
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "Waiting until stopped...");
      static_cast< void > (tiz_sem_wait (&sem_));
      void *p_result = NULL;
      static_cast< void > (tiz_thread_join (&thread_, &p_result));
    }
}

OMX_ERRORTYPE
cast::worker::connect (const std::vector< uint8_t > &uuid,
                       const std::string &name_or_ip)
{
  return post_cmd (new cast::cmd (cast::connect_evt (uuid, name_or_ip)));
}

OMX_ERRORTYPE
cast::worker::disconnect (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (cast::disconnect_evt (uuid)));
}

OMX_ERRORTYPE
cast::worker::load_url (const std::vector< uint8_t > &uuid,
                        const std::string &url, const std::string &mime_type,
                        const std::string &title, const std::string &album_art)
{
  return post_cmd (new cast::cmd (
      cast::load_url_evt (uuid, url, mime_type, title, album_art)));
}

OMX_ERRORTYPE
cast::worker::play (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (cast::play_evt (uuid)));
}

OMX_ERRORTYPE
cast::worker::stop (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (cast::stop_evt (uuid)));
}

OMX_ERRORTYPE
cast::worker::pause (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (cast::pause_evt (uuid)));
}

OMX_ERRORTYPE
cast::worker::volume_set (const std::vector< uint8_t > &uuid, int volume)
{
  return post_cmd (new cast::cmd (cast::volume_evt (uuid, volume)));
}

OMX_ERRORTYPE
cast::worker::volume_up (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (cast::volume_up_evt (uuid)));
}

OMX_ERRORTYPE
cast::worker::volume_down (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (cast::volume_down_evt (uuid)));
}

OMX_ERRORTYPE
cast::worker::mute (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (cast::mute_evt (uuid)));
}

OMX_ERRORTYPE
cast::worker::unmute (const std::vector< uint8_t > &uuid)
{
  return post_cmd (new cast::cmd (cast::unmute_evt (uuid)));
}

//
// Private methods
//

OMX_ERRORTYPE
cast::worker::start_fsm ()
{
  return post_cmd (new cast::cmd (cast::start_evt ()));
}

OMX_ERRORTYPE
cast::worker::stop_fsm ()
{
  return post_cmd (new cast::cmd (cast::quit_evt ()));
}

OMX_ERRORTYPE
cast::worker::cast_status_received ()
{
  return post_cmd (new cast::cmd (cast::cast_status_evt ()));
}

OMX_ERRORTYPE
cast::worker::init_cmd_queue ()
{
  tiz_check_omx_ret_oom (tiz_mutex_init (&mutex_));
  tiz_check_omx_ret_oom (tiz_sem_init (&sem_, 0));
  tiz_check_omx_ret_oom (
      tiz_queue_init (&p_queue_, TIZ_CAST_WORKER_QUEUE_MAX_ITEMS));
  return OMX_ErrorNone;
}

void cast::worker::deinit_cmd_queue ()
{
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
}

OMX_ERRORTYPE
cast::worker::post_cmd (cast::cmd *p_cmd)
{
  assert (p_cmd);
  assert (p_queue_);

  tiz_check_omx_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_ret_oom (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_ret_oom (tiz_mutex_unlock (&mutex_));

  return OMX_ErrorNone;
}

bool cast::worker::dispatch_cmd (cast::worker *p_worker, const cast::cmd *p_cmd)
{
  assert (p_worker);
  assert (p_worker->p_ops_);
  assert (p_cmd);

  p_cmd->inject (p_worker->fsm_);

  // Check for internal errors produced during the processing of the last
  // event. If any, inject an "internal" error event. This is fatal and shall
  // terminate the state machine.
  if (OMX_ErrorNone != p_worker->p_ops_->internal_error ())
  {
    TIZ_LOG (TIZ_PRIORITY_ERROR,
             "WORKER error detected. Injecting err_evt (this is fatal)");
    bool is_internal_error = true;
    p_worker->fsm_.process_event (cast::err_evt (
        p_worker->p_ops_->internal_error (), p_worker->p_ops_->internal_error_msg (),
        is_internal_error));
  }
  if (p_worker->fsm_.terminated_)
  {
    TIZ_LOG (TIZ_PRIORITY_NOTICE, "WORKER fsm terminated");
    p_worker->p_ops_->deinit ();
  }

  return p_worker->fsm_.terminated_;
}

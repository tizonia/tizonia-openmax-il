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
 * @file   tizmprismgr.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  MPRIS interface manager implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include <OMX_Component.h>

#include <tizplatform.h>
#include <tizmacros.h>

#include "tizmprismgrcmd.hpp"
#include "tizmprismgr.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.control.mprismgr"
#endif

namespace control = tiz::control;

void *control::thread_func (void *p_arg)
{
  mgr *p_mgr = static_cast< mgr * >(p_arg);
  void *p_data = NULL;
  bool done = false;

  assert (NULL != p_mgr);

  (void)tiz_thread_setname (&(p_mgr->thread_), (char *)"tizmprismgr");
  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_mgr->sem_)));

  while (!done)
  {
    tiz_check_omx_err_ret_null (tiz_queue_receive (p_mgr->p_queue_, &p_data));

    assert (NULL != p_data);

    cmd *p_cmd = static_cast< cmd * >(p_data);
    done = mgr::dispatch_cmd (p_mgr, p_cmd);

    delete p_cmd;
  }

  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_mgr->sem_)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "MPRIS interface thread exiting...");

  return NULL;
}

//
// mprismgr
//
control::mprismgr::mprismgr ()
    thread_ (),
    mutex_ (),
    sem_ (),
    p_queue_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

control::mprismgr::~mprismgr ()
{
}

OMX_ERRORTYPE
control::mprismgr::init (const tizplaylist_ptr_t &playlist,
                     const error_callback_t &error_cback)
{
  // Init command queue infrastructure
  tiz_check_omx_err_ret_oom (init_cmd_queue ());

  // Create the manager's thread
  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom (
      tiz_thread_create (&thread_, 0, 0, thread_func, this));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));

  // Let's wait until the manager's thread is ready to receive requests
  tiz_check_omx_err_ret_oom (tiz_sem_wait (&sem_));

  return OMX_ErrorNone;
}

void control::mprismgr::deinit ()
{
  TIZ_LOG (TIZ_PRIORITY_NOTICE, "Waiting until stopped...");
  static_cast< void >(tiz_sem_wait (&sem_));
  void *p_result = NULL;
  static_cast< void >(tiz_thread_join (&thread_, &p_result));
  deinit_cmd_queue ();
}

OMX_ERRORTYPE
control::mprismgr::start ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::pause_evt ()));
}

OMX_ERRORTYPE
control::mprismgr::stop ()
{
  return post_cmd (new graphmgr::cmd (graphmgr::start_evt ()));
}

OMX_ERRORTYPE
control::mprismgr::init_cmd_queue ()
{
  tiz_check_omx_err_ret_oom (tiz_mutex_init (&mutex_));
  tiz_check_omx_err_ret_oom (tiz_sem_init (&sem_, 0));
  tiz_check_omx_err_ret_oom (tiz_queue_init (&p_queue_, 10));
  return OMX_ErrorNone;
}

void control::mprismgr::deinit_cmd_queue ()
{
  tiz_mutex_destroy (&mutex_);
  tiz_sem_destroy (&sem_);
  tiz_queue_destroy (p_queue_);
}

OMX_ERRORTYPE
control::mprismgr::post_cmd (control::cmd *p_cmd)
{
  assert (NULL != p_cmd);
  assert (NULL != p_queue_);

  tiz_check_omx_err_ret_oom (tiz_mutex_lock (&mutex_));
  tiz_check_omx_err_ret_oom (tiz_queue_send (p_queue_, p_cmd));
  tiz_check_omx_err_ret_oom (tiz_mutex_unlock (&mutex_));

  return OMX_ErrorNone;
}

bool control::mprismgr::dispatch_cmd (control::mprismgr *p_mprismgr,
                                  const control::cmd *p_cmd)
{
  assert (NULL != p_mprismgr);
  assert (NULL != p_cmd);
  return true;
}

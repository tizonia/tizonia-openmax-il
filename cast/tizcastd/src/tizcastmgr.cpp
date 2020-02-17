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
 * @file   tizcastmgr.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Chromecast manager implementation.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <algorithm>
#include <boost/any.hpp>

#include <OMX_Component.h>

#include <tizmacros.h>
#include <tizplatform.h>

#include "tizcastmgr.hpp"
#include "tizcastmgrcmd.hpp"
#include "tizcastmgrops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.mgr"
#endif

namespace cast = tiz::cast;

//
// mgr
//
cast::mgr::mgr (const std::string &device_name_or_ip, const uuid_t &uuid,
                const tiz_chromecast_ctx_t *p_cc_ctx,
                cast_status_cback_t cast_cb, media_status_cback_t media_cb,
                error_status_callback_t error_cb)
  : p_ops_ (),
    fsm_ (boost::msm::back::states_, &p_ops_),
    name_or_ip_ (device_name_or_ip),
    uuid_ (uuid),
    p_cc_ctx_ (p_cc_ctx),
    cast_cb_ (cast_cb),
    media_cb_ (media_cb),
    error_cb_ (error_cb)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

cast::mgr::~mgr ()
{
  delete p_ops_;
  p_ops_ = NULL;
}

OMX_ERRORTYPE
cast::mgr::init ()
{
  // Init this mgr's operations using the do_init template method
  tiz_check_null_ret_oom (
      (p_ops_
       = new ops (this, p_cc_ctx_,
                  boost::bind (&tiz::cast::mgr::cast_status_received, this),
                  cast_cb_, media_cb_, error_cb_)));

  // Get the fsm to start processing events
  return start_fsm ();
}

void cast::mgr::deinit ()
{
  if (p_ops_ && !fsm_.terminated_)
  {
    (void)stop_fsm ();
  }
  delete p_ops_;
  p_ops_ = NULL;
}

bool cast::mgr::terminated () const
{
  return fsm_.terminated_;
}

cast::uuid_t cast::mgr::uuid () const
{
  return uuid_;
}

std::string cast::mgr::device_name_or_ip () const
{
  return name_or_ip_;
}

//
// Private methods
//

OMX_ERRORTYPE
cast::mgr::start_fsm ()
{
  return post_internal_cmd (cast::start_evt ());
}

OMX_ERRORTYPE
cast::mgr::stop_fsm ()
{
  return post_internal_cmd (cast::quit_evt ());
}

OMX_ERRORTYPE
cast::mgr::cast_status_received ()
{
  return post_internal_cmd (cast::cast_status_evt ());
}

OMX_ERRORTYPE
cast::mgr::post_internal_cmd (const boost::any &any_event)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  uuid_t null_uuid;
  bool done = false;
  cast::cmd cmd (null_uuid, any_event);
  done = dispatch_cmd (&cmd);
  if (done)
  {
    rc = OMX_ErrorInsufficientResources;
  }
  return rc;
}

bool cast::mgr::dispatch_cmd (const cast::cmd *p_cmd)
{
  assert (p_cmd);
  if (p_ops_)
  {
    p_cmd->inject (fsm_);

    // Check for internal errors produced during the processing of the last
    // event. If any, inject an "internal" error event. This is fatal and shall
    // terminate the state machine.
    if (OMX_ErrorNone != p_ops_->internal_error ())
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "MGR error detected. Injecting err_evt (this is fatal)");
      bool is_internal_error = true;
      fsm_.process_event (cast::err_evt (p_ops_->internal_error (),
                                         p_ops_->internal_error_msg (),
                                         is_internal_error));
    }

    if (fsm_.terminated_)
    {
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "MGR fsm terminated");
    }
  }
  return fsm_.terminated_;
}

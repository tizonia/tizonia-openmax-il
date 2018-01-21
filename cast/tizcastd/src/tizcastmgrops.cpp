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
 * @file   tizcastmgrops.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph manager operations
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/make_shared.hpp>

#include <tizmacros.h>
#include <tizplatform.h>

#include "tizcast.hpp"
#include "tizcastmgr.hpp"
#include "tizcastmgrops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.mgr.ops"
#endif

namespace castmgr = tiz::castmgr;

//
// ops
//
castmgr::ops::ops (mgr *p_mgr)
  : p_mgr_ (p_mgr), error_code_ (OMX_ErrorNone), error_msg_ (), p_cc_ (NULL)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

castmgr::ops::~ops ()
{
}

void castmgr::ops::deinit ()
{
  tiz_chromecast_destroy (p_cc_);
  p_cc_ = NULL;

  //   termination_cback_ (OMX_ErrorNone, "");
}

void castmgr::ops::do_connect ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  // Make sure a previous client has been disconnected
  disconnect ();
  if (0 != tiz_chromecast_init (&(p_cc_), name_or_ip.c_str(),
                                cc_new_media_status_cback,
                                this))
    {
      outcome = TIZ_CAST_MISUSE;
    }
  return outcome;
}

void castmgr::ops::do_disconnect ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (p_cc_)
    {
      tiz_chromecast_destroy (p_cc_);
      p_cc_ = NULL;
    }
  return outcome;
}

void castmgr::ops::do_load_url ()
{
    tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_load_url (p_cc_, url.c_str (), mime_type.c_str (),
                                    title.c_str ()))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While loading url : [%s]", url.c_str ());
      outcome = TIZ_CAST_URL_LOAD_FAILURE;
    }
  return outcome;
}

void castmgr::ops::do_play ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_play (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking play");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

void castmgr::ops::do_stop ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_stop (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking stop");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

void castmgr::ops::do_pause ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_pause (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking pause");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

void castmgr::ops::do_volume_up ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_volume_up (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking volume up");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

void castmgr::ops::do_volume_down ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_volume_down (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking volume down");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

void castmgr::ops::do_mute ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_mute (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking mute");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

void castmgr::ops::do_unmute ()
{
  tiz_cast_error_t outcome = TIZ_CAST_SUCCESS;
  if (0 != tiz_chromecast_unmute (p_cc_))
    {
      TIZ_LOG(TIZ_PRIORITY_ERROR, "While invoking unmute");
      outcome = TIZ_CAST_CC_CMD_FAILURE;
    }
  return outcome;
}

void castmgr::ops::do_report_fatal_error (const OMX_ERRORTYPE error,
                                          const std::string &msg)
{
  //   termination_cback_ (error, msg);
}

bool castmgr::ops::is_fatal_error (const OMX_ERRORTYPE error,
                                   const std::string &msg)
{
  TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (error),
           msg.c_str ());
  // This is a generic implementation. We use here some common understanding of
  // fatal errors. Each manager cast may decide to use its own list of fatal
  // errors.
  // return tiz::cast::util::is_fatal_error (error);
  return true;
}

OMX_ERRORTYPE
castmgr::ops::internal_error () const
{
  return error_code_;
}

std::string castmgr::ops::internal_error_msg () const
{
  return error_msg_;
}

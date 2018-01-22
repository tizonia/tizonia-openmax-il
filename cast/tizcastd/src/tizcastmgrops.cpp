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

#include "tizcastmgr.hpp"
#include "tizcastmgrops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.mgr.ops"
#endif

namespace castmgr = tiz::castmgr;

static void cc_new_media_status_cback (void *ap_user_data)
{
  // TODO
}

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

void castmgr::ops::do_connect (const std::string &name_or_ip)
{
  // Make sure a previous client has been disconnected
  // disconnect ();
  CAST_MGR_OPS_BAIL_IF_ERROR (
      tiz_chromecast_init (&(p_cc_), name_or_ip.c_str (),
                           cc_new_media_status_cback, this),
      "TODO: ADD ERROR MSG");
}

void castmgr::ops::do_disconnect ()
{
  if (p_cc_)
    {
      tiz_chromecast_destroy (p_cc_);
      p_cc_ = NULL;
    }
}

void castmgr::ops::do_load_url (const std::string &url,
                                const std::string &mime_type,
                                const std::string &title)
{
  CAST_MGR_OPS_BAIL_IF_ERROR (
      tiz_chromecast_load_url (p_cc_, url.c_str (), mime_type.c_str (),
                               title.c_str ()),
      "TODO: ADD ERROR MSG");
}

void castmgr::ops::do_play ()
{
  CAST_MGR_OPS_BAIL_IF_ERROR (tiz_chromecast_play (p_cc_),
                              "TODO: ADD ERROR MSG");
}

void castmgr::ops::do_stop ()
{
  CAST_MGR_OPS_BAIL_IF_ERROR (tiz_chromecast_stop (p_cc_),
                              "TODO: ADD ERROR MSG");
}

void castmgr::ops::do_pause ()
{
  CAST_MGR_OPS_BAIL_IF_ERROR (tiz_chromecast_pause (p_cc_),
                              "TODO: ADD ERROR MSG");
}

void castmgr::ops::do_volume_up ()
{
  CAST_MGR_OPS_BAIL_IF_ERROR (tiz_chromecast_volume_up (p_cc_),
                              "TODO: ADD ERROR MSG");
}

void castmgr::ops::do_volume_down ()
{
  CAST_MGR_OPS_BAIL_IF_ERROR (tiz_chromecast_volume_down (p_cc_),
                              "TODO: ADD ERROR MSG");
}

void castmgr::ops::do_mute ()
{
  CAST_MGR_OPS_BAIL_IF_ERROR (tiz_chromecast_mute (p_cc_),
                              "TODO: ADD ERROR MSG");
}

void castmgr::ops::do_unmute ()
{
  CAST_MGR_OPS_BAIL_IF_ERROR (tiz_chromecast_unmute (p_cc_),
                              "TODO: ADD ERROR MSG");
}

void castmgr::ops::do_report_fatal_error (const int error,
                                          const std::string &msg)
{
  //   termination_cback_ (error, msg);
}

bool castmgr::ops::is_fatal_error (const int error,
                                   const std::string &msg)
{
  TIZ_LOG (TIZ_PRIORITY_ERROR, "[%d] : %s", error,
           msg.c_str ());
  // This is a generic implementation. We use here some common understanding of
  // fatal errors. Each manager cast may decide to use its own list of fatal
  // errors.
  // return tiz::cast::util::is_fatal_error (error);
  return true;
}

int
castmgr::ops::internal_error () const
{
  return error_code_;
}

std::string castmgr::ops::internal_error_msg () const
{
  return error_msg_;
}

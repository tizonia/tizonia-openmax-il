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
  : p_mgr_ (p_mgr), error_code_ (OMX_ErrorNone), error_msg_ ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

castmgr::ops::~ops ()
{
}

void castmgr::ops::deinit ()
{
  //   termination_cback_ (OMX_ErrorNone, "");
}

void castmgr::ops::do_connect ()
{
}

void castmgr::ops::do_disconnect ()
{
}

void castmgr::ops::do_load_url ()
{
}

void castmgr::ops::do_play ()
{
}

void castmgr::ops::do_stop ()
{
}

void castmgr::ops::do_pause ()
{
}

void castmgr::ops::do_volume_up ()
{
}

void castmgr::ops::do_volume_down ()
{
}

void castmgr::ops::do_mute ()
{
}

void castmgr::ops::do_unmute ()
{
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

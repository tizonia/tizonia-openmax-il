/* -*-Mode: c++; -*- */
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
 * @file   tizdecgraphmgr.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Decode graph manager implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tizosal.h>

#include "tizdecgraphmgr.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.decodemgr"
#endif

namespace graphmgr = tiz::graphmgr;

//
// mgr
//
graphmgr::decodemgr::decodemgr () : graphmgr::mgr ()
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Constructing...");
}

graphmgr::decodemgr::~decodemgr ()
{
}

graphmgr::ops *graphmgr::decodemgr::do_init (
    const tizplaylist_ptr_t &playlist, const error_callback_t &error_cback)
{
  return new decodemgrops (this, playlist, error_cback);
}

//
// decodemgrops
//
graphmgr::decodemgrops::decodemgrops (mgr *p_mgr,
                                      const tizplaylist_ptr_t &playlist,
                                      const error_callback_t &error_cback)
  : tiz::graphmgr::ops (p_mgr, playlist, error_cback)
{
}

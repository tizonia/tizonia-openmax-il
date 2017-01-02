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
 * @file   tizdecgraphmgr.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Decode graph manager implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/assign/list_of.hpp> // for 'list_of()'

#include <tizplatform.h>

#include "tizgraphmgrcaps.hpp"
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
    const tizplaylist_ptr_t &playlist, const termination_callback_t &termination_cback,
    graphmgr_capabilities_t &graphmgr_caps)
{
  // Fill this graph manager capabilities
  graphmgr_caps.can_quit_ = false;
  graphmgr_caps.can_raise_ = false;
  graphmgr_caps.has_track_list_ = true;
  graphmgr_caps.identity_.assign ("Tizonia version ");
  graphmgr_caps.identity_.append (PACKAGE_VERSION);
  graphmgr_caps.uri_schemes_ = boost::assign::list_of("file");
  graphmgr_caps.mime_types_ = boost::assign::list_of
    ("audio/mpg")("audio/mp3")("audio/aac")("audio/aacp")("audio/vorbis")("audio/opus")("audio/flac");
  graphmgr_caps.minimum_rate_ = 1.0;
  graphmgr_caps.maximum_rate_ = 1.0;
  graphmgr_caps.can_go_next_ = true;
  graphmgr_caps.can_go_previous_ = true;
  graphmgr_caps.can_play_ = true;
  graphmgr_caps.can_pause_ = true;
  graphmgr_caps.can_seek_ = false;
  graphmgr_caps.can_control_ = false;

  return new decodemgrops (this, playlist, termination_cback);
}

//
// decodemgrops
//
graphmgr::decodemgrops::decodemgrops (mgr *p_mgr,
                                      const tizplaylist_ptr_t &playlist,
                                      const termination_callback_t &termination_cback)
  : tiz::graphmgr::ops (p_mgr, playlist, termination_cback)
{
}

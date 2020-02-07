/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphcmd.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph command class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "tizgraphcmd.hpp"

namespace graph = tiz::graph;

graph::cmd::cmd (boost::any any_event, bool kill_thread /* = false */)
  : evt_ (any_event), kill_thread_ (kill_thread), cmd_name_("CMD [")
{
  cmd_name_.append (typeid(evt ()).name ());
  cmd_name_.append ("]");
}

const boost::any graph::cmd::evt () const
{
  return evt_;
}

/*@observer@*/ const char* graph::cmd::c_str () const
{
  return cmd_name_.c_str ();
}

bool graph::cmd::kill_thread () const
{
  return kill_thread_;
}

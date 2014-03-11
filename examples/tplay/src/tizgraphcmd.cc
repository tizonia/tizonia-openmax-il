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
 * @file   tizgraphcmd.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph command class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <string>

#include "tizgraphevt.h"
#include "tizgraphcmd.h"

namespace
{
  template <typename T>
  bool is_type (const boost::any& operand)
  {
    return operand.type () == typeid(T);
  }
}

namespace graph = tiz::graph;

graph::cmd::cmd (boost::any any_event, bool kill_thread /* = false */)
  : evt_ (any_event), kill_thread_ (kill_thread)
{
}

const boost::any graph::cmd::evt () const
{
  return evt_;
}

/*@observer@*/ const char* graph::cmd::c_str () const
{
  std::string cmd_name ("CMD [");
  cmd_name.append (typeid(evt ()).name ());
  cmd_name.append ("]");
  return cmd_name.c_str ();
}

bool graph::cmd::kill_thread () const
{
  return kill_thread_;
}

void graph::cmd::inject (fsm& machine) const
{
#define INJECT_EVENT(the_evt)                               \
  if (is_type<the_evt>(evt_))                               \
  {                                                         \
    std::string arg (#the_evt);                             \
    TIZ_LOG (TIZ_PRIORITY_NOTICE,                           \
             "GRAPH : Injecting "                           \
             "CMD [%s] in STATE [%s]...",                   \
             arg.c_str (), tiz::graph::pstate (machine));   \
    machine.process_event (boost::any_cast<the_evt>(evt_)); \
  }

  INJECT_EVENT (load_evt)
  else INJECT_EVENT (execute_evt)
  else INJECT_EVENT (configured_evt)
  else INJECT_EVENT (omx_trans_evt)
  else INJECT_EVENT (skip_evt)
  else INJECT_EVENT (skipped_evt)
  else INJECT_EVENT (seek_evt)
  else INJECT_EVENT (volume_evt)
  else INJECT_EVENT (mute_evt)
  else INJECT_EVENT (pause_evt)
  else INJECT_EVENT (omx_evt)
  else INJECT_EVENT (omx_eos_evt)
  else INJECT_EVENT (unload_evt)
  else INJECT_EVENT (omx_port_disabled_evt)
  else INJECT_EVENT (omx_port_enabled_evt)
  else INJECT_EVENT (omx_port_settings_evt)
  else INJECT_EVENT (omx_err_evt)
  else INJECT_EVENT (err_evt)
  else
  {
    assert (0);
  }
}

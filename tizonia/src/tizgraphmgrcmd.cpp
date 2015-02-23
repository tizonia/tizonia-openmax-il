/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphmgrcmd.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph manager command class
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <string>

#include "tizgraphmgrcmd.hpp"

namespace
{
  template < typename T >
  bool is_type (const boost::any& operand)
  {
    return operand.type () == typeid(T);
  }
}

namespace graphmgr = tiz::graphmgr;

graphmgr::cmd::cmd (boost::any any_event) : evt_ (any_event)
{
}

const boost::any graphmgr::cmd::evt () const
{
  return evt_;
}

/*@observer@*/ const char* graphmgr::cmd::c_str () const
{
  if (is_type< start_evt >(evt_))
  {
    return "start_evt";
  }
  return "Unknown Graph Manager command";
}

void graphmgr::cmd::inject (fsm& machine) const
{
#define INJECT_EVENT(the_evt)                                 \
  if (is_type< the_evt >(evt_))                               \
  {                                                           \
    std::string arg (#the_evt);                               \
    TIZ_LOG (TIZ_PRIORITY_NOTICE,                             \
             "GRAPH MGR : Injecting "                         \
             "CMD [%s] in STATE [%s]...",                     \
             arg.c_str (), tiz::graphmgr::pstate (machine));  \
    machine.process_event (boost::any_cast< the_evt >(evt_)); \
  }

  INJECT_EVENT (start_evt)
  else INJECT_EVENT (next_evt)
    else INJECT_EVENT (prev_evt)
      else INJECT_EVENT (fwd_evt)
        else INJECT_EVENT (rwd_evt)
          else INJECT_EVENT (vol_up_evt)
            else INJECT_EVENT (vol_down_evt)
              else INJECT_EVENT (mute_evt)
                else INJECT_EVENT (pause_evt)
                  else INJECT_EVENT (stop_evt)
                    else INJECT_EVENT (quit_evt)
                      else INJECT_EVENT (graph_eop_evt)
                        else INJECT_EVENT (err_evt)
                          else INJECT_EVENT (graph_loaded_evt)
                            else INJECT_EVENT (graph_execd_evt)
                              else INJECT_EVENT (graph_stopped_evt)
                               else INJECT_EVENT (graph_paused_evt)
                                else INJECT_EVENT (graph_unpaused_evt)
                                else INJECT_EVENT (graph_metadata_evt)
                                 else INJECT_EVENT (graph_unlded_evt)
                                  else
                                    {
                                      assert (0);
                                    }
}

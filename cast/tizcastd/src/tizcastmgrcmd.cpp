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
 * @file   tizcastmgrcmd.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Cast manager command class
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <string>

#include "tizcastmgrcmd.hpp"

namespace
{
  template < typename T >
  bool is_type (const boost::any& operand)
  {
    return operand.type () == typeid(T);
  }
}

namespace castmgr = tiz::castmgr;

castmgr::cmd::cmd (boost::any any_event) : evt_ (any_event)
{
}

const boost::any castmgr::cmd::evt () const
{
  return evt_;
}

/*@observer@*/ const char* castmgr::cmd::c_str () const
{
  if (is_type< start_evt >(evt_))
  {
    return "start_evt";
  }
  return "Unknown Cast Manager command";
}

void castmgr::cmd::inject (fsm& machine) const
{
#define INJECT_EVENT(the_evt)                                   \
  if (is_type< the_evt >(evt_))                                 \
    {                                                           \
      std::string arg (#the_evt);                               \
      TIZ_LOG (TIZ_PRIORITY_NOTICE,                             \
               "CAST MGR : Injecting "                          \
               "CMD [%s] in STATE [%s]...",                     \
               arg.c_str (), tiz::castmgr::pstate (machine));   \
      machine.process_event (boost::any_cast< the_evt >(evt_)); \
    }

  INJECT_EVENT (start_evt)
  else INJECT_EVENT (quit_evt)
    else INJECT_EVENT (connect_evt)
      else INJECT_EVENT (disconnect_evt)
        else INJECT_EVENT (load_url_evt)
          else INJECT_EVENT (play_evt)
            else INJECT_EVENT (stop_evt)
              else INJECT_EVENT (pause_evt)
                else INJECT_EVENT (volume_evt)
                  else INJECT_EVENT (volume_up_evt)
                    else INJECT_EVENT (volume_down_evt)
                      else INJECT_EVENT (mute_evt)
                        else INJECT_EVENT (unmute_evt)
                          else INJECT_EVENT (poll_evt)
                            else INJECT_EVENT (err_evt)
                              else
                                {
                                  assert (0);
                                }
}

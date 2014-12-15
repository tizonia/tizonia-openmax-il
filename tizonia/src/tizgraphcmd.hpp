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
 * @file   tizgraphcmd.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph command class.
 *
 *
 */

#ifndef TIZGRAPHCMD_HPP
#define TIZGRAPHCMD_HPP

#include <boost/any.hpp>
#include <boost/function.hpp>

#include <tizplatform.h>

#include "tizgraphevt.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.cmd"
#endif

namespace tiz
{
  namespace graph
  {

    class cmd
    {

    private:
      template < typename T >
      bool is_type (const boost::any& operand) const
      {
        return operand.type () == typeid(T);
      }

    public:
      explicit cmd (boost::any any_event, bool kill_thread = false);

    public:
      const boost::any evt () const;
      const char* c_str () const;
      bool kill_thread () const;
      template < typename Fsm >
      void inject (
          Fsm& machine,
          boost::function< char const* const(Fsm const& machine) > pstate) const
      {
#define INJECT_EVENT(the_evt)                                           \
        if (is_type< the_evt >(evt_))                                   \
          {                                                             \
            std::string arg (#the_evt);                                 \
            TIZ_LOG (TIZ_PRIORITY_NOTICE,                               \
                     "GRAPH : Injecting "                               \
                     "CMD [%s] in STATE [%s]...",                       \
                     arg.c_str (), pstate (machine));                   \
            machine.process_event (boost::any_cast< the_evt >(evt_));   \
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
                              else INJECT_EVENT (stop_evt)
                                else INJECT_EVENT (unload_evt)
                                  else INJECT_EVENT (omx_port_disabled_evt)
                                    else INJECT_EVENT (omx_port_enabled_evt)
                                      else INJECT_EVENT (omx_port_settings_evt)
                                        else INJECT_EVENT (omx_format_detected_evt)
                                          else INJECT_EVENT (omx_err_evt)
                                            else INJECT_EVENT (err_evt)
                                              else INJECT_EVENT (auto_detected_evt)
                                                else INJECT_EVENT (graph_updated_evt)
                                                  else INJECT_EVENT (graph_reconfigured_evt)
                                                    else
        {
          assert (0);
        }
      }

    private:
      const boost::any evt_;
      const bool kill_thread_;
    };

  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHCMD_HPP

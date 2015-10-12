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
 * @file   tizgraphstate.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph fsm states
 *
 */

#ifndef TIZGRAPHSTATE_HPP
#define TIZGRAPHSTATE_HPP

#include <boost/msm/front/states.hpp>

#include <tizplatform.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.state"
#endif

#define G_STATE_LOG()                                                  \
  do                                                                   \
  {                                                                    \
    TIZ_LOG (TIZ_PRIORITY_TRACE, "STATE [%s]", typeid(*this).name ()); \
  } while (0)

namespace tiz
{
  namespace graph
  {

    struct inited : public boost::msm::front::state<>
    {
      typedef boost::mpl::vector<execute_evt> deferred_events;
      // optional entry/exit methods
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm)
      {
        G_STATE_LOG ();
        fsm.terminated_ = false;
      }
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
    };

    struct loaded : public boost::msm::front::state<>
    {
      // optional entry/exit methods
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
    };

    struct configured : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
    };

    struct config2idle : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StateIdle;
      }
    };

    struct idle2exe : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StateExecuting;
      }
    };

    struct executing : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StateExecuting;
      }
    };

    struct exe2pause : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StatePause;
      }
    };

    struct pause : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StatePause;
      }
    };

    struct pause2exe : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StateExecuting;
      }
    };

    struct pause2idle : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StateIdle;
      }
    };

    struct exe2idle : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StateIdle;
      }
    };

    struct idle : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StateIdle;
      }
    };

    struct idle2loaded : public boost::msm::front::state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
      OMX_STATETYPE target_omx_state () const
      {
        return OMX_StateLoaded;
      }
    };

    struct disabling_ports : public boost::msm::front::state<>
    {
      template <class Event,class FSM>
      void on_entry(Event const & evt, FSM & fsm)
      {
        G_STATE_LOG();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            (*(fsm.pp_ops_))->do_disable_ports ();
          }
      }
      template <class Event,class FSM>
      void on_exit(Event const & evt, FSM & fsm) {G_STATE_LOG();}
    };

    struct disabling_tunnel : public boost::msm::front::state<>
    {
      template <class Event,class FSM>
      void on_entry(Event const & evt, FSM & fsm) {G_STATE_LOG();}
      template <class Event,class FSM>
      void on_exit(Event const & evt, FSM & fsm) {G_STATE_LOG();}
    };

    struct enabling_tunnel : public boost::msm::front::state<>
    {
      template <class Event,class FSM>
      void on_entry(Event const & evt, FSM & fsm) {G_STATE_LOG();}
      template <class Event,class FSM>
      void on_exit(Event const & evt, FSM & fsm) {G_STATE_LOG();}
    };

    struct awaiting_port_disabled_evt : public boost::msm::front::state<>
    {
      template <class Event,class FSM>
      void on_entry(Event const & evt, FSM & fsm) {G_STATE_LOG();}
      template <class Event,class FSM>
      void on_exit(Event const & evt, FSM & fsm) {G_STATE_LOG();}
    };

    struct awaiting_port_settings_evt : public boost::msm::front::state<>
    {
      template <class Event,class FSM>
      void on_entry(Event const & evt, FSM & fsm) {G_STATE_LOG();}
      template <class Event,class FSM>
      void on_exit(Event const & evt, FSM & fsm) {G_STATE_LOG();}
    };

    struct awaiting_format_detected_evt : public boost::msm::front::state<>
    {
      template <class Event,class FSM>
      void on_entry(Event const & evt, FSM & fsm) {G_STATE_LOG();}
      template <class Event,class FSM>
      void on_exit(Event const & evt, FSM & fsm) {G_STATE_LOG();}
    };

    // terminate state
    struct unloaded : public boost::msm::front::terminate_state<>
    {
      template < class Event, class FSM >
      void on_entry (Event const &evt, FSM &fsm)
      {
        TIZ_LOG (TIZ_PRIORITY_TRACE, "ack unload");
        if (!fsm.terminated_)
        {
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            (*(fsm.pp_ops_))->do_ack_unloaded ();
          }
          TIZ_LOG (TIZ_PRIORITY_TRACE, "terminating");
          fsm.terminated_ = true;
        }
      }
      template < class Event, class FSM >
      void on_exit (Event const &evt, FSM &fsm) {G_STATE_LOG ();}
    };

    struct AllOk : public boost::msm::front::state<>
    {
      template <class Event,class FSM>
      void on_entry(Event const&,FSM& ) {G_STATE_LOG ();}
      template <class Event,class FSM>
      void on_exit(Event const&,FSM& ) {G_STATE_LOG ();}
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHSTATE_HPP

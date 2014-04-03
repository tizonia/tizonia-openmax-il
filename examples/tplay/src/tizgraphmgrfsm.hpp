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
 * @file   tizgraphmgrfsm.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph mgr fsm class
 *
 *
 */

#ifndef TIZGRAPHMGRFSM_H
#define TIZGRAPHMGRFSM_H

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 30
#define FUSION_MAX_VECTOR_SIZE      20
#define SPIRIT_ARGUMENTS_LIMIT      20

#include <sys/time.h>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/back/mpl_graph_fsm_check.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/back/tools.hpp>

#include <tizosal.h>

#include "tizgraphmgrops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graphmgr.fsm"
#endif

#define GMGR_FSM_LOG()                                                  \
  do                                                                    \
    {                                                                   \
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid(*this).name ());      \
    }                                                                   \
  while(0)

namespace tiz
{
  namespace graphmgr
  {
    static char const* const state_names[] = { "inited",
                                               "starting",
                                               "running",
                                               "restarting",
                                               "stopping",
                                               "stopped"};

    // main fsm events
    struct start_evt {};
    struct next_evt {};
    struct prev_evt {};
    struct fwd_evt {};
    struct rwd_evt {};
    struct vol_up_evt {};
    struct vol_down_evt {};
    struct mute_evt {};
    struct pause_evt {};
    struct stop_evt {};
    struct graph_eop_evt {};
    struct err_evt
    {
      err_evt(const OMX_ERRORTYPE error, const std::string & error_str, bool is_internal)
        :
        error_code_ (error),
        error_str_(error_str),
        is_internal_ (is_internal)
      {}
      OMX_ERRORTYPE error_code_;
      std::string   error_str_;
      bool is_internal_;
    };

    // 'starting/restarting' submachine events
    struct graph_loaded_evt {};
    struct graph_execd_evt {};
    struct graph_unlded_evt {};

    // Concrete FSM implementation
    struct fsm_ : public boost::msm::front::state_machine_def<fsm_>
    {
      // no need for exception handling
      typedef int no_exception_thrown;

      /* Forward declarations */
      struct running;
      struct stopped;
      struct loading_graph;
      struct executing_graph;
      struct unloading_graph;
      struct do_report_fatal_error;

      // data members
      ops ** pp_ops_;
      bool terminated_;

      fsm_(ops **pp_ops)
        :
        pp_ops_(pp_ops),
        terminated_ (false)
      {}

      // states
      struct inited : public boost::msm::front::state<>
      {
        // optional entry/exit methods
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) { GMGR_FSM_LOG (); }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) { GMGR_FSM_LOG (); }
      };

      /* 'starting' is a submachine */
      struct starting_ : public boost::msm::front::state_machine_def<starting_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;

        // data members
        ops ** pp_ops_;

        starting_()
          :
          pp_ops_(NULL)
        {}
        starting_(ops **pp_ops)
          :
          pp_ops_(pp_ops)
        {
          assert (NULL != pp_ops);
        }

        // submachine states
        struct loading_graph : public boost::msm::front::state<>
        {
          typedef boost::mpl::vector<next_evt, prev_evt, fwd_evt, rwd_evt, vol_up_evt, vol_down_evt, mute_evt, pause_evt, stop_evt> deferred_events;
          template <class Event,class FSM>
          void on_entry(Event const&, FSM& fsm) {GMGR_FSM_LOG ();}
        };

        struct executing_graph : public boost::msm::front::state<>
        {
          typedef boost::mpl::vector<next_evt, prev_evt, fwd_evt, rwd_evt, vol_up_evt, vol_down_evt, mute_evt, pause_evt, stop_evt> deferred_events;
          template <class Event,class FSM>
          void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
        };

        struct starting_exit : public boost::msm::front::exit_pseudo_state<graph_execd_evt>
        {
          typedef boost::mpl::vector<next_evt, prev_evt, fwd_evt, rwd_evt, vol_up_evt, vol_down_evt, mute_evt, pause_evt, stop_evt> deferred_events;
          template <class Event,class FSM>
          void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
        };

        // the initial state. Must be defined
        typedef loading_graph initial_state;

        // transition actions
        struct do_execute_graph
        {
          template <class FSM,class EVT,class SourceState,class TargetState>
          void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
          {
            GMGR_FSM_LOG ();
            /* STEP: issue 'execute' graph command */
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                (*(fsm.pp_ops_))->do_execute ();
              }
          }
        };

        // guard conditions

        // Transition table for starting
        struct transition_table : boost::mpl::vector<
          //                       Start              Event               Next                Action             Guard
          //    +-----------------+------------------+-------------------+-------------------+-----------------------+
          boost::msm::front::Row < loading_graph     , graph_loaded_evt  , executing_graph   , do_execute_graph      >,
          boost::msm::front::Row < executing_graph   , graph_execd_evt   , starting_exit                      >
          //    +-----------------+------------------+-------------------+-------------------+-----------------------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }

      };
      // typedef boost::msm::back::state_machine<starting_, boost::msm::back::mpl_graph_fsm_check> starting;
      typedef boost::msm::back::state_machine<starting_> starting;

      struct running : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      /* restarting is a submachine */
      struct restarting_ : public boost::msm::front::state_machine_def<restarting_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;

        // data members
        ops ** pp_ops_;

        restarting_()
          :
          pp_ops_(NULL)
        {}

        restarting_(ops **pp_ops)
          :
          pp_ops_(pp_ops)
        {
          assert (NULL != pp_ops);
        }

        struct restarting_exit : public boost::msm::front::exit_pseudo_state<graph_unlded_evt>
        {
          typedef boost::mpl::vector<next_evt, prev_evt, fwd_evt, rwd_evt, vol_up_evt, vol_down_evt, mute_evt, pause_evt, stop_evt> deferred_events;
          template <class Event,class FSM>
          void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
        };

        // the initial state. Must be defined
        typedef unloading_graph initial_state;

        // transition actions

        // guard conditions

        // Transition table for restarting
        struct transition_table : boost::mpl::vector<
          //                       Start            Event                Next              Action                   Guard
          //    +-----------------+----------------+--------------------+------------------+------------------------+--------+
          boost::msm::front::Row < unloading_graph , graph_unlded_evt   , restarting_exit                                    >
          //    +-----------------+----------------+--------------------+------------------+------------------------+--------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }
      };
      // typedef boost::msm::back::state_machine<restarting_, boost::msm::back::mpl_graph_fsm_check> restarting;
      typedef boost::msm::back::state_machine<restarting_> restarting;

      /* stopping is a submachine */
      struct stopping_ : public boost::msm::front::state_machine_def<stopping_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;

        // data members
        ops ** pp_ops_;

        stopping_ ()
          :
          pp_ops_(NULL)
        {}

        stopping_ (ops **pp_ops)
          :
          pp_ops_(pp_ops)
        {
          assert (NULL != pp_ops);
        }

        // The list of FSM states
        struct stopping_exit : public boost::msm::front::exit_pseudo_state<graph_unlded_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
        };

        // the initial state. Must be defined
        typedef unloading_graph initial_state;

        // transition actions

        // guard conditions

        // Transition table for stopping
        struct transition_table : boost::mpl::vector<
          //                       Start            Event                Next           Action                   Guard
          //    +-----------------+----------------+-------=------------+---------------+------------------------+--------+
          boost::msm::front::Row < unloading_graph , graph_unlded_evt   , stopping_exit                                   >
          //    +-----------------+----------------+--------------------+---------------+------------------------+--------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }
      };
      // typedef boost::msm::back::state_machine<stopping_, boost::msm::back::mpl_graph_fsm_check> stopping;
      typedef boost::msm::back::state_machine<stopping_> stopping;

      // terminate state
      struct stopped : public boost::msm::front::terminate_state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
          GMGR_FSM_LOG ();
          fsm.terminated_ = true;
        }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      struct unloading_graph : public boost::msm::front::state<>
      {
        typedef boost::mpl::vector<next_evt, prev_evt, fwd_evt, rwd_evt, vol_up_evt, vol_down_evt, mute_evt, pause_evt, stop_evt> deferred_events;
        template <class Event,class FSM>
        void on_entry(Event const&, FSM& fsm) {GMGR_FSM_LOG ();}
      };

      // The initial state of the SM. Must be defined
      typedef inited initial_state;

      // transition actions
      struct do_load
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_load ();
            }
        }
      };

      struct do_next
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_next ();
            }
        }
      };

      struct do_prev
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_prev ();
            }
        }
      };

      struct do_fwd
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_fwd ();
            }
        }
      };

      struct do_rwd
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_rwd ();
            }
        }
      };

      struct do_vol_up
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_vol_up ();
            }
        }
      };

      struct do_vol_down
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_vol_down ();
            }
        }
      };

      struct do_mute
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm, SourceState& , TargetState&)
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_mute ();
            }
        }
      };

      struct do_pause
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_pause ();
            }
        }
      };

      struct do_unload
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_unload ();
            }
        }
      };

      struct do_report_fatal_error
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_report_fatal_error (evt.error_code_, evt.error_str_);
            }
        }
      };

      // guard conditions
      struct is_fatal_error
      {
        template <class EVT,class FSM,class SourceState,class TargetState>
        bool operator()(EVT const& evt ,FSM& fsm, SourceState& , TargetState& )
        {
          bool rc = false;
          if (evt.is_internal_)
            {
              // Graph manager internal errors are always treated as fatal and
              // the application will be terminated.
              rc = true;
            }
          else
            {
              if (fsm.pp_ops_ && *(fsm.pp_ops_))
                {
                  rc = (*(fsm.pp_ops_))->is_fatal_error (evt.error_code_, evt.error_str_);
                }
            }

          TIZ_LOG (TIZ_PRIORITY_ERROR, "is_fatal_error [%s]", rc ? "YES" : "NO");
          return rc;
        }
      };

      // Transition table for the graph mgr fsm
      struct transition_table : boost::mpl::vector<
        //                       Start                   Event                     Next                      Action                   Guard
        //    +------------------------------------------+-------------------------+-------------------------+------------------------+--------------------+
        boost::msm::front::Row < inited                  , start_evt               , starting                , do_load                                     >,
        //    +------------------------------------------+-------------------------+-------------------------+------------------------+--------------------+
        boost::msm::front::Row < starting
                                 ::exit_pt
                                 <starting_
                                  ::starting_exit >      , graph_execd_evt         , running                                                               >,
        boost::msm::front::Row < starting                , err_evt                 , restarting              , boost::msm::front::none, boost::msm::front::euml::Not_<
                                                                                                                                          is_fatal_error>  >,
        boost::msm::front::Row < starting                , err_evt                 , stopped                 , do_report_fatal_error  , is_fatal_error     >,
        //    +------------------------------------------+-------------------------+-------------------------+------------------------+--------------------+
        boost::msm::front::Row < running                 , next_evt                , boost::msm::front::none , do_next                                     >,
        boost::msm::front::Row < running                 , prev_evt                , boost::msm::front::none , do_prev                                     >,
        boost::msm::front::Row < running                 , fwd_evt                 , boost::msm::front::none , do_fwd                                      >,
        boost::msm::front::Row < running                 , rwd_evt                 , boost::msm::front::none , do_rwd                                      >,
        boost::msm::front::Row < running                 , vol_up_evt              , boost::msm::front::none , do_vol_up                                   >,
        boost::msm::front::Row < running                 , vol_down_evt            , boost::msm::front::none , do_vol_down                                 >,
        boost::msm::front::Row < running                 , mute_evt                , boost::msm::front::none , do_mute                                     >,
        boost::msm::front::Row < running                 , pause_evt               , boost::msm::front::none , do_pause                                    >,
        boost::msm::front::Row < running                 , stop_evt                , stopping                , do_unload                                   >,
        boost::msm::front::Row < running                 , graph_eop_evt           , restarting              , boost::msm::front::none                     >,
        boost::msm::front::Row < running                 , err_evt                 , restarting              , boost::msm::front::none, boost::msm::front::euml::Not_<
                                                                                                                                          is_fatal_error>  >,
        boost::msm::front::Row < running                 , err_evt                 , stopped                 , do_report_fatal_error  , is_fatal_error     >,
        //    +------------------------------------------+-------------------------+-------------------------+---------------------------------------------+
        boost::msm::front::Row < restarting
                                 ::exit_pt
                                 <restarting_
                                  ::restarting_exit >    , graph_unlded_evt        , starting                , do_load                                     >,
        boost::msm::front::Row < restarting              , err_evt                 , stopped                 , do_report_fatal_error                       >,
        //    +------------------------------------------+-------------------------+----------------+------------------------------------------------------+
        boost::msm::front::Row < stopping
                                 ::exit_pt
                                 <stopping_
                                  ::stopping_exit >      , graph_unlded_evt        , stopped                                                               >,
        boost::msm::front::Row < stopping                , err_evt                 , stopped                 , do_report_fatal_error                       >
        //    +------------------------------------------+-------------------------+----------------+------------------------------------------------------+
        > {};

      // Replaces the default no-transition response.
      template <class FSM,class Event>
      void no_transition(Event const& e, FSM&,int state)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state [%s] on event [%s]",
                 tiz::graphmgr::state_names[state], typeid(e).name());
      }
    };
    // typedef boost::msm::back::state_machine<fsm_, boost::msm::back::mpl_graph_fsm_check> fsm;
    typedef boost::msm::back::state_machine<fsm_> fsm;

    static char const* const pstate(fsm const& p)
    {
      return tiz::graphmgr::state_names[p.current_state()[0]];
    }

  } // namespace graphmgr
} // namespace tiz

#endif // TIZGRAPHMGRFSM_H

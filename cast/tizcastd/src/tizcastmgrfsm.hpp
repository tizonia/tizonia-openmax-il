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
 * @file   tizcastmgrfsm.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Cast mgr fsm class
 *
 *
 */

#ifndef TIZCASTMGRFSM_HPP
#define TIZCASTMGRFSM_HPP

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 40
#define FUSION_MAX_VECTOR_SIZE      20
#define SPIRIT_ARGUMENTS_LIMIT      20

#include <sys/time.h>

#include <boost/msm/back/state_machine.hpp>
//#include <boost/msm/back/mpl_cast_fsm_check.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/back/tools.hpp>

#include <tizplatform.h>

#include "tizcasttypes.hpp"
#include "tizplaybackstatus.hpp"
#include "tizcastmgrops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.cast.mgr.fsm"
#endif

#define GMGR_FSM_LOG()                                                  \
  do                                                                    \
    {                                                                   \
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid(*this).name ());      \
    }                                                                   \
  while(0)

namespace bmf = boost::msm::front;
namespace tc = tiz::control;

namespace tiz
{
  namespace castmgr
  {
    static char const* const state_names[] = { "inited",
                                               "starting",
                                               "running",
                                               "restarting",
                                               "stopping",
                                               "stopped",
                                               "quitting",
                                               "quitted"};

    // main fsm events
    struct start_evt {};
    struct quit_evt {};
    struct connect_evt {};
    struct disconnect_evt {};
    struct load_url_evt {};
    struct play_evt {};
    struct stop_evt {};
    struct pause_evt {};
    struct volume_up_evt {};
    struct volume_down_evt {};
    struct mute_evt {};
    struct unmute_evt {};
    struct poll_evt {};
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

    // Concrete FSM implementation
    struct fsm_ : public boost::msm::front::state_machine_def<fsm_>
    {
      // no need for exception handling
      typedef int no_exception_thrown;

      /* Forward declarations */
      struct running;
      struct stopped;
      struct quitted;
      struct loading_cast;
      struct executing_cast;
      struct stopping_cast;
      struct unloading_cast;
      struct do_execute_cast;
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
          assert (pp_ops);
        }

        // submachine states
        struct loading_cast : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const&, FSM& fsm) {GMGR_FSM_LOG ();}
        };

        struct starting_exit : public boost::msm::front::exit_pseudo_state<cast_execd_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
        };

        // the initial state. Must be defined
        typedef loading_cast initial_state;

        // transition actions

        // guard conditions

        // Transition table for starting
        struct transition_table : boost::mpl::vector<
          //       Start               Event               Next                Action             Guard
          //    +--+-------------------+-------------------+-------------------+------------------+-----+
          bmf::Row < loading_cast     , cast_loaded_evt  , executing_cast   , do_execute_cast       >,
          bmf::Row < loading_cast     , cast_execd_evt   , starting_exit                              >,
          bmf::Row < executing_cast   , cast_execd_evt   , starting_exit                              >
          //    +--+-------------------+-------------------+-------------------+------------------+-----+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }

      };
      // typedef boost::msm::back::state_machine<starting_, boost::msm::back::mpl_cast_fsm_check> starting;
      typedef boost::msm::back::state_machine<starting_> starting;

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
          assert (pp_ops);
        }

        struct restarting_exit : public boost::msm::front::exit_pseudo_state<cast_unlded_evt>
        {
          typedef boost::mpl::vector<next_evt, prev_evt, fwd_evt, rwd_evt, vol_up_evt, vol_down_evt, vol_evt, mute_evt, pause_evt, stop_evt, quit_evt> deferred_events;
          template <class Event,class FSM>
          void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
        };

        // the initial state. Must be defined
        typedef unloading_cast initial_state;

        // transition actions

        // guard conditions

        // Transition table for restarting
        struct transition_table : boost::mpl::vector<
          //        Start            Event                Next               Action                   Guard
          //    +---+----------------+--------------------+------------------+------------------------+--------+
          bmf::Row < unloading_cast , cast_unlded_evt   , restarting_exit                                    >
          //    +---+----------------+--------------------+------------------+------------------------+--------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }
      };
      // typedef boost::msm::back::state_machine<restarting_, boost::msm::back::mpl_cast_fsm_check> restarting;
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
          assert (pp_ops);
        }

        // The list of FSM states
        struct stopping_exit : public boost::msm::front::exit_pseudo_state<cast_stopped_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
        };

        // the initial state. Must be defined
        typedef stopping_cast initial_state;

        // transition actions

        // guard conditions

        // Transition table for stopping
        struct transition_table : boost::mpl::vector<
          //       Start             Event                Next             Action                   Guard
          //    +--+-----------------+--------------------+----------------+------------------------+--------+
          bmf::Row < stopping_cast , cast_stopped_evt   , stopping_exit                                    >
          //    +--+-----------------+--------------------+----------------+------------------------+--------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }
      };
      // typedef boost::msm::back::state_machine<stopping_, boost::msm::back::mpl_cast_fsm_check> stopping;
      typedef boost::msm::back::state_machine<stopping_> stopping;

      /* quitting is a submachine */
      struct quitting_ : public boost::msm::front::state_machine_def<quitting_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;

        // data members
        ops ** pp_ops_;

        quitting_ ()
          :
          pp_ops_(NULL)
        {}

        quitting_ (ops **pp_ops)
          :
          pp_ops_(pp_ops)
        {
          assert (pp_ops);
        }

        // The list of FSM states
        struct quitting_exit : public boost::msm::front::exit_pseudo_state<cast_unlded_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
        };

        // the initial state. Must be defined
        typedef unloading_cast initial_state;

        // transition actions

        // guard conditions

        // Transition table for quitting
        struct transition_table : boost::mpl::vector<
          //       Start             Event                Next             Action                   Guard
          //    +--+-----------------+--------------------+----------------+------------------------+--------+
          bmf::Row < unloading_cast , cast_unlded_evt   , quitting_exit                                    >
          //    +--+-----------------+--------------------+----------------+------------------------+--------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }
      };
      // typedef boost::msm::back::state_machine<quitting_, boost::msm::back::mpl_cast_fsm_check> quitting;
      typedef boost::msm::back::state_machine<quitting_> quitting;

      struct executing_cast : public boost::msm::front::state<>
      {
        typedef boost::mpl::vector<next_evt, prev_evt, fwd_evt, rwd_evt, vol_up_evt, vol_down_evt, vol_evt, mute_evt, pause_evt, stop_evt, quit_evt> deferred_events;
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      struct running : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const&, FSM& fsm)
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            (*(fsm.pp_ops_))->do_update_control_ifcs (tiz::control::Playing);
          }
        }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };


      struct stopping_cast : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) {GMGR_FSM_LOG ();}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      struct stopped : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            (*(fsm.pp_ops_))->do_update_control_ifcs (tiz::control::Stopped);
          }
        }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      struct unloading_cast : public boost::msm::front::state<>
      {
        typedef boost::mpl::vector<next_evt, prev_evt, fwd_evt, rwd_evt, vol_up_evt, vol_down_evt, vol_evt, mute_evt, pause_evt> deferred_events;
        template <class Event,class FSM>
        void on_entry(Event const&, FSM& fsm) {GMGR_FSM_LOG ();}
      };

      // terminate state
      struct quitted : public boost::msm::front::terminate_state<>
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

      struct do_execute_cast
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_execute ();
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

      struct do_vol
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_vol (evt.vol_);
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

      struct do_stop
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_stop ();
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

      struct do_deinit
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_deinit ();
            }
        }
      };

      template<tiz::control::playback_status_t playstatus>
      struct do_update_control_ifcs
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_update_control_ifcs (playstatus);
            }
        }
      };

      struct do_update_metadata
      {
        template < class FSM, class EVT, class SourceState, class TargetState >
        void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_update_metadata (evt.metadata_);
            }
        }
      };

      struct do_update_volume
      {
        template < class FSM, class EVT, class SourceState, class TargetState >
        void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_update_volume (evt.volume_);
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

      struct do_end_of_play
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_end_of_play ();
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

      // Transition table for the cast mgr fsm
      struct transition_table : boost::mpl::vector<
        //         Start                 Event              Next          Action                   Guard
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < inited                , start_evt        , starting    , do_load                                     >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < starting
                   ::exit_pt
                   <starting_
                    ::starting_exit >    , cast_execd_evt  , running                                                   >,
        bmf::Row < starting              , err_evt          , restarting  , bmf::none              , bmf::euml::Not_<
                                                                                                       is_fatal_error>  >,
        bmf::Row < starting              , quit_evt         , quitting    , do_unload                                   >,
        bmf::Row < starting              , err_evt          , quitted     , do_report_fatal_error  , is_fatal_error     >,
        bmf::Row < starting              , cast_unlded_evt , quitted     , do_end_of_play                              >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < running               , next_evt         , bmf::none   , do_next                                     >,
        bmf::Row < running               , prev_evt         , bmf::none   , do_prev                                     >,
        bmf::Row < running               , fwd_evt          , bmf::none   , do_fwd                                      >,
        bmf::Row < running               , rwd_evt          , bmf::none   , do_rwd                                      >,
        bmf::Row < running               , vol_up_evt       , bmf::none   , do_vol_up                                   >,
        bmf::Row < running               , vol_down_evt     , bmf::none   , do_vol_down                                 >,
        bmf::Row < running               , vol_evt          , bmf::none   , do_vol                                      >,
        bmf::Row < running               , mute_evt         , bmf::none   , do_mute                                     >,
        bmf::Row < running               , pause_evt        , bmf::none   , do_pause                                    >,
        bmf::Row < running               , cast_paused_evt , bmf::none   , do_update_control_ifcs<tc::Paused>          >,
        bmf::Row < running               , cast_unpaused_evt, bmf::none  , do_update_control_ifcs<tc::Playing>         >,
        bmf::Row < running               , cast_metadata_evt, bmf::none  , do_update_metadata                          >,
        bmf::Row < running               , cast_volume_evt , bmf::none   , do_update_volume                            >,
        bmf::Row < running               , start_evt        , bmf::none   , do_pause                                    >,
        bmf::Row < running               , stop_evt         , stopping    , do_stop                                     >,
        bmf::Row < running               , quit_evt         , quitting    , do_unload                                   >,
        bmf::Row < running               , cast_eop_evt    , restarting  , bmf::none                                   >,
        bmf::Row < running               , err_evt          , restarting  , bmf::none              , bmf::euml::Not_<
                                                                                                        is_fatal_error> >,
        bmf::Row < running               , err_evt          , quitted     , do_report_fatal_error  , is_fatal_error     >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < restarting
                   ::exit_pt
                   <restarting_
                    ::restarting_exit >  , cast_unlded_evt , starting    , bmf::ActionSequence_<
                                                                              boost::mpl::vector<
                                                                                do_deinit,
                                                                                do_load> >                              >,
        bmf::Row < restarting            , err_evt          , quitted     , do_report_fatal_error                       >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < stopping
                   ::exit_pt
                   <stopping_
                    ::stopping_exit >    , cast_stopped_evt, stopped                                                   >,
        bmf::Row < stopping              , err_evt          , quitted     , do_report_fatal_error                       >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < stopped               , start_evt        , starting    , do_execute_cast                            >,
        bmf::Row < stopped               , quit_evt         , quitting    , do_unload                                   >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < quitting
                   ::exit_pt
                   <quitting_
                    ::quitting_exit >    , cast_unlded_evt , quitted                                                   >,
        bmf::Row < quitting              , err_evt          , quitted     , do_report_fatal_error                       >
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        > {};

      // Replaces the default no-transition response.
      template <class FSM,class Event>
      void no_transition(Event const& e, FSM&,int state)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state [%s] on event [%s]",
                 tiz::castmgr::state_names[state], typeid(e).name());
      }
    };
    // typedef boost::msm::back::state_machine<fsm_, boost::msm::back::mpl_cast_fsm_check> fsm;
    typedef boost::msm::back::state_machine<fsm_> fsm;

    // Helper function to aid in printing the current state when debugging
    char const* const pstate(fsm const& p);

  } // namespace castmgr
} // namespace tiz

#endif // TIZCASTMGRFSM_HPP

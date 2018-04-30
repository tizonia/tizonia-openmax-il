/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphfsm.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph fsm
 *
 */

#ifndef TIZGRAPHFSM_HPP
#define TIZGRAPHFSM_HPP

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 40
#define FUSION_MAX_VECTOR_SIZE      20
#define SPIRIT_ARGUMENTS_LIMIT      20

#include <sys/time.h>

#include <boost/msm/back/state_machine.hpp>
//#include <boost/msm/back/mpl_graph_fsm_check.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/back/tools.hpp>

#include <tizplatform.h>

#include "tizgraphops.hpp"
#include "tizgraphevt.hpp"
#include "tizgraphguard.hpp"
#include "tizgraphaction.hpp"
#include "tizgraphstate.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.fsm"
#endif

#define G_FSM_LOG()                                                     \
  do                                                                    \
    {                                                                   \
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid(*this).name ());      \
    }                                                                   \
  while(0)

namespace bmf = boost::msm::front;

namespace tiz
{
  namespace graph
  {
    static char const* const state_names[] = { "inited",
                                               "loaded",
                                               "configuring",
                                               "executing",
                                               "skipping",
                                               "exe2pause",
                                               "pause",
                                               "pause2exe",
                                               "pause2idle",
                                               "exe2idle",
                                               "idle",
                                               "idle2loaded",
                                               "AllOk",
                                               "unloaded"};


    // Concrete FSM implementation
    struct fsm_ : public boost::msm::front::state_machine_def<fsm_>
    {
      // no need for exception handling
      typedef int no_exception_thrown;

      // data members
      ops ** pp_ops_;
      bool terminated_;

      fsm_(ops **pp_ops)
        :
        pp_ops_(pp_ops),
        terminated_ (false)
      {
        assert (pp_ops);
      }

      // states

      /* 'configuring' is a submachine */
      struct configuring_ : public boost::msm::front::state_machine_def<configuring_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;

        // data members
        ops ** pp_ops_;

        configuring_()
          :
          pp_ops_(NULL)
        {}
        configuring_(ops **pp_ops)
          :
        pp_ops_(pp_ops)
        {
          assert (pp_ops);
        }

        // submachine states
        struct probing : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        struct conf_exit : public boost::msm::front::exit_pseudo_state<configured_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef disabling_ports<0, 0> disabling_comp_ports;
        typedef disabling_comp_ports initial_state;

        // transition actions

        // guard conditions

        // Transition table for configuring
        struct transition_table : boost::mpl::vector<
          //                       Start                       Event                      Next                         Action                                 Guard
          //    +-----------------+----------------------------+--------------------------+----------------------------+--------------------------------------+----------------------------------------+
          boost::msm::front::Row < disabling_comp_ports        , boost::msm::front::none  , awaiting_port_disabled_evt , boost::msm::front::none              , is_disabled_evt_required               >,
          boost::msm::front::Row < disabling_comp_ports        , boost::msm::front::none  , probing                    , do_probe                             , boost::msm::front::euml::Not_<
                                                                                                                                                                  is_disabled_evt_required >           >,
          //    +-----------------+----------------------------+--------------------------+----------------------------+--------------------------------------+----------------------------------------+
          boost::msm::front::Row < awaiting_port_disabled_evt  , omx_port_disabled_evt    , probing                    , do_probe                             , is_port_disabling_complete             >,
          //    +-----------------+----------------------------+--------------------------+----------------------------+--------------------------------------+----------------------------------------+
          boost::msm::front::Row < probing                     , boost::msm::front::none  , awaiting_port_settings_evt , boost::msm::front::none              , is_port_settings_evt_required          >,
          boost::msm::front::Row < probing                     , boost::msm::front::none  , config2idle                , boost::msm::front::ActionSequence_<
                                                                                                                           boost::mpl::vector<
                                                                                                                             do_configure,
                                                                                                                             do_loaded2idle > >           , boost::msm::front::euml::Not_<
                                                                                                                                                                  is_port_settings_evt_required >      >,
          boost::msm::front::Row < probing                     , boost::msm::front::none  , conf_exit                  , boost::msm::front::none              , is_end_of_play                         >,
          boost::msm::front::Row < probing                     , boost::msm::front::none  , probing                    , boost::msm::front::ActionSequence_<
                                                                                                                           boost::mpl::vector<
                                                                                                                             do_reset_internal_error,
                                                                                                                             do_skip,
                                                                                                                             do_probe > >                     , boost::msm::front::euml::And_<
                                                                                                                                                                  boost::msm::front::euml::Not_<
                                                                                                                                                                    is_end_of_play >,
                                                                                                                                                                  boost::msm::front::euml::Not_<
                                                                                                                                                                    is_probing_result_ok > >           >,
          //    +-----------------+----------------------------+--------------------------+----------------------------+--------------------------------------+----------------------------------------+
          boost::msm::front::Row < awaiting_port_settings_evt  , omx_port_settings_evt    , config2idle                , boost::msm::front::ActionSequence_<
                                                                                                                           boost::mpl::vector<
                                                                                                                             do_configure,
                                                                                                                             do_loaded2idle > >                                                    >,
          //    +-----------------+----------------------------+--------------------------+----------------------------+--------------------------------------+----------------------------------------+
          boost::msm::front::Row < config2idle                 , omx_trans_evt            , idle2exe                   , do_idle2exe                      , is_trans_complete                      >,
          //    +-----------------+----------------------------+--------------------------+----------------------------+--------------------------------------+----------------------------------------+
          boost::msm::front::Row < idle2exe                    , omx_trans_evt            , conf_exit                  , boost::msm::front::none              , is_trans_complete                      >
          //    +-----------------+----------------------------+--------------------------+----------------------------+--------------------------------------+----------------------------------------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }

      };
      // typedef boost::msm::back::state_machine<configuring_, boost::msm::back::mpl_graph_fsm_check> configuring;
      typedef boost::msm::back::state_machine<configuring_> configuring;

      /* 'skipping' is a submachine */
      struct skipping_ : public boost::msm::front::state_machine_def<skipping_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;

        // data members
        ops ** pp_ops_;
        int   jump_;

        skipping_()
          :
          pp_ops_(NULL),
          jump_ (1)
        {}
        skipping_(ops **pp_ops)
          :
          pp_ops_(pp_ops),
          jump_ (1)
        {
          assert (pp_ops);
        }

        // submachine states
        struct to_idle : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm)
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                (*(fsm.pp_ops_))->do_exe2idle ();
              }
          }
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          OMX_STATETYPE target_omx_state () const
          {
            return OMX_StateIdle;
          }
        };

        struct skip_exit : public boost::msm::front::exit_pseudo_state<skipped_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef to_idle initial_state;

        // transition actions

        // guard conditions

        // Transition table for skipping
        struct transition_table : boost::mpl::vector<
          //                       Start             Event            Next                   Action                           Guard
          //    +-----------------+------------------+----------------+----------------------+--------------------------------+---------------------------+
          boost::msm::front::Row < to_idle           , omx_trans_evt  , idle2loaded          , do_idle2loaded             , is_trans_complete         >,
          boost::msm::front::Row < idle2loaded       , omx_trans_evt  , skip_exit            , do_skip                        , is_trans_complete         >
          //    +-----------------+------------------+----------------+----------------------+--------------------------------+---------------------------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }

      };
      // typedef boost::msm::back::state_machine<skipping_, boost::msm::back::mpl_graph_fsm_check> skipping;
      typedef boost::msm::back::state_machine<skipping_> skipping;

      // The initial state of the SM. Must be defined
      typedef boost::mpl::vector<inited, AllOk> initial_state;

      // transition actions

      // guard conditions

      // Transition table for the graph fsm
      struct transition_table : boost::mpl::vector<
        //                       Start       Event             Next                      Action                    Guard
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < inited      , load_evt        , loaded                  , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_load,
                                                                                               do_setup,
                                                                                               do_ack_loaded> >                           >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < loaded      , execute_evt     , configuring             , do_store_config         , last_op_succeeded    >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < configuring , omx_err_evt     , unloaded                , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_record_fatal_error,
                                                                                               do_error,
                                                                                               do_tear_down_tunnels,
                                                                                               do_destroy_graph> > , is_fatal_error       >,
        boost::msm::front::Row < configuring
                                 ::exit_pt
                                 <configuring_
                                  ::conf_exit>, configured_evt , executing               , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_retrieve_metadata,
                                                                                               do_ack_execd,
                                                                                               do_start_progress_display> >               >,
        boost::msm::front::Row < configuring
                                 ::exit_pt
                                 <configuring_
                                  ::conf_exit>, configured_evt , unloaded                , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_end_of_play,
                                                                                               do_tear_down_tunnels,
                                                                                               do_destroy_graph> > , is_end_of_play       >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < executing   , skip_evt        , skipping                , do_store_skip                                  >,
        boost::msm::front::Row < executing   , seek_evt        , boost::msm::front::none , do_seek                                        >,
        boost::msm::front::Row < executing   , volume_step_evt , boost::msm::front::none , do_volume_step                                 >,
        boost::msm::front::Row < executing   , volume_evt      , boost::msm::front::none , do_volume                                      >,
        boost::msm::front::Row < executing   , mute_evt        , boost::msm::front::none , do_mute                                        >,
        boost::msm::front::Row < executing   , pause_evt       , exe2pause               , do_exe2pause                               >,
        boost::msm::front::Row < executing   , stop_evt        , exe2idle                , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_record_destination < OMX_StateIdle >,
                                                                                               do_exe2idle> >                         >,
        boost::msm::front::Row < executing   , unload_evt      , exe2idle                , do_exe2idle                                >,
        boost::msm::front::Row < executing   , omx_err_evt     , skipping                , boost::msm::front::none                        >,
        boost::msm::front::Row < executing   , omx_err_evt     , skipping                , do_record_fatal_error   , is_fatal_error       >,
        boost::msm::front::Row < executing   , omx_eos_evt     , skipping                , boost::msm::front::none , is_last_eos          >,
        boost::msm::front::Row < executing   , timer_evt       , boost::msm::front::none , do_increase_progress_display                   >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < skipping
                                 ::exit_pt
                                 <skipping_
                                  ::skip_exit>, skipped_evt    , unloaded                , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_error,
                                                                                               do_tear_down_tunnels,
                                                                                               do_destroy_graph> > , is_internal_error    >,
        boost::msm::front::Row < skipping
                                 ::exit_pt
                                 <skipping_
                                  ::skip_exit>, skipped_evt    , unloaded                , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_end_of_play,
                                                                                               do_tear_down_tunnels,
                                                                                               do_destroy_graph> > , is_end_of_play       >,
        boost::msm::front::Row < skipping
                                 ::exit_pt
                                 <skipping_
                                  ::skip_exit>, skipped_evt    , configuring             , do_stop_progress_display , boost::msm::front::euml::Not_<
                                                                                                                       is_end_of_play>   >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < exe2pause   , omx_trans_evt   , pause                   , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_ack_paused,
                                                                                               do_pause_progress_display
                                                                                               > >                 , is_trans_complete    >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < pause       , execute_evt     , pause2exe               , do_pause2exe                               >,
        boost::msm::front::Row < pause       , pause_evt       , pause2exe               , do_pause2exe                               >,
        boost::msm::front::Row < pause       , stop_evt        , pause2idle              , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_record_destination < OMX_StateIdle >,
                                                                                               do_pause2idle > >                      >,
        boost::msm::front::Row < pause       , unload_evt      , pause2idle              , do_pause2idle                              >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < pause2exe   , omx_trans_evt   , executing               , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_ack_resumed,
                                                                                               do_resume_progress_display >
                                                                                               >                   , is_trans_complete    >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < pause2idle  , omx_trans_evt   , idle2loaded             , do_idle2loaded      , is_trans_complete    >,
        boost::msm::front::Row < pause2idle  , omx_trans_evt   , idle                    , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_record_destination <
                                                                                                 OMX_StateMax >,
                                                                                               do_ack_stopped,
                                                                                               do_stop_progress_display> >  , bmf::euml::And_<
                                                                                                                       is_trans_complete,
                                                                                                                       is_destination_state <
                                                                                                                         OMX_StateIdle > > >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < exe2idle    , omx_trans_evt   , idle2loaded             , do_idle2loaded      , is_trans_complete    >,
        boost::msm::front::Row < exe2idle    , omx_trans_evt   , idle                    , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_record_destination <
                                                                                                 OMX_StateMax >,
                                                                                               do_ack_stopped,
                                                                                               do_stop_progress_display> >  , bmf::euml::And_<
                                                                                                                       is_destination_state <
                                                                                                                         OMX_StateIdle >,
                                                                                                                         is_trans_complete > >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < idle        , execute_evt     , executing               , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_ack_execd,
                                                                                               do_idle2exe,
                                                                                               do_start_progress_display> >                        >,
        boost::msm::front::Row < idle        , unload_evt      , idle2loaded             , do_idle2loaded                             >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < idle2loaded , omx_trans_evt   , unloaded                , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_tear_down_tunnels,
                                                                                               do_destroy_graph> > , is_trans_complete    >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < AllOk       , err_evt         , unloaded                , do_error                                       >
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        > {};

      // Replaces the default no-transition response.
      template <class FSM,class Event>
      void no_transition(Event const& e, FSM&,int state)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state [%s] on event [%s]",
                 tiz::graph::state_names[state], typeid(e).name());
      }
    };
    // typedef boost::msm::back::state_machine<fsm_, boost::msm::back::mpl_graph_fsm_check> fsm;
    typedef boost::msm::back::state_machine<fsm_> fsm;

    // Helper function to aid in printing the current state when debugging
    char const* const pstate(fsm const& p);

  } // namespace graph
} // namespace tiz

#endif // TIZGRAPHFSM_HPP

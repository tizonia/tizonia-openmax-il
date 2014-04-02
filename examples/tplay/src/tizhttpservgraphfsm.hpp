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
 * @file   tizhttpservgraphfsm.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  HTTP server graph's fsm variations
 *
 */

#ifndef TIZHTTPSERVGRAPHFSM_H
#define TIZHTTPSERVGRAPHFSM_H

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 30
#define FUSION_MAX_VECTOR_SIZE      20
#define SPIRIT_ARGUMENTS_LIMIT      20

#include <sys/time.h>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/back/mpl_graph_fsm_check.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/back/tools.hpp>

#include <tizosal.h>

#include "tizgraphfsm.hpp"
#include "tizgraphevt.hpp"
#include "tizgraphguard.hpp"
#include "tizgraphaction.hpp"
#include "tizgraphstate.hpp"
#include "tizhttpservgraphops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.httpservfsm"
#endif

#define G_FSM_LOG()                                                     \
  do                                                                    \
    {                                                                   \
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid(*this).name ());      \
    }                                                                   \
  while(0)

namespace tiz
{
  namespace graph
  {
    namespace hsfsm
    {

      // Some common guard conditions
      struct is_initial_configuration
      {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const & evt, FSM & fsm, SourceState & source, TargetState & target)
        {
          bool rc = false;
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              // This is a httpservops-specific guard
              rc = dynamic_cast<httpservops*>(*(fsm.pp_ops_))->is_initial_configuration ();
            }
          TIZ_LOG (TIZ_PRIORITY_TRACE, " is_initial_configuration [%s]", rc ? "YES" : "NO");
          return rc;
        }
      };

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
        assert (NULL != pp_ops);
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
          assert (NULL != pp_ops);
        }

        // submachine states
        struct configuring_server : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        struct probing : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        // TODO: Move this with the common states
        struct enabling_tunnel : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        struct conf_exit : public boost::msm::front::exit_pseudo_state<tiz::graph::configured_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef configuring_server initial_state;

        // transition actions

        struct do_configure_server
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_configure_server ();
              }
          }
        };

        struct do_configure_station
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_configure_station ();
              }
          }
        };

        struct do_configure_stream
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_configure_stream ();
              }
          }
        };

        struct do_source_omx_loaded2idle
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_source_omx_loaded2idle ();
              }
          }
        };

        struct do_source_omx_idle2exe
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_source_omx_idle2exe ();
              }
          }
        };

        // TODO: Move this with the common actions
        struct do_enable_tunnel
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_enable_tunnel ();
              }
          }
        };

        struct do_flag_initial_config_done
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_flag_initial_config_done ();
              }
          }
        };

        // guard conditions

        // Transition table for configuring
        struct transition_table : boost::mpl::vector<
          //                       Start                   Event                       Next                      Action                                 Guard
          //    +-----------------+------------------------+---------------------------+-------------------------+--------------------------------------+----------------------------+
          boost::msm::front::Row < configuring_server      , boost::msm::front::none   , probing                 , boost::msm::front::ActionSequence_<
                                                                                                                     boost::mpl::vector<
                                                                                                                       do_configure_server,
                                                                                                                       do_configure_station,
                                                                                                                       tiz::graph::do_probe > >         , is_initial_configuration      >,
          boost::msm::front::Row < configuring_server      , boost::msm::front::none   , probing                 , tiz::graph::do_probe                 , boost::msm::front::euml::Not_<
                                                                                                                                                            is_initial_configuration>   >,
          //    +-----------------+------------------------+---------------------------+-------------------------+--------------------------------------+----------------------------+
          boost::msm::front::Row < probing                 , boost::msm::front::none   , tiz::graph::config2idle , boost::msm::front::ActionSequence_<
                                                                                                                     boost::mpl::vector<
                                                                                                                       do_configure_stream,
                                                                                                                       tiz::graph::do_omx_loaded2idle > >, is_initial_configuration      >,
          boost::msm::front::Row < probing                 , boost::msm::front::none   , tiz::graph::config2idle , boost::msm::front::ActionSequence_<
                                                                                                                     boost::mpl::vector<
                                                                                                                       do_configure_stream,
                                                                                                                       do_source_omx_loaded2idle > >    , boost::msm::front::euml::Not_<
                                                                                                                                                            is_initial_configuration>     >,
          boost::msm::front::Row < probing                 , boost::msm::front::none  , conf_exit                , boost::msm::front::none              , tiz::graph::is_end_of_play      >,
          boost::msm::front::Row < probing                 , boost::msm::front::none  , probing                  , boost::msm::front::ActionSequence_<
                                                                                                                     boost::mpl::vector<
                                                                                                                       tiz::graph::do_reset_internal_error,
                                                                                                                       tiz::graph::do_skip,
                                                                                                                       tiz::graph::do_probe > >         , boost::msm::front::euml::And_<
                                                                                                                                                            boost::msm::front::euml::Not_<
                                                                                                                                                              tiz::graph::is_end_of_play >,
                                                                                                                                                            boost::msm::front::euml::Not_<
                                                                                                                                                              tiz::graph::is_probing_result_ok > >  >,
          //    +-----------------+------------------------+---------------------------+-------------------------+--------------------------------------+-------------------------------+
          boost::msm::front::Row < tiz::graph::config2idle , tiz::graph::omx_trans_evt , tiz::graph::idle2exe    , tiz::graph::do_omx_idle2exe          , boost::msm::front::euml::And_<
                                                                                                                                                            is_initial_configuration,
                                                                                                                                                            tiz::graph::is_trans_complete>  >,
          boost::msm::front::Row < tiz::graph::config2idle , tiz::graph::omx_trans_evt , tiz::graph::idle2exe    , do_source_omx_idle2exe               , boost::msm::front::euml::And_<
                                                                                                                                                            boost::msm::front::euml::Not_<
                                                                                                                                                              is_initial_configuration>,
                                                                                                                                                            tiz::graph::is_trans_complete >  >,
          //    +-----------------+------------------------+---------------------------+-------------------------+--------------------------------------+-------------------------------+
          boost::msm::front::Row < tiz::graph::idle2exe    , tiz::graph::omx_trans_evt , conf_exit               , do_flag_initial_config_done          , boost::msm::front::euml::And_<
                                                                                                                                                            is_initial_configuration,
                                                                                                                                                            tiz::graph::is_trans_complete >  >,
          boost::msm::front::Row < tiz::graph::idle2exe    , tiz::graph::omx_trans_evt , enabling_tunnel         , do_enable_tunnel                     , boost::msm::front::euml::And_<
                                                                                                                                                            boost::msm::front::euml::Not_<
                                                                                                                                                              is_initial_configuration>,
                                                                                                                                                            tiz::graph::is_trans_complete >   >,
          //    +-----------------+------------------------+---------------------------+-------------------------+--------------------------------------+-------------------------------+
          boost::msm::front::Row < enabling_tunnel         , tiz::graph::omx_port_enabled_evt , conf_exit        , boost::msm::front::none              , tiz::graph::is_port_enabling_complete >
          //    +-----------------+------------------------+---------------------------+-------------------------+--------------------------------------+-------------------------------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }

      };
      // typedef boost::msm::back::state_machine<configuring_, boost::msm::back::mpl_graph_fsm_check> configuring;
      typedef boost::msm::back::state_machine<configuring_> configuring;

      /* 'skipping' is a submachine of tiz::graph::fsm_ */
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
          assert (NULL != pp_ops);
        }

        // submachine states
        struct skipping_initial : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        struct to_idle : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          OMX_STATETYPE target_omx_state () const
          {
            return OMX_StateIdle;
          }
        };

        // TODO: Move this with the common states
        struct disabling_tunnel : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        struct skip_exit : public boost::msm::front::exit_pseudo_state<tiz::graph::skipped_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef skipping_initial initial_state;

        // transition actions

        // TODO: Move this with the common actions
        struct do_disable_tunnel
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_disable_tunnel ();
              }
          }
        };

        struct do_source_omx_exe2idle
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_source_omx_exe2idle ();
              }
          }
        };

        struct do_source_omx_idle2loaded
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                // This is a httpservops-specific method
                dynamic_cast<httpservops*>(*(fsm.pp_ops_))->do_source_omx_idle2loaded ();
              }
          }
        };

        // guard conditions

        // Transition table for skipping
        struct transition_table : boost::mpl::vector<
          //                       Start                         Event                               Next                      Action                      Guard
          //    +-----------------+------------------------------+-----------------------------------+-------------------------+---------------------------+---------------------------+
          boost::msm::front::Row < skipping_initial              , boost::msm::front::none           , disabling_tunnel        , do_disable_tunnel                                      >,
          boost::msm::front::Row < disabling_tunnel              , tiz::graph::omx_port_disabled_evt , to_idle                 , do_source_omx_exe2idle    , tiz::graph::is_port_disabling_complete >,
          boost::msm::front::Row < to_idle                       , tiz::graph::omx_trans_evt         , tiz::graph::idle2loaded , do_source_omx_idle2loaded , tiz::graph::is_trans_complete >,
          boost::msm::front::Row < tiz::graph::idle2loaded       , tiz::graph::omx_trans_evt         , skip_exit               , tiz::graph::do_skip       , tiz::graph::is_trans_complete >
          //    +-----------------+------------------------------+-----------------------------------+-------------------------+---------------------------+---------------------------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }

      };
      // typedef boost::msm::back::state_machine<skipping_, boost::msm::back::mpl_graph_fsm_check> skipping;
      typedef boost::msm::back::state_machine<skipping_> skipping;

      // The initial state of the SM. Must be defined
      typedef boost::mpl::vector<tiz::graph::inited, tiz::graph::AllOk> initial_state;

      // transition actions


      // guard conditions

      // Transition table for the httpserver graph fsm
      struct transition_table : boost::mpl::vector<
        //                       Start                   Event                         Next                      Action                        Guard
        //    +------------------------------------------+-----------------------------+-------------------------+-----------------------------+------------------------------+
        boost::msm::front::Row < tiz::graph::inited      , tiz::graph::load_evt        , tiz::graph::loaded      , boost::msm::front::ActionSequence_<
                                                                                                                     boost::mpl::vector<
                                                                                                                       tiz::graph::do_load,
                                                                                                                       tiz::graph::do_setup,
                                                                                                                       tiz::graph::do_ack_loaded> >                           >,
        //    +------------------------------------------+-----------------------------+-------------------------+-----------------------------+------------------------------+
        boost::msm::front::Row < tiz::graph::loaded      , tiz::graph::execute_evt     , configuring             , tiz::graph::do_store_config , tiz::graph::last_op_succeeded>,
        //    +------------------------------------------+-----------------------------+-------------------------+-----------------------------+------------------------------+
        boost::msm::front::Row < configuring
                                 ::exit_pt
                                 <configuring_
                                  ::conf_exit>           , tiz::graph::configured_evt  , tiz::graph::executing   , tiz::graph::do_ack_execd                                   >,
        boost::msm::front::Row < configuring
                                 ::exit_pt
                                 <configuring_
                                  ::conf_exit>           , tiz::graph::configured_evt  , tiz::graph::unloaded    , boost::msm::front::ActionSequence_<
                                                                                                                     boost::mpl::vector<
                                                                                                                       tiz::graph::do_end_of_play,
                                                                                                                       tiz::graph::do_tear_down_tunnels,
                                                                                                                       tiz::graph::do_destroy_graph> > , tiz::graph::is_end_of_play  >,
        //    +------------------------------------------+-----------------------------+-------------------------+-----------------------------+------------------------------+
        boost::msm::front::Row < tiz::graph::executing   , tiz::graph::skip_evt        , skipping                , tiz::graph::do_store_skip                                  >,
        boost::msm::front::Row < tiz::graph::executing   , tiz::graph::unload_evt      , tiz::graph::exe2idle    , tiz::graph::do_omx_exe2idle                                >,
        boost::msm::front::Row < tiz::graph::executing   , tiz::graph::omx_err_evt     , skipping                , boost::msm::front::none                                    >,
        boost::msm::front::Row < tiz::graph::executing   , tiz::graph::omx_eos_evt     , skipping                , boost::msm::front::none     , tiz::graph::is_last_eos      >,
        //    +------------------------------------------+-----------------------------+-------------------------+-----------------------------+------------------------------+
        boost::msm::front::Row < skipping
                                 ::exit_pt
                                 <skipping_
                                  ::skip_exit>           , skipped_evt                 , tiz::graph::unloaded    , boost::msm::front::ActionSequence_<
                                                                                                                     boost::mpl::vector<
                                                                                                                       tiz::graph::do_error,
                                                                                                                       tiz::graph::do_tear_down_tunnels,
                                                                                                                       tiz::graph::do_destroy_graph> > , tiz::graph::is_fatal_error >,
        boost::msm::front::Row < skipping
                                 ::exit_pt
                                 <skipping_
                                  ::skip_exit>           , skipped_evt                 , tiz::graph::unloaded     , boost::msm::front::ActionSequence_<
                                                                                                                      boost::mpl::vector<
                                                                                                                        tiz::graph::do_end_of_play,
                                                                                                                        tiz::graph::do_tear_down_tunnels,
                                                                                                                        tiz::graph::do_destroy_graph> > , tiz::graph::is_end_of_play >,
        boost::msm::front::Row < skipping
                                 ::exit_pt
                                 <skipping_
                                  ::skip_exit>           , skipped_evt                 , configuring             , boost::msm::front::none , boost::msm::front::euml::Not_<
                                                                                                                                               tiz::graph::is_end_of_play>   >,
        //    +------------------------------------------+-----------------------------+-------------------------+-----------------------------+------------------------------+
        boost::msm::front::Row < tiz::graph::exe2idle    , tiz::graph::omx_trans_evt   , tiz::graph::idle2loaded , tiz::graph::do_omx_idle2loaded , tiz::graph::is_trans_complete    >,
        //    +------------------------------------------+-----------------------------+-------------------------+-----------------------------+------------------------------+
        boost::msm::front::Row < tiz::graph::idle2loaded , tiz::graph::omx_trans_evt   , tiz::graph::unloaded    , boost::msm::front::ActionSequence_<
                                                                                                                     boost::mpl::vector<
                                                                                                                       tiz::graph::do_tear_down_tunnels,
                                                                                                                       tiz::graph::do_destroy_graph> > , tiz::graph::is_trans_complete >,
        //    +------------------------------------------+-----------------------------+-------------------------+-----------------------------+------------------------------+
        boost::msm::front::Row < tiz::graph::AllOk       , tiz::graph::err_evt         , tiz::graph::unloaded    , tiz::graph::do_error                                       >
        //    +------------------------------------------+-----------------------------+-------------------------+-----------------------------+------------------------------+
        > {};

      // Replaces the default no-transition response.
      template <class FSM,class Event>
      void no_transition(Event const& e, FSM&,int state)
      {
        TIZ_LOG (TIZ_PRIORITY_TRACE, "no transition from state %d on event %s",
                 state, typeid(e).name());
      }
    };
    // typedef boost::msm::back::state_machine<fsm_, boost::msm::back::mpl_graph_fsm_check> fsm;
    typedef boost::msm::back::state_machine<fsm_> fsm;

    static char const* const state_names[] = { "inited",
                                               "loaded",
                                               "configuring",
                                               "executing",
                                               "skipping",
                                               "exe2idle",
                                               "idle2loaded",
                                               "AllOk"
                                               "unloaded"};
    static char const* const pstate(fsm const& p)
    {
      return tiz::graph::hsfsm::state_names[p.current_state()[0]];
    }

    } // namespace hsfsm
  } // namespace graph
} // namespace tiz

#endif // TIZHTTPSERVGRAPHFSM_H

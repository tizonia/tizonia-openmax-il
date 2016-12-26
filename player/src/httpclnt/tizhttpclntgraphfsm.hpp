/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   tizhttpclntgraphfsm.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  HTTP client graph fsm
 *
 */

#ifndef TIZHTTPCLNTGRAPHFSM_HPP
#define TIZHTTPCLNTGRAPHFSM_HPP

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
#include "tizhttpclntgraphops.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.httpclnt.fsm"
#endif

#define G_FSM_LOG()                                                     \
  do                                                                    \
    {                                                                   \
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid(*this).name ());      \
    }                                                                   \
  while(0)

namespace tg = tiz::graph;
namespace bmf = boost::msm::front;

namespace tiz
{
  namespace graph
  {
    namespace hcfsm
    {
      static char const* const state_names[] = { "inited",
                                                 "loaded",
                                                 "auto_detecting",
                                                 "updating_graph",
                                                 "executing",
                                                 "reconfiguring_graph",
                                                 "exe2idle",
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

      /* 'auto_detecting' is a submachine */
      struct auto_detecting_ : public boost::msm::front::state_machine_def<auto_detecting_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;

        // data members
        ops ** pp_ops_;

        auto_detecting_()
          :
          pp_ops_(NULL)
        {}
        auto_detecting_(ops **pp_ops)
          :
          pp_ops_(pp_ops)
        {
          assert (pp_ops);
        }

        // submachine states
        struct auto_detecting_exit : public boost::msm::front::exit_pseudo_state<tg::auto_detected_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef tg::disabling_ports initial_state;

        // transition actions

        // guard conditions

        // Transition table for auto_detecting
        struct transition_table : boost::mpl::vector<
          //       Start                              Event                         Next                              Action                   Guard
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::disabling_ports              , bmf::none                   , tg::awaiting_port_disabled_evt  , bmf::none              , bmf::none                  >,
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::awaiting_port_disabled_evt   , tg::omx_port_disabled_evt   , tg::config2idle                 , bmf::ActionSequence_<
                                                                                                                          boost::mpl::vector<
                                                                                                                            tg::do_configure_comp<0>,
                                                                                                                            tg::do_omx_loaded2idle_comp<0> > > , bmf::none   >,
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::config2idle                  , tg::omx_trans_evt           , tg::idle2exe                    , tg::do_omx_idle2exe_comp<0> , tg::is_trans_complete  >,
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::idle2exe                     , tg::omx_trans_evt           , tg::executing                   , bmf::none              , tg::is_trans_complete      >,
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::executing                    , tg::omx_port_settings_evt   , tg::awaiting_format_detected_evt, bmf::none              , bmf::none                  >,
          bmf::Row < tg::executing                    , tg::omx_format_detected_evt , tg::awaiting_port_settings_evt  , bmf::none              , bmf::none                  >,
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::awaiting_format_detected_evt , tg::omx_format_detected_evt , auto_detecting_exit             , bmf::none              , bmf::none                  >,
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::awaiting_port_settings_evt   , tg::omx_port_settings_evt   , auto_detecting_exit             , bmf::none              , bmf::none                  >
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }

      };
      // typedef boost::msm::back::state_machine<auto_detecting_, boost::msm::back::mpl_graph_fsm_check> auto_detecting;
      typedef boost::msm::back::state_machine<auto_detecting_> auto_detecting;

      /* 'updating_graph' is a submachine */
      struct updating_graph_ : public boost::msm::front::state_machine_def<updating_graph_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;

        // data members
        ops ** pp_ops_;

        updating_graph_()
          :
          pp_ops_(NULL)
        {}
        updating_graph_(ops **pp_ops)
          :
          pp_ops_(pp_ops)
        {
          assert (pp_ops);
        }

        // submachine states
        struct updating_graph_initial : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        struct updating_graph_exit : public boost::msm::front::exit_pseudo_state<tg::graph_updated_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef updating_graph_initial initial_state;

        // transition actions

        // guard conditions

        // Transition table for updating_graph
        struct transition_table : boost::mpl::vector<
          //       Start                            Event                         Next                              Action                          Guard
          //    +--+--------------------------------+---------------------------+---------------------------------+-------------------------------+--------------------------------+
          bmf::Row < updating_graph_initial         , bmf::none                 , tg::awaiting_port_disabled_evt  , bmf::ActionSequence_<
                                                                                                                      boost::mpl::vector<
                                                                                                                        tg::do_load,
                                                                                                                        tg::do_configure,
                                                                                                                        tg::do_setup,
                                                                                                                        tg::do_disable_tunnel<0> > > , bmf::none                      >,
          //    +--+--------------------------------+---------------------------+---------------------------------+-------------------------------+--------------------------------+
          bmf::Row < tg::awaiting_port_disabled_evt , tg::omx_port_disabled_evt , tg::config2idle                 , tg::do_omx_loaded2idle        , tg::is_port_disabling_complete >,
          //    +--+--------------------------------+---------------------------+---------------------------------+-------------------------------+--------------------------------+
          bmf::Row < tg::config2idle                , tg::omx_trans_evt         , tg::idle2exe                    , tg::do_omx_idle2exe           , tg::is_trans_complete          >,
          //    +--+--------------------------------+---------------------------+---------------------------------+-------------------------------+--------------------------------+
          bmf::Row < tg::idle2exe                   , tg::omx_trans_evt         , tg::enabling_tunnel             , tg::do_enable_tunnel<0>       , tg::is_trans_complete          >,
          //    +--+--------------------------------+---------------------------+---------------------------------+-------------------------------+--------------------------------+
          bmf::Row < tg::enabling_tunnel            , tg::omx_port_enabled_evt  , updating_graph_exit             , bmf::none                     , tg::is_port_enabling_complete  >
          //    +--+--------------------------------+---------------------------+---------------------------------+-------------------------------+--------------------------------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }

      };
      // typedef boost::msm::back::state_machine<updating_graph_, boost::msm::back::mpl_graph_fsm_check> updating_graph;
      typedef boost::msm::back::state_machine<updating_graph_> updating_graph;

      /* 'reconfiguring_graph' is a submachine */
      struct reconfiguring_graph_ : public boost::msm::front::state_machine_def<reconfiguring_graph_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;

        // data members
        ops ** pp_ops_;

        reconfiguring_graph_()
          :
          pp_ops_(NULL)
        {}
        reconfiguring_graph_(ops **pp_ops)
          :
          pp_ops_(pp_ops)
        {
          assert (pp_ops);
        }

        // submachine states
        struct reconfiguring_graph_initial : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        struct reconfiguring_graph_exit : public boost::msm::front::exit_pseudo_state<tg::graph_reconfigured_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef reconfiguring_graph_initial initial_state;

        // transition actions

        // guard conditions

        // Transition table for reconfiguring_graph
        struct transition_table : boost::mpl::vector<
          //       Start                            Event                         Next                              Action                           Guard
          //    +--+--------------------------------+---------------------------+---------------------------------+----------------------------------+--------------------------------+
          bmf::Row < reconfiguring_graph_initial    , bmf::none                 , tg::awaiting_port_disabled_evt  , tg::do_disable_tunnel<1>         , bmf::none                      >,
          //    +--+--------------------------------+---------------------------+---------------------------------+----------------------------------+--------------------------------+
          bmf::Row < tg::awaiting_port_disabled_evt , tg::omx_port_disabled_evt , tg::enabling_tunnel             , bmf::ActionSequence_<
                                                                                                                      boost::mpl::vector<
                                                                                                                        tg::do_reconfigure_tunnel<1>,
                                                                                                                        tg::do_enable_tunnel<1> > >  , tg::is_port_disabling_complete >,
          //    +--+--------------------------------+---------------------------+---------------------------------+----------------------------------+--------------------------------+
          bmf::Row < tg::enabling_tunnel            , tg::omx_port_enabled_evt  , reconfiguring_graph_exit        , bmf::none                        , tg::is_port_enabling_complete  >
          //    +--+--------------------------------+---------------------------+---------------------------------+----------------------------------+--------------------------------+
          > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state %d on event %s",
                   state, typeid(e).name());
        }

      };
      // typedef boost::msm::back::state_machine<reconfiguring_graph_, boost::msm::back::mpl_graph_fsm_check> reconfiguring_graph;
      typedef boost::msm::back::state_machine<reconfiguring_graph_> reconfiguring_graph;

      // The initial state of the SM. Must be defined
      typedef boost::mpl::vector<tg::inited, tg::AllOk> initial_state;

      // transition actions

      // guard conditions

      // Transition table for the http client graph fsm
      struct transition_table : boost::mpl::vector<
        //       Start                          Event                       Next                      Action                        Guard
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::inited                   , tg::load_evt              , tg::loaded              , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_load_comp<0>,
                                                                                                            tg::do_ack_loaded> >                                   >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::loaded                   , tg::execute_evt           , auto_detecting          , boost::msm::front::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_store_config,
                                                                                                            tg::do_enable_auto_detection<0,0> > > , tg::last_op_succeeded    >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < auto_detecting               , tg::unload_evt            , tg::exe2idle            , tg::do_omx_exe2idle                                        >,
        bmf::Row < auto_detecting
                   ::exit_pt
                   <auto_detecting_
                    ::auto_detecting_exit>      , tg::auto_detected_evt     , updating_graph          , bmf::none                                                  >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < updating_graph
                   ::exit_pt
                   <updating_graph_
                    ::updating_graph_exit>      , tg::graph_updated_evt     , tg::executing           , tg::do_ack_execd                                           >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::executing                , tg::omx_err_evt           , tg::exe2idle            , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_record_fatal_error,
                                                                                                            tg::do_omx_exe2idle> >                                 >,
        bmf::Row < tg::executing                , tg::unload_evt            , tg::exe2idle            , tg::do_omx_exe2idle                                        >,
        bmf::Row < tg::executing                , tg::omx_port_settings_evt , reconfiguring_graph     , tg::do_mute                                                >,
        bmf::Row < tg::executing                , tg::volume_step_evt       , bmf::none               , tg::do_volume_step                                         >,
        bmf::Row < tg::executing                , tg::volume_evt            , bmf::none               , tg::do_volume                                              >,
        bmf::Row < tg::executing                , tg::mute_evt              , bmf::none               , tg::do_mute                                                >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < reconfiguring_graph
                   ::exit_pt
                   <reconfiguring_graph_
                    ::reconfiguring_graph_exit> , tg::graph_reconfigured_evt, tg::executing           , tg::do_mute                                                >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::exe2idle                 , tg::omx_trans_evt         , tg::idle2loaded         , tg::do_omx_idle2loaded      , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::idle2loaded              , tg::omx_trans_evt         , tg::unloaded            , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_tear_down_tunnels,
                                                                                                            tg::do_destroy_graph> > , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::AllOk                    , tg::err_evt               , tg::unloaded            , tg::do_error                                               >
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        > {};

      // Replaces the default no-transition response.
      template <class FSM,class Event>
      void no_transition(Event const& e, FSM&,int state)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state [%s] on event [%s]",
                 tg::hcfsm::state_names[state], typeid(e).name());
      }
    };
    // typedef boost::msm::back::state_machine<fsm_, boost::msm::back::mpl_graph_fsm_check> fsm;
    typedef boost::msm::back::state_machine<fsm_> fsm;

    char const* const pstate(fsm const& p);

    } // namespace hcfsm
  } // namespace graph
} // namespace tiz

#endif // TIZHTTPCLNTGRAPHFSM_HPP

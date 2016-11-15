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
 * @file   tizyoutubegraphfsm.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Youtube streaming client graph fsm
 *
 */

#ifndef TIZYOUTUBEGRAPHFSM_HPP
#define TIZYOUTUBEGRAPHFSM_HPP

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
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.youtube.fsm"
#endif

#define YOUTUBE_FSM_LOG()                                        \
  do                                                             \
  {                                                              \
    TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid(*this).name ()); \
  } while (0)

namespace tg = tiz::graph;
namespace bmf = boost::msm::front;

namespace tiz
{
  namespace graph
  {
    namespace youtubefsm
    {
      static char const* const state_names[] = { "inited",
                                                 "loaded",
                                                 "auto_detecting",
                                                 "updating_graph",
                                                 "executing",
                                                 "exe2pause",
                                                 "pause",
                                                 "pause2exe",
                                                 "reconfiguring_tunnel_0",
                                                 "reconfiguring_tunnel_1",
                                                 "skipping",
                                                 "exe2idle",
                                                 "idle2loaded",
                                                 "AllOk",
                                                 "unloaded"};

    // Concrete FSM implementation
    struct fsm_ : public boost::msm::front::state_machine_def<fsm_>
    {
      // no need for exception handling
      typedef int no_exception_thrown;
      // require deferred events capability
      // typedef int activate_deferred_events;

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
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
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
                                                                                                                            tg::do_configure_source,
                                                                                                                            tg::do_source_omx_loaded2idle > > , tg::is_port_disabling_complete   >,
          bmf::Row < tg::awaiting_port_disabled_evt   , tg::omx_port_disabled_evt   , tg::config2idle                    , bmf::ActionSequence_<
                                                                                                                          boost::mpl::vector<
                                                                                                                            tg::do_enable_auto_detection<0,0>,
                                                                                                                            tg::do_omx_exe2idle > > , bmf::euml::And_<
                                                                                                                                                        tg::is_component_state<0, OMX_StateExecuting>,
                                                                                                                                                        tg::is_port_disabling_complete> >,
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::config2idle                  , tg::omx_trans_evt           , tg::idle2exe                    , tg::do_source_omx_idle2exe , tg::is_trans_complete  >,
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::idle2exe                     , tg::omx_trans_evt           , tg::executing                   , bmf::none              , tg::is_trans_complete      >,
          //    +--+----------------------------------+-----------------------------+---------------------------------+------------------------+----------------------------+
          bmf::Row < tg::executing                    , tg::omx_port_settings_evt   , tg::awaiting_format_detected_evt, bmf::none              , bmf::none                  >,
          bmf::Row < tg::executing                    , tg::omx_format_detected_evt , tg::awaiting_port_settings_evt  , bmf::none              , bmf::none                  >,
          bmf::Row < tg::executing                    , tg::omx_err_evt             , bmf::none                       , tg::do_skip            , tg::is_error<OMX_ErrorFormatNotDetected> >,
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
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
        };

        struct updating_graph_exit : public boost::msm::front::exit_pseudo_state<tg::graph_updated_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
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

      /* 'reconfiguring_tunnel_' is a submachine */
      template<int tunnel_id>
      struct reconfiguring_tunnel_ : public boost::msm::front::state_machine_def<reconfiguring_tunnel_< tunnel_id > >
      {
        // no need for exception handling
        typedef int no_exception_thrown;
        // require deferred events capability
        typedef int activate_deferred_events;

        // data members
        ops ** pp_ops_;

        reconfiguring_tunnel_()
          :
          pp_ops_(NULL)
        {}
        reconfiguring_tunnel_(ops **pp_ops)
          :
          pp_ops_(pp_ops)
        {
          assert (pp_ops);
        }

        // submachine states
        struct reconfiguring_tunnel_initial : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
        };

        struct reconfiguring_tunnel_exit : public boost::msm::front::exit_pseudo_state<tg::tunnel_reconfigured_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef reconfiguring_tunnel_initial initial_state;

        // transition actions

        // guard conditions

        // Transition table for reconfiguring_tunnel_
        struct transition_table : boost::mpl::vector<
          //       Start                            Event                         Next                              Action                           Guard
          //    +--+--------------------------------+---------------------------+---------------------------------+----------------------------------+--------------------------------+
          bmf::Row < reconfiguring_tunnel_initial   , bmf::none                 , tg::awaiting_port_disabled_evt  , tg::do_disable_tunnel<tunnel_id>         , bmf::none                      >,
          //    +--+--------------------------------+---------------------------+---------------------------------+----------------------------------+--------------------------------+
          bmf::Row < tg::awaiting_port_disabled_evt , tg::omx_port_disabled_evt , tg::enabling_tunnel             , bmf::ActionSequence_<
                                                                                                                      boost::mpl::vector<
                                                                                                                        tg::do_reconfigure_tunnel<tunnel_id>,
                                                                                                                        tg::do_enable_tunnel<tunnel_id> > >  , tg::is_port_disabling_complete >,
          bmf::Row < tg::awaiting_port_disabled_evt , tg::skip_evt              , bmf::none                       , bmf::Defer                                                                >,
          //    +--+--------------------------------+---------------------------+---------------------------------+----------------------------------+--------------------------------+
          bmf::Row < tg::enabling_tunnel            , tg::omx_port_enabled_evt  , reconfiguring_tunnel_exit       , bmf::none                        , tg::is_port_enabling_complete  >
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
      // typedef boost::msm::back::state_machine<reconfiguring_tunnel_, boost::msm::back::mpl_graph_fsm_check> reconfiguring_tunnel_0;
      typedef boost::msm::back::state_machine<reconfiguring_tunnel_<0> > reconfiguring_tunnel_0;
      typedef boost::msm::back::state_machine<reconfiguring_tunnel_<1> > reconfiguring_tunnel_1;

      /* 'skipping' is a submachine of tiz::graph::fsm_ */
      struct skipping_ : public boost::msm::front::state_machine_def<skipping_>
      {
        // no need for exception handling
        typedef int no_exception_thrown;
        // require deferred events capability
        typedef int activate_deferred_events;

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
        struct skipping_initial : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
        };

        struct to_idle : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
          OMX_STATETYPE target_omx_state () const
          {
            return OMX_StateIdle;
          }
        };

        struct skip_exit : public boost::msm::front::exit_pseudo_state<tiz::graph::skipped_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
        };

        struct disabling_2nd_tunnel : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
        };

        struct enabling_2nd_tunnel : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {YOUTUBE_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef skipping_initial initial_state;

        // transition actions

        // guard conditions

        // Transition table for skipping
        struct transition_table : boost::mpl::vector<
          //         Start                 Event                       Next                      Action                      Guard
          //    +----+---------------------+---------------------------+-------------------------+---------------------------+---------------------------------+
          bmf::Row < skipping_initial      , bmf::none                 , tg::disabling_tunnel    , bmf::ActionSequence_<
                                                                                                     boost::mpl::vector<
                                                                                                       tg::do_mute,
                                                                                                       tg::do_disable_tunnel<0> > >                           >,
          bmf::Row < tg::disabling_tunnel  , tg::omx_port_disabled_evt , disabling_2nd_tunnel    , tg::do_disable_tunnel<1>  , tg::is_port_disabling_complete >,
          bmf::Row < tg::disabling_tunnel  , tg::skip_evt              , bmf::none               , bmf::Defer                                                 >,
          bmf::Row < disabling_2nd_tunnel  , tg::skip_evt              , bmf::none               , bmf::Defer                                                 >,
          bmf::Row < disabling_2nd_tunnel  , tg::omx_port_disabled_evt , skip_exit               , bmf::ActionSequence_<
                                                                                                     boost::mpl::vector<
                                                                                                       tg::do_mute,
                                                                                                       tg::do_tear_down_tunnels,
                                                                                                       tg::do_destroy_component<1>,
                                                                                                       tg::do_destroy_component<1>,
                                                                                                       tg::do_skip > >       , tg::is_port_disabling_complete >
          //    +----+---------------------+---------------------------+-------------------------+---------------------------+---------------------------------+
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
      typedef boost::mpl::vector<tg::inited, tg::AllOk> initial_state;

      // transition actions

      // guard conditions

      // Transition table for the youtube client graph fsm
      struct transition_table : boost::mpl::vector<
        //       Start                          Event                       Next                      Action                        Guard
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::inited                   , tg::load_evt              , tg::loaded              , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_load_source,
                                                                                                            tg::do_ack_loaded> >                                   >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::loaded                   , tg::execute_evt           , auto_detecting          , boost::msm::front::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_store_config,
                                                                                                            tg::do_enable_auto_detection<0,0> > > , tg::last_op_succeeded    >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < auto_detecting               , tg::omx_err_evt           , tg::exe2idle            , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_record_fatal_error,
                                                                                                            tg::do_omx_exe2idle> >                                 >,
        bmf::Row < auto_detecting               , tg::unload_evt            , tg::exe2idle            , tg::do_omx_exe2idle                                        >,
        bmf::Row < auto_detecting
                   ::exit_pt
                   <auto_detecting_
                    ::auto_detecting_exit>      , tg::auto_detected_evt     , updating_graph          , bmf::none                                                  >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < updating_graph
                   ::exit_pt
                   <updating_graph_
                    ::updating_graph_exit>      , tg::graph_updated_evt     , tg::executing           , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_retrieve_metadata,
                                                                                                            tg::do_ack_execd> >                                    >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::executing                , tg::omx_err_evt           , tg::exe2idle            , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_record_fatal_error,
                                                                                                            tg::do_omx_exe2idle> >                                 >,
        bmf::Row < tg::executing                , tg::unload_evt            , tg::exe2idle            , tg::do_omx_exe2idle                                        >,
        bmf::Row < tg::executing                , tg::omx_port_settings_evt , reconfiguring_tunnel_0  , tg::do_mute                                                >,
        bmf::Row < tg::executing                , tg::omx_port_settings_evt , reconfiguring_tunnel_1  , tg::do_mute                 , tg::is_tunnel_altered<1>     >,
        bmf::Row < tg::executing                , tg::pause_evt             , tg::exe2pause           , tg::do_omx_exe2pause                                       >,
        bmf::Row < tg::executing                , tg::volume_step_evt       , bmf::none               , tg::do_volume_step                                         >,
        bmf::Row < tg::executing                , tg::volume_evt            , bmf::none               , tg::do_volume                                              >,
        bmf::Row < tg::executing                , tg::mute_evt              , bmf::none               , tg::do_mute                                                >,
        bmf::Row < tg::executing                , tg::skip_evt              , skipping                , tg::do_store_skip                                          >,
        bmf::Row < tg::executing                , tg::omx_eos_evt           , bmf::none               , tg::do_retrieve_metadata    , tg::is_last_eos              >,
        bmf::Row < tg::executing                , tg::omx_eos_evt           , bmf::none               , tg::do_skip                 , tg::is_first_eos             >,
        bmf::Row < tg::executing                , tg::omx_err_evt           , skipping                , bmf::none                   , tg::is_error<OMX_ErrorStreamCorruptFatal> >,
        bmf::Row < tg::executing                , tg::omx_err_evt           , skipping                , bmf::none                   , tg::is_error<OMX_ErrorFormatNotDetected> >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::exe2pause                , tg::omx_trans_evt         , tg::pause               , tg::do_ack_paused           , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::pause                    , tg::execute_evt           , tg::pause2exe           , tg::do_omx_pause2exe                                       >,
        bmf::Row < tg::pause                    , tg::pause_evt             , tg::pause2exe           , tg::do_omx_pause2exe                                       >,
        bmf::Row < tg::pause                    , tg::stop_evt              , tg::pause2idle          , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_record_destination < OMX_StateIdle >,
                                                                                                            tg::do_omx_pause2idle > >                              >,
        bmf::Row < tg::pause                    , tg::unload_evt            , tg::pause2idle          , tg::do_omx_pause2idle                                      >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::pause2exe                , tg::omx_trans_evt         , tg::executing           , tg::do_ack_unpaused         , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < reconfiguring_tunnel_0
                   ::exit_pt
                   <reconfiguring_tunnel_<0>
                    ::reconfiguring_tunnel_exit> , tg::tunnel_reconfigured_evt, tg::executing           , tg::do_mute                                                >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < reconfiguring_tunnel_1
                   ::exit_pt
                   <reconfiguring_tunnel_<1>
                    ::reconfiguring_tunnel_exit> , tg::tunnel_reconfigured_evt, tg::executing           , tg::do_mute                                                >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < skipping
                   ::exit_pt
                   <skipping_
                    ::skip_exit>                , tg::skipped_evt           , tg::unloaded            , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_error,
                                                                                                            tg::do_tear_down_tunnels,
                                                                                                            tg::do_destroy_graph> >     , tg::is_internal_error    >,
        bmf::Row < skipping
                   ::exit_pt
                   <skipping_
                    ::skip_exit>                , tg::skipped_evt           , tg::unloaded            , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_end_of_play,
                                                                                                            tg::do_tear_down_tunnels,
                                                                                                            tg::do_destroy_graph> >     , tg::is_end_of_play       >,
        bmf::Row < skipping
                   ::exit_pt
                   <skipping_
                    ::skip_exit>                , tg::skipped_evt           , auto_detecting          , bmf::none                   , bmf::euml::Not_<
                                                                                                                                        tg::is_end_of_play >       >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::exe2idle                 , tg::omx_err_evt           , bmf::none               , bmf::none                   , tg::is_error<OMX_ErrorStreamCorruptFatal> >,
        bmf::Row < tg::exe2idle                 , tg::omx_trans_evt         , tg::idle2loaded         , tg::do_omx_idle2loaded      , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::idle2loaded              , tg::omx_trans_evt         , tg::unloaded            , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_tear_down_tunnels,
                                                                                                            tg::do_destroy_graph> > , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::AllOk                    , tg::unload_evt            , tg::unloaded            , bmf::none                                                  >,
        bmf::Row < tg::AllOk                    , tg::omx_err_evt           , bmf::none               , bmf::none                   , bmf::euml::Not_<
                                                                                                                                        tg::is_fatal_error >       >,
        bmf::Row < tg::AllOk                    , tg::omx_err_evt           , tg::unloaded            , boost::msm::front::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_record_fatal_error,
                                                                                                            tg::do_error,
                                                                                                            tg::do_tear_down_tunnels,
                                                                                                            tg::do_destroy_graph> > , tg::is_fatal_error           >,
        bmf::Row < tg::AllOk                    , tg::err_evt               , tg::unloaded            , tg::do_error                                               >
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        > {};

      // Replaces the default no-transition response.
      template <class FSM,class Event>
      void no_transition(Event const& e, FSM&,int state)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state [%s] on event [%s]",
                 tg::youtubefsm::state_names[state], typeid(e).name());
      }
    };
    // typedef boost::msm::back::state_machine<fsm_, boost::msm::back::mpl_graph_fsm_check> fsm;
    typedef boost::msm::back::state_machine<fsm_> fsm;

    char const* const pstate(fsm const& p);

    } // namespace youtubefsm
  } // namespace graph
} // namespace tiz

#endif // TIZYOUTUBEGRAPHFSM_HPP

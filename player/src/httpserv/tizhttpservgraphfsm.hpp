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
 * @file   tizhttpservgraphfsm.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  HTTP server graph's fsm variations
 *
 */

#ifndef TIZHTTPSERVGRAPHFSM_HPP
#define TIZHTTPSERVGRAPHFSM_HPP

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 40
#define FUSION_MAX_VECTOR_SIZE      20
#define SPIRIT_ARGUMENTS_LIMIT      20

#include <sys/time.h>

#include <boost/msm/back/state_machine.hpp>
//#include <boost/msm/back/mpl_graph_fsm_check.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/back/tools.hpp>

#include <tizplatform.h>

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

namespace tg = tiz::graph;
namespace bmf = boost::msm::front;

namespace tiz
{
  namespace graph
  {
    namespace hsfsm
    {

      static char const* const state_names[] = { "inited",
                                                 "loaded",
                                                 "configuring",
                                                 "executing",
                                                 "skipping",
                                                 "exe2idle",
                                                 "idle2loaded",
                                                 "AllOk",
                                                 "unloaded"};

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
          //        Start                Event                      Next                  Action                                  Guard
          //    +---+--------------------+--------------------------+---------------------+---------------------------------------+----------------------------------------------------+
          bmf::Row < configuring_server  , bmf::none                , probing             , bmf::ActionSequence_<
                                                                                              boost::mpl::vector<
                                                                                                do_configure_server,
                                                                                                do_configure_station,
                                                                                                tg::do_probe > >                  , is_initial_configuration                          >,
          bmf::Row < configuring_server  , bmf::none                , probing             , tg::do_probe                          , bmf::euml::Not_< is_initial_configuration >       >,
          //    +---+--------------------+--------------------------+---------------------+---------------------------------------+----------------------------------------------------+
          bmf::Row < probing             , bmf::none                , tg::config2idle     , bmf::ActionSequence_<
                                                                                              boost::mpl::vector<
                                                                                                do_configure_stream,
                                                                                                tg::do_omx_loaded2idle > >        , is_initial_configuration                          >,
          bmf::Row < probing             , bmf::none                , tg::config2idle     , bmf::ActionSequence_<
                                                                                              boost::mpl::vector<
                                                                                                do_configure_stream,
                                                                                                tg::do_source_omx_loaded2idle > > , bmf::euml::Not_< is_initial_configuration >       >,
          bmf::Row < probing             , bmf::none                , conf_exit           , bmf::none                             , tg::is_end_of_play                                >,
          bmf::Row < probing             , bmf::none                , probing             , bmf::ActionSequence_<
                                                                                              boost::mpl::vector<
                                                                                                tg::do_reset_internal_error,
                                                                                                tg::do_skip,
                                                                                                tg::do_probe > >                  , bmf::euml::And_<
                                                                                                                                      bmf::euml::Not_< tg::is_end_of_play >,
                                                                                                                                      bmf::euml::Not_< tg::is_probing_result_ok > >  >,
          //    +---+--------------------+--------------------------+---------------------+---------------------------------------+----------------------------------------------------+
          bmf::Row < tg::config2idle     , tg::omx_trans_evt        , tg::idle2exe        , tg::do_omx_idle2exe                   , bmf::euml::And_<
                                                                                                                                      is_initial_configuration,
                                                                                                                                      tg::is_trans_complete >                         >,
          bmf::Row < tg::config2idle     , tg::omx_trans_evt        , tg::idle2exe        , do_source_omx_idle2exe                , bmf::euml::And_<
                                                                                                                                      bmf::euml::Not_< is_initial_configuration >,
                                                                                                                                      tg::is_trans_complete >                         >,
          //    +---+--------------------+--------------------------+---------------------+---------------------------------------+----------------------------------------------------+
          bmf::Row < tg::idle2exe        , tg::omx_trans_evt        , conf_exit           , do_flag_initial_config_done           , bmf::euml::And_<
                                                                                                                                      is_initial_configuration,
                                                                                                                                      tg::is_trans_complete >                         >,
          bmf::Row < tg::idle2exe        , tg::omx_trans_evt        , tg::enabling_tunnel , tg::do_enable_tunnel<0>               , bmf::euml::And_<
                                                                                                                                      bmf::euml::Not_< is_initial_configuration>,
                                                                                                                                      tg::is_trans_complete >                         >,
          //    +---+--------------------+--------------------------+---------------------+---------------------------------------+----------------------------------------------------+
          bmf::Row < tg::enabling_tunnel , tg::omx_port_enabled_evt , conf_exit           , bmf::none                             , tg::is_port_enabling_complete                     >
          //    +---+--------------------+--------------------------+---------------------+---------------------------------------+----------------------------------------------------+
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
          assert (pp_ops);
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

        struct skip_exit : public boost::msm::front::exit_pseudo_state<tiz::graph::skipped_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        // the initial state. Must be defined
        typedef skipping_initial initial_state;

        // transition actions

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
          //         Start                 Event                       Next                      Action                      Guard
          //    +----+---------------------+---------------------------+-------------------------+---------------------------+---------------------------------+
          bmf::Row < skipping_initial      , bmf::none                 , tg::disabling_tunnel    , tg::do_disable_tunnel<0>                                      >,
          bmf::Row < tg::disabling_tunnel  , tg::omx_port_disabled_evt , to_idle                 , do_source_omx_exe2idle    , tg::is_port_disabling_complete >,
          bmf::Row < to_idle               , tg::omx_trans_evt         , tg::idle2loaded         , do_source_omx_idle2loaded , tg::is_trans_complete          >,
          bmf::Row < tg::idle2loaded       , tg::omx_trans_evt         , skip_exit               , tg::do_skip               , tg::is_trans_complete          >
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
      typedef boost::mpl::vector<tiz::graph::inited, tiz::graph::AllOk> initial_state;

      // transition actions


      // guard conditions

      // Transition table for the httpserver graph fsm
      struct transition_table : boost::mpl::vector<
        //        Start            Event                 Next              Action                            Guard
        //    +---+----------------+---------------------+-----------------+---------------------------------+--------------------------+
        bmf::Row < tg::inited      , tg::load_evt        , tg::loaded      , bmf::ActionSequence_<
                                                                               boost::mpl::vector<
                                                                                 tg::do_load,
                                                                                 tg::do_setup,
                                                                                 tg::do_ack_loaded> >                                  >,
        //    +---+----------------+---------------------+-----------------+---------------------------------+--------------------------+
        bmf::Row < tg::loaded      , tg::execute_evt     , configuring     , tg::do_store_config             , tg::last_op_succeeded   >,
        //    +---+----------------+---------------------+-----------------+---------------------------------+--------------------------+
        bmf::Row < configuring     , tg::omx_err_evt     , tg::unloaded    , bmf::ActionSequence_<
                                                                               boost::mpl::vector<
                                                                                 tg::do_record_fatal_error,
                                                                                 tg::do_error,
                                                                                 tg::do_tear_down_tunnels,
                                                                                 tg::do_destroy_graph> >     , tg::is_fatal_error      >,
        bmf::Row < configuring
                   ::exit_pt
                   <configuring_
                    ::conf_exit>   , tg::configured_evt  , tg::executing   , tg::do_ack_execd                                          >,
        bmf::Row < configuring
                   ::exit_pt
                   <configuring_
                    ::conf_exit>   , tg::configured_evt  , tg::unloaded    , bmf::ActionSequence_<
                                                                               boost::mpl::vector<
                                                                                 tg::do_end_of_play,
                                                                                 tg::do_tear_down_tunnels,
                                                                                 tg::do_destroy_graph> >     , tg::is_end_of_play      >,
        //    +---+----------------+---------------------+-----------------+---------------------------------+--------------------------+
        bmf::Row < tg::executing   , tg::skip_evt        , skipping        , tg::do_store_skip                                         >,
        bmf::Row < tg::executing   , tg::unload_evt      , tg::exe2idle    , tg::do_omx_exe2idle                                       >,
        bmf::Row < tg::executing   , tg::omx_err_evt     , skipping        , bmf::none                                                 >,
        bmf::Row < tg::executing   , tg::omx_err_evt     , skipping        , tg::do_record_fatal_error       , tg::is_fatal_error      >,
        bmf::Row < tg::executing   , tg::omx_eos_evt     , skipping        , bmf::none                       , tg::is_last_eos         >,
        //    +---+----------------+---------------------+-----------------+---------------------------------+--------------------------+
        bmf::Row < skipping
                   ::exit_pt
                   <skipping_
                    ::skip_exit>   , skipped_evt         , tg::unloaded    , bmf::ActionSequence_<
                                                                               boost::mpl::vector<
                                                                                 tg::do_error,
                                                                                 tg::do_tear_down_tunnels,
                                                                                 tg::do_destroy_graph> >     , tg::is_internal_error    >,
        bmf::Row < skipping
                   ::exit_pt
                   <skipping_
                    ::skip_exit>   , skipped_evt         , tg::unloaded    , bmf::ActionSequence_<
                                                                               boost::mpl::vector<
                                                                                 tg::do_end_of_play,
                                                                                 tg::do_tear_down_tunnels,
                                                                                 tg::do_destroy_graph> >     , tg::is_end_of_play       >,
        bmf::Row < skipping
                   ::exit_pt
                   <skipping_
                    ::skip_exit>   , skipped_evt         , configuring     , bmf::none                       , bmf::euml::Not_<
                                                                                                                 tg::is_end_of_play >   >,
        //    +---+----------------+---------------------+-----------------+---------------------------------+--------------------------+
        bmf::Row < tg::exe2idle    , tg::omx_trans_evt   , tg::idle2loaded , tg::do_omx_idle2loaded          , tg::is_trans_complete    >,
        //    +---+----------------+---------------------+-----------------+---------------------------------+--------------------------+
        bmf::Row < tg::idle2loaded , tg::omx_trans_evt   , tg::unloaded    , bmf::ActionSequence_<
                                                                               boost::mpl::vector<
                                                                                 tg::do_tear_down_tunnels,
                                                                                 tg::do_destroy_graph> >     , tg::is_trans_complete    >,
        //    +---+----------------+---------------------+-----------------+---------------------------------+--------------------------+
        bmf::Row < tg::AllOk       , tg::err_evt         , tg::unloaded    , tg::do_error                                               >
        //    +---+----------------+---------------------+-----------------+---------------------------------+--------------------------+
        > {};

      // Replaces the default no-transition response.
      template <class FSM,class Event>
      void no_transition(Event const& e, FSM&,int state)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state [%s] on event [%s]",
                 tiz::graph::hsfsm::state_names[state], typeid(e).name());
      }
    };
    // typedef boost::msm::back::state_machine<fsm_, boost::msm::back::mpl_graph_fsm_check> fsm;
    typedef boost::msm::back::state_machine<fsm_> fsm;

    char const* const pstate(fsm const& p);

    } // namespace hsfsm
  } // namespace graph
} // namespace tiz

#endif // TIZHTTPSERVGRAPHFSM_HPP

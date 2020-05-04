/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tizchromecastgraphfsm.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Chromecast renderer graph fsm
 *
 */

#ifndef TIZCHROMECASTGRAPHFSM_HPP
#define TIZCHROMECASTGRAPHFSM_HPP

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
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.chromecast.fsm"
#endif

#define CHROMECAST_FSM_LOG()                                        \
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
    namespace ccfsm
    {
      static char const* const state_names[] = { "inited",
                                                 "loaded",
                                                 "awaiting_port_disabled_evt",
                                                 "config2idle",
                                                 "idle2exe",
                                                 "executing",
                                                 "exe2pause",
                                                 "pause",
                                                 "pause2exe",
                                                 "pause2idle",
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

      // The initial state of the SM. Must be defined
      typedef boost::mpl::vector<tg::inited, tg::AllOk> initial_state;

      // transition actions

      // guard conditions

      // Transition table for the chromecast client graph fsm
      struct transition_table : boost::mpl::vector<
        //       Start                          Event                       Next                      Action                        Guard
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::inited                   , tg::load_evt              , tg::loaded              , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_store_config,
                                                                                                            tg::do_load,
                                                                                                            tg::do_configure,
                                                                                                            tg::do_ack_loaded> >    , bmf::none                    >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::loaded                   , tg::execute_evt           , tg::awaiting_port_disabled_evt, tg::do_disable_comp_ports<0,0>, bmf::none            >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::awaiting_port_disabled_evt, tg::omx_port_disabled_evt, tg::config2idle         , tg::do_loaded2idle        , tg::is_port_disabling_complete >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::config2idle              , tg::omx_trans_evt         , tg::idle2exe            , tg::do_idle2exe             , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::idle2exe                 , tg::omx_trans_evt         , tg::executing           , tg::do_ack_execd            , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::executing                , tg::omx_err_evt           , tg::exe2idle            , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_record_fatal_error,
                                                                                                            tg::do_exe2idle> >                                     >,
        bmf::Row < tg::executing                , tg::unload_evt            , tg::exe2idle            , tg::do_exe2idle                                            >,
        bmf::Row < tg::executing                , tg::pause_evt             , tg::exe2pause           , tg::do_exe2pause                                           >,
        bmf::Row < tg::executing                , tg::volume_step_evt       , bmf::none               , tg::do_volume_step                                         >,
        bmf::Row < tg::executing                , tg::volume_evt            , bmf::none               , tg::do_volume                                              >,
        bmf::Row < tg::executing                , tg::mute_evt              , bmf::none               , tg::do_mute                                                >,
        bmf::Row < tg::executing                , tg::omx_err_evt           , bmf::none               , tg::do_skip                 , tg::is_error<OMX_ErrorStreamCorruptFatal> >,
        bmf::Row < tg::executing                , tg::omx_err_evt           , bmf::none               , tg::do_skip                 , tg::is_error<OMX_ErrorFormatNotDetected> >,
        bmf::Row < tg::executing                , tg::position_evt          , bmf::none               , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_store_position,
                                                                                                            tg::do_skip>
                                                                                                          >                         , bmf::none                    >,
        bmf::Row < tg::executing                , tg::skip_evt              , bmf::none               , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_store_skip,
                                                                                                            tg::do_skip>
                                                                                                          >                         , bmf::none                    >,
        bmf::Row < tg::executing                , tg::omx_eos_evt           , bmf::none               , tg::do_skip                 , tg::is_last_eos              >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::exe2pause                , tg::omx_trans_evt         , tg::pause               , tg::do_ack_paused           , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::pause                    , tg::execute_evt           , tg::pause2exe           , tg::do_pause2exe                                           >,
        bmf::Row < tg::pause                    , tg::pause_evt             , tg::pause2exe           , tg::do_pause2exe                                           >,
        bmf::Row < tg::pause                    , tg::stop_evt              , tg::pause2idle          , bmf::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_record_destination < OMX_StateIdle >,
                                                                                                            tg::do_pause2idle > >                                  >,
        bmf::Row < tg::pause                    , tg::unload_evt            , tg::pause2idle          , tg::do_pause2idle                                          >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::pause2exe                , tg::omx_trans_evt         , tg::executing           , tg::do_ack_resumed         , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::pause2idle               , tg::omx_trans_evt         , tg::idle2loaded         , tg::do_idle2loaded          , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::exe2idle                 , tg::omx_err_evt           , bmf::none               , bmf::none                   , tg::is_error<OMX_ErrorStreamCorruptFatal> >,
        bmf::Row < tg::exe2idle                 , tg::omx_trans_evt         , tg::idle2loaded         , tg::do_idle2loaded          , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::idle2loaded              , tg::omx_trans_evt         , tg::unloaded            , tg::do_destroy_graph        , tg::is_trans_complete        >,
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        bmf::Row < tg::AllOk                    , tg::omx_index_setting_evt , bmf::none               , tg::do_retrieve_metadata                                   >,
        bmf::Row < tg::AllOk                    , tg::omx_err_evt           , bmf::none               , bmf::none                   , bmf::euml::Not_<
                                                                                                                                        tg::is_fatal_error >       >,
        bmf::Row < tg::AllOk                    , tg::omx_err_evt           , tg::unloaded            , boost::msm::front::ActionSequence_<
                                                                                                          boost::mpl::vector<
                                                                                                            tg::do_record_fatal_error,
                                                                                                            tg::do_error,
                                                                                                            tg::do_destroy_graph> > , tg::is_fatal_error           >,
        bmf::Row < tg::AllOk                    , tg::err_evt               , tg::unloaded            , tg::do_error                                               >
        //    +--+------------------------------+---------------------------+-------------------------+-----------------------------+------------------------------+
        > {};

      // Replaces the default no-transition response.
      template <class FSM,class Event>
      void no_transition(Event const& e, FSM&,int state)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state [%s] on event [%s]",
                 tg::ccfsm::state_names[state], typeid(e).name());
      }
    };
    // typedef boost::msm::back::state_machine<fsm_, boost::msm::back::mpl_graph_fsm_check> fsm;
    typedef boost::msm::back::state_machine<fsm_> fsm;

    char const* const pstate(fsm const& p);

    } // namespace ccfsm
  } // namespace graph
} // namespace tiz

#endif // TIZCHROMECASTGRAPHFSM_HPP

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
 * @file   tizgraphfsm.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Base graph fsm class
 *
 */

#ifndef TIZGRAPHFSM_H
#define TIZGRAPHFSM_H

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 30
#define FUSION_MAX_VECTOR_SIZE      20
#define SPIRIT_ARGUMENTS_LIMIT      20

#include <sys/time.h>

#include <iostream>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/back/mpl_graph_fsm_check.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/back/tools.hpp>

#include <tizosal.h>

#include "tizgraphops.h"

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

namespace tiz
{
  namespace graph
  {
    // Main fsm events. In order of appearance in the fsm table.
    struct load_evt {};
    struct execute_evt
    {
      execute_evt (const tizgraphconfig_ptr_t config)
        :
        config_ (config)
      {}
      const tizgraphconfig_ptr_t config_;
    };
    // Make this state convertible from any state (this exists a sub-machine)
    struct configured_evt
    {
      configured_evt(){}
      template <class Event>
      configured_evt(Event const&){}
    };
    struct omx_trans_evt
    {
      omx_trans_evt(const OMX_HANDLETYPE a_handle, const OMX_STATETYPE a_state,
                      const OMX_ERRORTYPE a_error)
        :
        handle_ (a_handle),
        state_(a_state),
        error_ (a_error)
      {}
      OMX_HANDLETYPE handle_;
      OMX_STATETYPE  state_;
      OMX_ERRORTYPE  error_;
    };
    struct skip_evt
    {
      skip_evt(const int a_jump)
        :
        jump_ (a_jump)
      {}
      int jump_;
    };
    // Make this state convertible from any state (this exists a sub-machine)
    struct skipped_evt
    {
      skipped_evt(){}
      template <class Event>
      skipped_evt(Event const&){}
    };
    struct seek_evt {};
    struct volume_evt
    {
      volume_evt(const int step)
        :
        step_ (step)
      {}
      int step_;
    };
    struct mute_evt {};
    struct pause_evt {};
    struct omx_evt
    {
      omx_evt(const OMX_HANDLETYPE a_handle,  const OMX_EVENTTYPE a_event,
                const OMX_U32 a_data1, const OMX_U32 a_data2,
                const OMX_PTR ap_eventdata)
        :
        handle_ (a_handle),
        event_ (a_event),
        data1_ (a_data1),
        data2_ (a_data2),
        p_eventdata_ (ap_eventdata)
      {}
      OMX_HANDLETYPE handle_;
      OMX_EVENTTYPE  event_;
      OMX_U32        data1_;
      OMX_U32        data2_;
      OMX_PTR        p_eventdata_;
    };
    struct omx_eos_evt
    {
      omx_eos_evt(const OMX_HANDLETYPE a_handle, const OMX_U32 port, const OMX_U32 flags)
        :
        handle_ (a_handle),
        port_ (port),
        flags_ (flags)
      {}
      OMX_HANDLETYPE handle_;
      OMX_U32 port_;
      OMX_U32 flags_;
    };
    struct unload_evt {};
    struct omx_port_disabled_evt
    {
      omx_port_disabled_evt(const OMX_HANDLETYPE a_handle, const OMX_U32 port, const OMX_ERRORTYPE error)
        :
        handle_ (a_handle),
        port_ (port),
        error_ (error)
      {}
      OMX_HANDLETYPE handle_;
      OMX_U32 port_;
      OMX_ERRORTYPE error_;
    };
    struct omx_port_settings_evt
    {
      omx_port_settings_evt(const OMX_HANDLETYPE a_handle, const OMX_U32 port, const OMX_INDEXTYPE index)
        :
        handle_ (a_handle),
        port_ (port),
        index_ (index)
      {}
      OMX_HANDLETYPE handle_;
      OMX_U32 port_;
      OMX_INDEXTYPE index_;
    };
    // NOTE: This is an error produced by an omx component
    struct omx_err_evt
    {
       omx_err_evt(const OMX_HANDLETYPE a_handle, const OMX_ERRORTYPE a_error,
                     const OMX_U32 port = OMX_ALL) // OMX_ALL here means "no port id"
        :
        handle_ (a_handle),
        error_ (a_error),
        port_ (port)
      {}
      OMX_HANDLETYPE handle_;
      OMX_ERRORTYPE error_;
      OMX_U32 port_;
    };
    // NOTE: This is an internal error (some internal operation failed)
    struct err_evt
    {
      err_evt(const OMX_ERRORTYPE error, const std::string & error_str)
        :
        error_code_ (error),
        error_str_(error_str)
      {}
      OMX_ERRORTYPE error_code_;
      std::string   error_str_;
    };

    // Concrete FSM implementation
    struct fsm_ : public boost::msm::front::state_machine_def<fsm_>
    {
      // no need for exception handling
      typedef int no_exception_thrown;
      /* Forward declarations */
      struct do_omx_idle2loaded;
      struct idle2loaded;
      struct is_trans_complete;

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
      struct inited : public boost::msm::front::state<>
      {
        // optional entry/exit methods
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());
          fsm.terminated_ = false;
        }
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
      };

      struct loaded : public boost::msm::front::state<>
      {
        // optional entry/exit methods
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
      };

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
        struct disabling_ports : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm)
          {
            G_FSM_LOG();
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                (*(fsm.pp_ops_))->do_disable_ports ();
              }
          }
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

        struct awaiting_port_disabled_evt : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        struct awaiting_port_settings_evt : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm) {G_FSM_LOG();}
          template <class Event,class FSM>
          void on_exit(Event const & evt, FSM & fsm) {G_FSM_LOG();}
        };

        struct conf_exit : public boost::msm::front::exit_pseudo_state<configured_evt>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm)
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());
          }
        };

        // the initial state. Must be defined
        typedef disabling_ports initial_state;

        // transition actions
        struct do_probe
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                (*(fsm.pp_ops_))->do_probe ();
              }
          }
        };

        struct do_configure
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                (*(fsm.pp_ops_))->do_configure ();
              }
          }
        };

        // guard conditions
        struct is_disabled_evt_required
        {
          template <class EVT, class FSM, class SourceState, class TargetState>
          bool operator()(EVT const & evt, FSM & fsm, SourceState & source, TargetState & target)
          {
            bool rc = false;
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                rc = (*(fsm.pp_ops_))->is_disabled_evt_required ();
              }
            TIZ_LOG (TIZ_PRIORITY_TRACE, " is_disabled_evt_required [%s]", rc ? "YES" : "NO");
            return rc;
          }
        };

        struct is_port_disabling_complete
        {
          template <class EVT, class FSM, class SourceState, class TargetState>
          bool operator()(EVT const & evt, FSM & fsm, SourceState & source, TargetState & target)
          {
            bool rc = false;
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                rc = (*(fsm.pp_ops_))->is_port_disabling_complete (evt.handle_, evt.port_);
              }
            TIZ_LOG (TIZ_PRIORITY_TRACE, " is_port_disabling_complete [%s]", rc ? "YES" : "NO");
            return rc;
          }
        };

        struct is_port_settings_evt_required
        {
          template <class EVT, class FSM, class SourceState, class TargetState>
          bool operator()(EVT const & evt, FSM & fsm, SourceState & source, TargetState & target)
          {
            bool rc = false;
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                rc = (*(fsm.pp_ops_))->is_port_settings_evt_required ();
              }
            TIZ_LOG (TIZ_PRIORITY_TRACE, " is_port_settings_evt_required [%s]", rc ? "YES" : "NO");
            return rc;
          }
        };

        // Transition table for configuring
        struct transition_table : boost::mpl::vector<
          //                       Start                       Event                      Next                         Action                    Guard
          //    +-----------------+----------------------------+--------------------------+----------------------------+-------------------------+----------------------------------------+
          boost::msm::front::Row < disabling_ports             , boost::msm::front::none  , awaiting_port_disabled_evt , boost::msm::front::none , is_disabled_evt_required               >,
          boost::msm::front::Row < disabling_ports             , boost::msm::front::none  , probing                    , do_probe                , boost::msm::front::euml::Not_<
                                                                                                                                                     is_disabled_evt_required >           >,
          boost::msm::front::Row < awaiting_port_disabled_evt  , omx_port_disabled_evt    , probing                    , do_probe                , is_port_disabling_complete             >,
          boost::msm::front::Row < probing                     , boost::msm::front::none  , awaiting_port_settings_evt , boost::msm::front::none , is_port_settings_evt_required          >,
          boost::msm::front::Row < probing                     , boost::msm::front::none  , conf_exit                  , do_configure            , boost::msm::front::euml::Not_<
                                                                                                                                                     is_port_settings_evt_required >      >,
          boost::msm::front::Row < awaiting_port_settings_evt  , omx_port_settings_evt    , conf_exit                  , do_configure                                                     >
          //    +-----------------+----------------------------+--------------------------+----------------------------+-------------------------+----------------------------------------+
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

      struct configured : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
      };

      struct config2idle : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        OMX_STATETYPE target_omx_state () const
        {
          return OMX_StateIdle;
        }
      };

      struct idle2exe : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        OMX_STATETYPE target_omx_state () const
        {
          return OMX_StateExecuting;
        }
      };

      struct executing : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        OMX_STATETYPE target_omx_state () const
        {
          return OMX_StateExecuting;
        }
      };

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
          assert (NULL != pp_ops);
        }

        // submachine states
        struct to_idle : public boost::msm::front::state<>
        {
          template <class Event,class FSM>
          void on_entry(Event const & evt, FSM & fsm)
          {
            G_FSM_LOG();
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
          void on_entry(Event const & evt, FSM & fsm)
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());
          }
        };

        // the initial state. Must be defined
        typedef to_idle initial_state;

        // transition actions
        struct do_skip
        {
          template <class FSM, class EVT, class SourceState, class TargetState>
          void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
          {
            if (fsm.pp_ops_ && *(fsm.pp_ops_))
              {
                (*(fsm.pp_ops_))->do_skip ();
              }
          }
        };

        // guard conditions

        // Transition table for skipping
        struct transition_table : boost::mpl::vector<
          //                       Start             Event            Next                   Action                                 Guard
          //    +-----------------+------------------+----------------+----------------------+--------------------------------------+---------------------------+
          boost::msm::front::Row < to_idle           , omx_trans_evt  , fsm_::idle2loaded    , fsm_::do_omx_idle2loaded       , fsm_::is_trans_complete        >,
          boost::msm::front::Row < fsm_::idle2loaded , omx_trans_evt  , skip_exit            , do_skip                        , fsm_::is_trans_complete        >
          //    +-----------------+------------------+----------------+----------------------+--------------------------------+---------------------------------+
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

      struct exe2pause : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        OMX_STATETYPE target_omx_state () const
        {
          return OMX_StatePause;
        }
      };

      struct pause : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        OMX_STATETYPE target_omx_state () const
        {
          return OMX_StatePause;
        }
      };

      struct pause2exe : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        OMX_STATETYPE target_omx_state () const
        {
          return OMX_StateExecuting;
        }
      };

      struct exe2idle : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        OMX_STATETYPE target_omx_state () const
        {
          return OMX_StateIdle;
        }
      };

      struct idle2loaded : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
        OMX_STATETYPE target_omx_state () const
        {
          return OMX_StateLoaded;
        }
      };

      // terminate state
      struct unloaded : public boost::msm::front::terminate_state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const & evt, FSM & fsm)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "ack unload");
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_ack_unloaded ();
            }
          TIZ_LOG (TIZ_PRIORITY_TRACE, "terminating");
          fsm.terminated_ = true;
        }
        template <class Event,class FSM>
        void on_exit(Event const & evt, FSM & fsm) {TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", typeid (*this).name ());}
      };

      // The initial state of the SM. Must be defined
      typedef inited initial_state;

      // transition actions

      struct do_load
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_load ();
            }
        }
      };

      struct do_setup
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_setup ();
            }
        }
      };

      struct do_ack_loaded
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_ack_loaded ();
            }
        }
      };

      struct do_store_config
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_store_config (evt.config_);
            }
        }
      };

      struct do_omx_loaded2idle
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const&, FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_omx_loaded2idle ();
            }
        }
      };

      struct do_omx_idle2exe
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_omx_idle2exe ();
            }
        }
      };

      struct do_ack_execd
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_ack_execd ();
            }
        }
      };

      struct do_seek
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_seek ();
            }
        }
      };

      struct do_volume
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_volume (evt.step_);
            }
        }
      };

      struct do_mute
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_mute ();
            }
        }
      };

      struct do_omx_exe2pause
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_omx_exe2pause ();
            }
        }
      };

      struct do_omx_pause2exe
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_omx_pause2exe ();
            }
        }
      };

      struct do_omx_exe2idle
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_omx_exe2idle ();
            }
        }
      };

      struct do_store_skip
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_store_skip (evt.jump_);
            }
        }
      };

      struct do_omx_idle2loaded
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_omx_idle2loaded ();
            }
        }
      };

      struct do_end_of_play
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_end_of_play ();
            }
        }
      };

      struct do_error
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_error ();
            }
        }
      };

      struct do_tear_down_tunnels
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_tear_down_tunnels ();
            }
        }
      };

      struct do_destroy_graph
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_destroy_graph ();
            }
        }
      };

      struct do_report_fatal_error
      {
        template <class FSM, class EVT, class SourceState, class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          G_FSM_LOG();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_report_fatal_error (evt.error_code_, evt.error_str_);
            }
        }
      };

      // guard conditions
      struct last_op_succeeded
      {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const & evt, FSM & fsm, SourceState & source, TargetState & target)
        {
          bool rc = false;
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              rc = (*(fsm.pp_ops_))->last_op_succeeded ();
            }
          TIZ_LOG (TIZ_PRIORITY_TRACE, "last_op_succeeded [%s]", rc ? "YES" : "NO");
          return rc;
        }
      };

      struct is_trans_complete
      {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const & evt ,FSM & fsm, SourceState & source, TargetState & target)
        {
          bool rc = false;
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              if ((*(fsm.pp_ops_))->is_trans_complete (evt.handle_, evt.state_))
                {
                  TIZ_LOG (TIZ_PRIORITY_NOTICE, "evt.state_ [%s] target state [%s]...",
                           tiz_state_to_str (evt.state_), tiz_state_to_str (source.target_omx_state ()));
                  assert (evt.state_ == source.target_omx_state ());
                  rc = true;
                }
            }
          TIZ_LOG (TIZ_PRIORITY_NOTICE, "is_trans_complete [%s]...", rc ? "YES" : "NO");
          return rc;
        }
      };

      struct is_last_eos
      {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const& evt, FSM & fsm, SourceState& , TargetState&)
        {
          bool rc = false;
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              rc = (*(fsm.pp_ops_))->is_last_component (evt.handle_);
            }
          TIZ_LOG (TIZ_PRIORITY_TRACE, "is_last_eos [%s]", rc ? "YES" : "NO");
          return rc;
        }
      };

      struct is_fatal_error
      {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const& evt, FSM & fsm, SourceState& , TargetState&)
        {
          bool rc = false;
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              rc = (*(fsm.pp_ops_))->last_op_succeeded ();
            }
          TIZ_LOG (TIZ_PRIORITY_TRACE, "is_fatal_error [%s]", rc ? "YES" : "NO");
          return rc;
        }
      };

      struct is_end_of_play
      {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const& evt, FSM & fsm, SourceState& , TargetState&)
        {
          bool rc = false;
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              rc = (*(fsm.pp_ops_))->is_end_of_play ();
            }
          TIZ_LOG (TIZ_PRIORITY_TRACE, "is_end_of_play [%s]", rc ? "YES" : "NO");
          return rc;
        }
      };

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
        boost::msm::front::Row < configuring
                                 ::exit_pt
                                 <configuring_
                                  ::conf_exit>, configured_evt , config2idle             , do_omx_loaded2idle                             >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < config2idle , omx_trans_evt   , idle2exe                , do_omx_idle2exe         , is_trans_complete    >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < idle2exe    , omx_trans_evt   , executing               , do_ack_execd            , is_trans_complete    >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < executing   , skip_evt        , skipping                , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_store_skip, do_omx_exe2idle > >         >,
        boost::msm::front::Row < executing   , seek_evt        , boost::msm::front::none , do_seek                                        >,
        boost::msm::front::Row < executing   , volume_evt      , boost::msm::front::none , do_volume                                      >,
        boost::msm::front::Row < executing   , mute_evt        , boost::msm::front::none , do_mute                                        >,
        boost::msm::front::Row < executing   , pause_evt       , exe2pause               , do_omx_exe2pause                               >,
        boost::msm::front::Row < executing   , unload_evt      , exe2idle                , do_omx_exe2idle                                >,
        boost::msm::front::Row < executing   , omx_err_evt     , skipping                , do_omx_exe2idle                                >,
        boost::msm::front::Row < executing   , omx_eos_evt     , skipping                , do_omx_exe2idle         , is_last_eos          >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
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
                                  ::skip_exit>, skipped_evt    , configuring             , boost::msm::front::none , boost::msm::front::euml::Not_<
                                                                                                                       is_end_of_play>   >,
        boost::msm::front::Row < skipping
                                 ::exit_pt
                                 <skipping_
                                  ::skip_exit>, skipped_evt    , unloaded                , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_error,
                                                                                               do_tear_down_tunnels,
                                                                                               do_destroy_graph> > , is_fatal_error       >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < exe2pause   , omx_trans_evt   , pause                   , boost::msm::front::none , is_trans_complete    >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < pause       , pause_evt       , pause2exe               , do_omx_pause2exe                               >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < pause2exe   , omx_trans_evt   , executing               , boost::msm::front::none , is_trans_complete    >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < exe2idle    , omx_trans_evt   , idle2loaded             , do_omx_idle2loaded      , is_trans_complete    >,
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
        boost::msm::front::Row < idle2loaded , omx_trans_evt   , unloaded                , boost::msm::front::ActionSequence_<
                                                                                             boost::mpl::vector<
                                                                                               do_tear_down_tunnels,
                                                                                               do_destroy_graph> > , is_trans_complete    >
        //    +------------------------------+-----------------+-------------------------+-------------------------+----------------------+
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
                                               "config2idle",
                                               "idle2exe",
                                               "executing",
                                               "skipping",
                                               "exe2pause",
                                               "pause",
                                               "pause2exe",
                                               "exe2idle",
                                               "idle2loaded",
                                               "unloaded"};
    static char const* const pstate(fsm const& p)
    {
      return tiz::graph::state_names[p.current_state()[0]];
    }

  } // namespace graph
} // namespace tiz

#endif // TIZGRAPHFSM_H

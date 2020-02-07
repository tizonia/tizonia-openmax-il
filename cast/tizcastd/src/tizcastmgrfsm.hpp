/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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

namespace tiz
{
  namespace cast
  {
    static char const* const state_names[] = { "starting",
                                               "started",
                                               "connecting",
                                               "connected",
                                               "running",
                                               "quitting",
                                               "quitted",
                                               "polling"};

    // main fsm events
    struct start_evt {};
    struct quit_evt {};
    struct connect_evt
    {
      connect_evt (const std::string &name_or_ip)
        : name_or_ip_ (name_or_ip)
      {
      }
      const std::string name_or_ip_;
    };
    struct disconnect_evt {};
    struct cast_status_evt {};
    struct load_url_evt
    {
      load_url_evt (const std::string &url,
                    const std::string &mime_type,
                    const std::string &title,
                    const std::string &album_art)
        : url_ (url),
          mime_type_ (mime_type),
          title_ (title),
          album_art_ (album_art)
      {
      }
      const std::string url_;
      const std::string mime_type_;
      const std::string title_;
      const std::string album_art_;
    };
    struct play_evt {};
    struct stop_evt {};
    struct pause_evt {};
    struct volume_evt
    {
      volume_evt (int volume)
        : volume_ (volume)
      {
      }
      const int volume_;
    };
    struct volume_up_evt {};
    struct volume_down_evt {};
    struct mute_evt {};
    struct unmute_evt {};
    struct poll_evt
    {
      poll_evt (int poll_time_ms)
        : poll_time_ms_ (poll_time_ms)
      {
      }
      const int poll_time_ms_;
    };
    struct err_evt
    {
      err_evt(const int error, const std::string & error_str, bool is_internal)
        :
        error_code_ (error),
        error_str_(error_str),
        is_internal_ (is_internal)
      {}
      int error_code_;
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
      struct quitting;
      struct quitted;
      struct polling;

      // data members
      ops ** pp_ops_;
      bool terminated_;

      fsm_(ops **pp_ops)
        :
        pp_ops_(pp_ops),
        terminated_ (false)
      {}

      // states
      struct starting : public boost::msm::front::state<>
      {
        // optional entry/exit methods
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) { GMGR_FSM_LOG (); }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) { GMGR_FSM_LOG (); }
      };

      struct started : public boost::msm::front::state<>
      {
        typedef boost::mpl::vector<cast_status_evt, load_url_evt, volume_evt> deferred_events;
        template <class Event,class FSM>
        void on_entry(Event const&, FSM& fsm) {GMGR_FSM_LOG ();}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      struct connecting : public boost::msm::front::state<>
      {
        typedef boost::mpl::vector<load_url_evt, volume_evt> deferred_events;
        template <class Event,class FSM>
        void on_entry(Event const&, FSM& fsm) {GMGR_FSM_LOG ();}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      struct connected : public boost::msm::front::state<>
      {
        typedef boost::mpl::vector<volume_evt> deferred_events;
        template <class Event,class FSM>
        void on_entry(Event const&, FSM& fsm) {GMGR_FSM_LOG ();}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      struct running : public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const&, FSM& fsm) {GMGR_FSM_LOG ();}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      struct quitting: public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) {GMGR_FSM_LOG ();}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
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

      // Orthogonal region's state
      struct polling: public boost::msm::front::state<>
      {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) {GMGR_FSM_LOG ();}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {GMGR_FSM_LOG ();}
      };

      // The initial state of the SM. Must be defined
      typedef boost::mpl::vector<starting, polling> initial_state;

      // transition actions
      struct do_connect
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_connect (evt.name_or_ip_);
            }
        }
      };

      struct do_disconnect
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_disconnect ();
            }
        }
      };

      struct do_poll
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt,FSM& fsm, SourceState& , TargetState&)
        {
          // GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_poll (evt.poll_time_ms_);
            }
        }
      };

      struct do_update_volume
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const&,FSM& fsm, SourceState& , TargetState&)
        {
          // GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_update_volume ();
            }
        }
      };

      struct do_load_url
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_load_url (evt.url_, evt.mime_type_, evt.title_, evt.album_art_);
            }
        }
      };

      struct do_play
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_play ();
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

      struct do_volume
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_volume (evt.volume_);
            }
        }
      };

      struct do_volume_up
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_volume_up ();
            }
        }
      };

      struct do_volume_down
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& , FSM& fsm, SourceState& , TargetState& )
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_volume_down ();
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

      struct do_unmute
      {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm, SourceState& , TargetState&)
        {
          GMGR_FSM_LOG ();
          if (fsm.pp_ops_ && *(fsm.pp_ops_))
            {
              (*(fsm.pp_ops_))->do_unmute ();
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

      // Transition table for the cast mgr fsm
      struct transition_table : boost::mpl::vector<
        //         Start                 Event              Next          Action                   Guard
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < starting              , start_evt        , started     , bmf::none                                   >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < started               , connect_evt      , connecting  , do_connect                                  >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < connecting            , cast_status_evt  , connected   , bmf::none                                   >,
        bmf::Row < connecting            , poll_evt         , bmf::none   , do_poll                                     >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < connected             , load_url_evt     , running     , do_load_url                                 >,
        bmf::Row < connected             , poll_evt         , bmf::none   , do_poll                                     >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < running               , load_url_evt     , bmf::none   , do_load_url                                 >,
        bmf::Row < running               , play_evt         , bmf::none   , do_play                                     >,
        bmf::Row < running               , stop_evt         , bmf::none   , do_stop                                     >,
        bmf::Row < running               , pause_evt        , bmf::none   , do_pause                                    >,
        bmf::Row < running               , volume_evt       , bmf::none   , do_volume                                   >,
        bmf::Row < running               , volume_up_evt    , bmf::none   , do_volume_up                                >,
        bmf::Row < running               , volume_down_evt  , bmf::none   , do_volume_down                              >,
        bmf::Row < running               , mute_evt         , bmf::none   , do_mute                                     >,
        bmf::Row < running               , unmute_evt       , bmf::none   , do_unmute                                   >,
        bmf::Row < running               , disconnect_evt   , quitting    , do_disconnect                               >,
        bmf::Row < running               , quit_evt         , quitting    , do_disconnect                               >,
        bmf::Row < running               , poll_evt         , bmf::none   , do_poll                                     >,
        bmf::Row < running               , err_evt          , bmf::none   , bmf::none              , bmf::euml::Not_<
                                                                                                        is_fatal_error> >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < quitting              , bmf::none        , quitted     , bmf::none                                   >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < quitted               , connect_evt      , connecting  , do_connect                                  >,
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        bmf::Row < polling               , err_evt          , quitted     , do_report_fatal_error  , is_fatal_error     >,
        bmf::Row < polling               , poll_evt         , bmf::none   , do_poll                                     >
        //    +----+---------------------+------------------+-------------+------------------------+--------------------+
        > {};

      // Replaces the default no-transition response.
      template <class FSM,class Event>
      void no_transition(Event const& e, FSM&,int state)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "no transition from state [%s] on event [%s]",
                 tiz::cast::state_names[state], typeid(e).name());
      }
    };
    // typedef boost::msm::back::state_machine<fsm_, boost::msm::back::mpl_cast_fsm_check> fsm;
    typedef boost::msm::back::state_machine<fsm_> fsm;

    // Helper function to aid in printing the current state when debugging
    char const* const pstate(fsm const& p);

  } // namespace cast
} // namespace tiz

#endif // TIZCASTMGRFSM_HPP

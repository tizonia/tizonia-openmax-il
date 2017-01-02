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
 * @file   tizgraphguard.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph fsm guard conditions
 *
 */

#ifndef TIZGRAPHGUARD_HPP
#define TIZGRAPHGUARD_HPP

#include <OMX_Index.h>

#include <tizplatform.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.guard"
#endif

#define G_GUARD_LOG(bool_result)                                              \
  do                                                                          \
  {                                                                           \
    TIZ_LOG (TIZ_PRIORITY_TRACE, "GUARD [%s] -> [%s]", typeid(*this).name (), \
             bool_result ? "YES" : "NO");                                     \
  } while (0)

namespace tiz
{
  namespace graph
  {

    // guard conditions
    struct is_port_disabling_complete
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState& source,
                      TargetState& target)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))
                   ->is_port_disabling_complete (evt.handle_, evt.port_);
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_port_enabling_complete
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState& source,
                      TargetState& target)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))
                   ->is_port_enabling_complete (evt.handle_, evt.port_);
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_disabled_evt_required
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState& source,
                      TargetState& target)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))->is_disabled_evt_required ();
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_port_settings_evt_required
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState& source,
                      TargetState& target)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))->is_port_settings_evt_required ();
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct last_op_succeeded
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState& source,
                      TargetState& target)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))->last_op_succeeded ();
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_trans_complete
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState& source,
                      TargetState& target)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          if ((*(fsm.pp_ops_))->is_trans_complete (evt.handle_, evt.state_))
          {
            TIZ_LOG (TIZ_PRIORITY_NOTICE,
                     "evt.state_ [%s] target state [%s]...",
                     tiz_state_to_str (evt.state_),
                     tiz_state_to_str (source.target_omx_state ()));
            assert (evt.state_ == source.target_omx_state ());
            rc = true;
          }
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    template<int handle_id, OMX_STATETYPE state_id>
    struct is_component_state
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState& source,
                      TargetState& target)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))->is_component_state (handle_id, state_id);
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    template<OMX_STATETYPE state_id>
    struct is_destination_state
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState& source,
                      TargetState& target)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))->is_destination_state (state_id);
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_last_eos
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))->is_last_component (evt.handle_);
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_first_eos
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))->is_first_component (evt.handle_);
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_internal_error
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          // If the last operation didn't succeed, that will be fatal.
          rc = !(*(fsm.pp_ops_))->last_op_succeeded ();
        }

        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_fatal_error
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))->is_fatal_error (evt.error_);
        }

        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_end_of_play
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (*(fsm.pp_ops_))->is_end_of_play ();
        }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    struct is_probing_result_ok
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            rc = (*(fsm.pp_ops_))->is_probing_result_ok ();
          }
        G_GUARD_LOG (rc);
        return rc;
      }
    };

    template<int tunnel_id>
    struct is_tunnel_altered
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            rc = (*(fsm.pp_ops_))->is_tunnel_altered (tunnel_id, evt.handle_, evt.port_, evt.index_);
          }

        G_GUARD_LOG (rc);
        return rc;
      }
    };

    template<OMX_INDEXTYPE param_or_config_idx>
    struct is_setting_changed
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (param_or_config_idx == evt.index_);
        }

        G_GUARD_LOG (rc);
        return rc;
      }
    };

    template<OMX_ERRORTYPE error_id>
    struct is_error
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (error_id == evt.error_);
        }

        G_GUARD_LOG (rc);
        return rc;
      }
    };

    template<int expected_tunnel_id, int tunnel_id>
    struct is_tunnel_id
    {
      template < class EVT, class FSM, class SourceState, class TargetState >
      bool operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        bool rc = false;
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          rc = (expected_tunnel_id == tunnel_id);
        }

        G_GUARD_LOG (rc);
        return rc;
      }
    };

  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHGUARD_HPP

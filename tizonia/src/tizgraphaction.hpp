/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizgraphaction.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph fsm actions
 *
 */

#ifndef TIZGRAPHACTION_HPP
#define TIZGRAPHACTION_HPP

#include <OMX_Core.h>
#include <tizplatform.h>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.action"
#endif

#define G_ACTION_LOG()                                                  \
  do                                                                    \
  {                                                                     \
    TIZ_LOG (TIZ_PRIORITY_TRACE, "ACTION [%s]", typeid(*this).name ()); \
  } while (0)

namespace tiz
{
  namespace graph
  {

    struct do_probe
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_probe ();
        }
      }
    };

    struct do_configure
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_configure ();
        }
      }
    };

    struct do_configure_source
    {
      template <class FSM, class EVT, class SourceState, class TargetState>
      void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
      {
        G_ACTION_LOG();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            (*(fsm.pp_ops_))->do_configure_source ();
          }
      }
    };

    struct do_skip
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_skip ();
        }
      }
    };

    struct do_load
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_load ();
        }
      }
    };

    struct do_load_source
    {
      template <class FSM, class EVT, class SourceState, class TargetState>
      void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
      {
        G_ACTION_LOG();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            (*(fsm.pp_ops_))->do_load_source ();
          }
      }
    };

    struct do_setup
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_setup ();
        }
      }
    };

    struct do_ack_loaded
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_ack_loaded ();
        }
      }
    };

    struct do_store_config
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_store_config (evt.config_);
        }
      }
    };

    template<int handle_id, int port_id>
    struct do_enable_auto_detection
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_enable_auto_detection (handle_id, port_id);
        }
      }
    };

    struct do_omx_loaded2idle
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_omx_loaded2idle ();
        }
      }
    };

    struct do_source_omx_loaded2idle
    {
      template <class FSM, class EVT, class SourceState, class TargetState>
      void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_source_omx_loaded2idle ();
        }
      }
    };

    struct do_source_omx_idle2exe
    {
      template <class FSM, class EVT, class SourceState, class TargetState>
      void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_source_omx_idle2exe ();
        }
      }
    };

    struct do_omx_idle2exe
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_omx_idle2exe ();
        }
      }
    };

    struct do_ack_execd
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_ack_execd ();
        }
      }
    };

    struct do_ack_stopped
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_ack_stopped ();
        }
      }
    };

    struct do_ack_paused
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_ack_paused ();
        }
      }
    };

    struct do_ack_unpaused
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_ack_unpaused ();
        }
      }
    };

    struct do_seek
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_seek ();
        }
      }
    };

    struct do_volume
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_volume (evt.step_);
        }
      }
    };

    struct do_mute
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_mute ();
        }
      }
    };

    struct do_omx_exe2pause
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_omx_exe2pause ();
        }
      }
    };

    struct do_omx_pause2exe
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_omx_pause2exe ();
        }
      }
    };

    struct do_omx_pause2idle
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_omx_pause2idle ();
        }
      }
    };

    struct do_omx_exe2idle
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_omx_exe2idle ();
        }
      }
    };

    struct do_store_skip
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_store_skip (evt.jump_);
        }
      }
    };

    struct do_omx_idle2loaded
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_omx_idle2loaded ();
        }
      }
    };

    template<int tunnel_id>
    struct do_disable_tunnel
    {
      template <class FSM, class EVT, class SourceState, class TargetState>
      void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_disable_tunnel (tunnel_id);
        }
      }
    };

    template<int tunnel_id>
    struct do_enable_tunnel
    {
      template <class FSM, class EVT, class SourceState, class TargetState>
      void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
      {
        G_ACTION_LOG();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            (*(fsm.pp_ops_))->do_enable_tunnel (tunnel_id);
          }
      }
    };

    struct do_reconfigure_tunnel
    {
      template <class FSM, class EVT, class SourceState, class TargetState>
      void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
      {
        G_ACTION_LOG();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            (*(fsm.pp_ops_))->do_reconfigure_tunnel ();
          }
      }
    };

    struct do_end_of_play
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_end_of_play ();
        }
      }
    };

    struct do_error
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_error ();
        }
      }
    };

    struct do_tear_down_tunnels
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_tear_down_tunnels ();
        }
      }
    };

    struct do_destroy_graph
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const&, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_destroy_graph ();
        }
      }
    };

    struct do_record_fatal_error
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))
            ->do_record_fatal_error (evt.handle_, evt.error_, evt.port_);
        }
      }
    };

    struct do_reset_internal_error
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))->do_reset_internal_error ();
        }
      }
    };

    template<OMX_STATETYPE state_id>
    struct do_record_destination
    {
      template <class FSM, class EVT, class SourceState, class TargetState>
      void operator()(EVT const& evt, FSM& fsm, SourceState& , TargetState& )
      {
        G_ACTION_LOG();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
          {
            (*(fsm.pp_ops_))->do_record_destination (state_id);
          }
      }
    };

  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHACTION_HPP

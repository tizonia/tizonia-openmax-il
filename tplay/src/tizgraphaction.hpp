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
 * @file   tizgraphaction.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph fsm actions
 *
 */

#ifndef TIZGRAPHACTION_H
#define TIZGRAPHACTION_H

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

    struct do_report_fatal_error
    {
      template < class FSM, class EVT, class SourceState, class TargetState >
      void operator()(EVT const& evt, FSM& fsm, SourceState&, TargetState&)
      {
        G_ACTION_LOG ();
        if (fsm.pp_ops_ && *(fsm.pp_ops_))
        {
          (*(fsm.pp_ops_))
              ->do_report_fatal_error (evt.error_code_, evt.error_str_);
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

  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHACTION_H

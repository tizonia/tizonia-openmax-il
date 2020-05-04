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
 * @file   tizgraphevt.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph fsm events
 *
 */

#ifndef TIZGRAPHEVT_HPP
#define TIZGRAPHEVT_HPP

#include <OMX_Component.h>
#include <OMX_Core.h>

#include "tizgraphtypes.hpp"

namespace tiz
{
  namespace graph
  {

    // Main fsm events.
    struct load_evt
    {
      load_evt (const tizgraphconfig_ptr_t config = tizgraphconfig_ptr_t ())
        : config_ (config)
      {
      }
      const tizgraphconfig_ptr_t config_;
    };

    struct execute_evt
    {
      execute_evt (const tizgraphconfig_ptr_t config = tizgraphconfig_ptr_t ())
        : config_ (config)
      {
      }
      const tizgraphconfig_ptr_t config_;
    };

    // Make this state convertible from any state (this event exits a
    // sub-machine)
    struct configured_evt
    {
      configured_evt ()
      {
      }
      template < class Event >
      configured_evt (Event const &)
      {
      }
    };

    struct omx_trans_evt
    {
      omx_trans_evt (const OMX_HANDLETYPE a_handle, const OMX_STATETYPE a_state,
                     const OMX_ERRORTYPE a_error)
        : handle_ (a_handle), state_ (a_state), error_ (a_error)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_STATETYPE state_;
      OMX_ERRORTYPE error_;
    };

    struct position_evt
    {
      position_evt (const int a_pos) : pos_ (a_pos)
      {
      }
      int pos_;
    };

    struct skip_evt
    {
      skip_evt (const int a_jump) : jump_ (a_jump)
      {
      }
      int jump_;
    };

    // Make this state convertible from any state (this event exits a
    // sub-machine)
    struct skipped_evt
    {
      skipped_evt ()
      {
      }
      template < class Event >
      skipped_evt (Event const &)
      {
      }
    };

    struct seek_evt
    {
    };

    struct volume_step_evt
    {
      volume_step_evt (const int step) : step_ (step)
      {
      }
      const int step_;
    };

    struct volume_evt
    {
      volume_evt (const double vol) : vol_ (vol)
      {
      }
      const double vol_;
    };

    struct mute_evt
    {
    };

    struct pause_evt
    {
    };

    struct omx_evt
    {
      omx_evt (const OMX_HANDLETYPE a_handle, const OMX_EVENTTYPE a_event,
               const OMX_U32 a_data1, const OMX_U32 a_data2,
               const OMX_PTR ap_eventdata)
        : handle_ (a_handle),
          event_ (a_event),
          data1_ (a_data1),
          data2_ (a_data2),
          p_eventdata_ (ap_eventdata)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_EVENTTYPE event_;
      OMX_U32 data1_;
      OMX_U32 data2_;
      OMX_PTR p_eventdata_;
    };

    struct omx_eos_evt
    {
      omx_eos_evt (const OMX_HANDLETYPE a_handle, const OMX_U32 port,
                   const OMX_U32 flags)
        : handle_ (a_handle), port_ (port), flags_ (flags)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_U32 port_;
      OMX_U32 flags_;
    };

    struct stop_evt
    {
    };

    struct unload_evt
    {
    };

    struct omx_port_disabled_evt
    {
      omx_port_disabled_evt (const OMX_HANDLETYPE a_handle, const OMX_U32 port,
                             const OMX_ERRORTYPE error)
        : handle_ (a_handle), port_ (port), error_ (error)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_U32 port_;
      OMX_ERRORTYPE error_;
    };

    struct omx_port_enabled_evt
    {
      omx_port_enabled_evt (const OMX_HANDLETYPE a_handle, const OMX_U32 port,
                            const OMX_ERRORTYPE error)
        : handle_ (a_handle), port_ (port), error_ (error)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_U32 port_;
      OMX_ERRORTYPE error_;
    };

    struct omx_port_flushed_evt
    {
      omx_port_flushed_evt (const OMX_HANDLETYPE a_handle, const OMX_U32 port,
                            const OMX_ERRORTYPE error)
        : handle_ (a_handle), port_ (port), error_ (error)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_U32 port_;
      OMX_ERRORTYPE error_;
    };

    struct omx_port_settings_evt
    {
      omx_port_settings_evt (const OMX_HANDLETYPE a_handle, const OMX_U32 port,
                             const OMX_INDEXTYPE index)
        : handle_ (a_handle), port_ (port), index_ (index)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_U32 port_;
      OMX_INDEXTYPE index_;
    };

    struct omx_index_setting_evt
    {
      omx_index_setting_evt (const OMX_HANDLETYPE a_handle, const OMX_U32 port,
                             const OMX_INDEXTYPE index)
        : handle_ (a_handle), port_ (port), index_ (index)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_U32 port_;
      OMX_INDEXTYPE index_;
    };

    struct omx_format_detected_evt
    {
      omx_format_detected_evt (const OMX_HANDLETYPE a_handle)
        : handle_ (a_handle)
      {
      }
      OMX_HANDLETYPE handle_;
    };

    // NOTE: This is an error produced by an omx component
    struct omx_err_evt
    {
      omx_err_evt (const OMX_HANDLETYPE handle, const OMX_ERRORTYPE error,
                   const OMX_U32 port = OMX_ALL, // OMX_ALL here means "no port id"
                   const OMX_PTR p_eventdata = NULL)
          : handle_ (handle),
            error_ (error),
            port_ (port),
            p_eventdata_ (p_eventdata)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_ERRORTYPE error_;
      OMX_U32 port_;
      const OMX_PTR p_eventdata_;
    };

    // NOTE: This is an internal error (some internal operation failed)
    struct err_evt
    {
      err_evt (const OMX_ERRORTYPE error, const std::string &error_str)
        : error_code_ (error), error_str_ (error_str)
      {
      }
      OMX_ERRORTYPE error_code_;
      std::string error_str_;
    };

    // Make this state convertible from any state (this event exits a
    // sub-machine)
    struct auto_detected_evt
    {
      auto_detected_evt ()
      {
      }
      template < class Event >
      auto_detected_evt (Event const &)
      {
      }
    };

    // Make this state convertible from any state (this event exits a
    // sub-machine)
    struct graph_updated_evt
    {
      graph_updated_evt ()
      {
      }
      template < class Event >
      graph_updated_evt (Event const &)
      {
      }
    };

    // Make this state convertible from any state (this event exits a
    // sub-machine)
    struct graph_reconfigured_evt
    {
      graph_reconfigured_evt ()
      {
      }
      template < class Event >
      graph_reconfigured_evt (Event const &)
      {
      }
    };

    // Make this state convertible from any state (this event exits a
    // sub-machine)
    struct tunnel_reconfigured_evt
    {
      tunnel_reconfigured_evt ()
      {
      }
      template < class Event >
      tunnel_reconfigured_evt (Event const &)
      {
      }
    };

    struct timer_evt
    {
      timer_evt (void *ap_arg1, const unsigned int a_id)
        : p_arg1_ (ap_arg1), id_ (a_id)
      {
      }
      void * p_arg1_;
      unsigned int id_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHEVT_HPP

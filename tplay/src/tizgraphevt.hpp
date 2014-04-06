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
 * @file   tizgraphevt.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Graph fsm events
 *
 */

#ifndef TIZGRAPHEVT_HPP
#define TIZGRAPHEVT_HPP

#include <OMX_Core.h>
#include <OMX_Component.h>

#include "tizgraphtypes.hpp"

namespace tiz
{
  namespace graph
  {

    // Main fsm events.
    struct load_evt
    {
    };
    struct execute_evt
    {
      execute_evt (const tizgraphconfig_ptr_t config) : config_ (config)
      {
      }
      const tizgraphconfig_ptr_t config_;
    };
    // Make this state convertible from any state (this exists a sub-machine)
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
    struct skip_evt
    {
      skip_evt (const int a_jump) : jump_ (a_jump)
      {
      }
      int jump_;
    };
    // Make this state convertible from any state (this exists a sub-machine)
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
    struct volume_evt
    {
      volume_evt (const int step) : step_ (step)
      {
      }
      int step_;
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
    // NOTE: This is an error produced by an omx component
    struct omx_err_evt
    {
      omx_err_evt (const OMX_HANDLETYPE a_handle, const OMX_ERRORTYPE a_error,
                   const OMX_U32 port
                   = OMX_ALL)  // OMX_ALL here means "no port id"
          : handle_ (a_handle),
            error_ (a_error),
            port_ (port)
      {
      }
      OMX_HANDLETYPE handle_;
      OMX_ERRORTYPE error_;
      OMX_U32 port_;
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

  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHEVT_HPP

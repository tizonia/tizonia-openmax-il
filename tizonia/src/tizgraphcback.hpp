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
 * @file   tizgraphcback.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL callback handler
 *
 *
 */

#ifndef TIZGRAPHCBACK_HPP
#define TIZGRAPHCBACK_HPP

#include <string>

#include <OMX_Core.h>
#include <OMX_Component.h>

namespace tiz
{
  namespace graph
  {
    // Forward declaration
    class graph;

    struct omx_event_info
    {
      // general purpose constructor
      omx_event_info (OMX_HANDLETYPE component, OMX_EVENTTYPE event,
                      OMX_U32 ndata1, OMX_U32 ndata2, OMX_PTR pEventData);

      // state transition constructor
      omx_event_info (OMX_HANDLETYPE component, OMX_STATETYPE state,
                      OMX_ERRORTYPE error);

      // port transition constructor
      omx_event_info (OMX_HANDLETYPE component, OMX_U32 port_id,
                      OMX_COMMANDTYPE disable_or_enable, OMX_ERRORTYPE error);

      bool operator==(const omx_event_info &b);
      std::string to_string () const;

      OMX_HANDLETYPE component_;
      OMX_EVENTTYPE event_;
      OMX_U32 ndata1_;
      OMX_U32 ndata2_;
      OMX_PTR pEventData_;
    };

    class cbackhandler
    {

    public:
      explicit cbackhandler (graph *p_graph);

    public:
      static OMX_ERRORTYPE event_handler_wrapper (
          OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent,
          OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);

      OMX_CALLBACKTYPE *get_omx_cbacks ();

    private:
      void event_handler (OMX_HANDLETYPE component, OMX_EVENTTYPE event,
                          OMX_U32 ndata1, OMX_U32 ndata2, OMX_PTR pEventData);

    private:
      const graph *p_graph_;
      OMX_CALLBACKTYPE cbacks_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZGRAPHCBACK_HPP

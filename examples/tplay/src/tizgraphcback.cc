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
 * @file   tizgraphcback.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL callback handler implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tizosal.h>
#include <tizmacros.h>

#include <tizgraph.h>

#include <boost/lexical_cast.hpp>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.cbackhandler"
#endif

namespace graph = tiz::graph;

graph::omx_event_info::omx_event_info (OMX_HANDLETYPE component,
                                       OMX_EVENTTYPE event, OMX_U32 ndata1,
                                       OMX_U32 ndata2, OMX_PTR pEventData)
  : component_ (component),
    event_ (event),
    ndata1_ (ndata1),
    ndata2_ (ndata2),
    pEventData_ (pEventData)
{
}

graph::omx_event_info::omx_event_info (OMX_HANDLETYPE component,
                                       OMX_STATETYPE state, OMX_ERRORTYPE error)
  : component_ (component),
    event_ (OMX_EventCmdComplete),
    ndata1_ (OMX_CommandStateSet),
    ndata2_ (state),
    pEventData_ (NULL)
{
  pEventData_ = (OMX_PTR)error;
}

// port transition constructor
graph::omx_event_info::omx_event_info (OMX_HANDLETYPE component,
                                       OMX_U32 port_id,
                                       OMX_COMMANDTYPE disable_or_enable,
                                       OMX_ERRORTYPE error)
  : component_ (component),
    event_ (OMX_EventCmdComplete),
    ndata1_ (disable_or_enable),
    ndata2_ (port_id),
    pEventData_ (NULL)
{
  pEventData_ = (OMX_PTR)error;
}

bool graph::omx_event_info::operator==(const omx_event_info &b)
{
  if (component_ == b.component_ && event_ == b.event_ && ndata1_ == b.ndata1_
      && ndata2_ == b.ndata2_)
  // TODO: Ignore pEventData for now. This is to make events like this pass the
  // comparison:
  // e.g.: [tizgraph.cc:receive_event:238] --- [OMX.Aratelia.file_reader.binary]
  // : [OMX_EventCmdComplete] [OMX_CommandStateSet] [OMX_StateLoaded] error
  // [0x80001017]
  //       && pEventData_ == b.pEventData_)
  {
    return true;
  }
  return false;
}

std::string graph::omx_event_info::to_string () const
{
  std::string info ("[");
  info.append (tiz_evt_to_str (static_cast<OMX_EVENTTYPE>(event_)));
  info.append ("]");
  switch (event_)
  {
    case OMX_EventCmdComplete:
    {
      info.append (" [");
      info.append (tiz_cmd_to_str (static_cast<OMX_COMMANDTYPE>(ndata1_)));
      info.append ("]");
      if (OMX_CommandStateSet == ndata1_)
      {
        info.append (" [");
        info.append (tiz_state_to_str (static_cast<OMX_STATETYPE>(ndata2_)));
        info.append ("]");
      }
      else
      {
        info.append (" PORT [");
        info.append (boost::lexical_cast<std::string>(ndata2_));
        info.append ("]");
      }
    }
    break;

    case OMX_EventError:
    {
      info.append (" [");
      info.append (tiz_err_to_str (static_cast<OMX_ERRORTYPE>(ndata1_)));
      info.append ("]");
      info.append (" [");
      info.append (boost::lexical_cast<std::string>(ndata2_));
      info.append ("]");
    }
    break;

    case OMX_EventPortSettingsChanged:
    {
      info.append (" PORT [");
      info.append (boost::lexical_cast<std::string>(ndata1_));
      info.append ("]");
      info.append (" [");
      info.append (tiz_idx_to_str (static_cast<OMX_INDEXTYPE>(ndata2_)));
      info.append ("]");
    }
    break;

    case OMX_EventBufferFlag:
    {
      info.append (" PORT [");
      info.append (boost::lexical_cast<std::string>(ndata1_));
      info.append ("]");
      info.append (" nFlags [");
      info.append (boost::lexical_cast<std::string>(ndata2_));
      info.append ("]");
    }
    break;

    default:
    {
      info.append (" [TO BE DONE]");
    }
    break;
  };
  return info;
}

//
// cbackhandler
//
graph::cbackhandler::cbackhandler (graph *p_graph)
  : p_graph_ (p_graph), cbacks_ ()
{
  assert (NULL != p_graph);
  cbacks_.EventHandler = &cbackhandler::event_handler_wrapper;
  cbacks_.EmptyBufferDone = NULL;
  cbacks_.FillBufferDone = NULL;
}

OMX_ERRORTYPE
graph::cbackhandler::event_handler_wrapper (OMX_HANDLETYPE hComponent,
                                            OMX_PTR pAppData,
                                            OMX_EVENTTYPE eEvent,
                                            OMX_U32 nData1, OMX_U32 nData2,
                                            OMX_PTR pEventData)
{
  cbackhandler *p_handler = static_cast<cbackhandler *>(pAppData);
  assert (NULL != p_handler);
  p_handler->event_handler (hComponent, eEvent, nData1, nData2, pEventData);
  return OMX_ErrorNone;
}

OMX_CALLBACKTYPE *graph::cbackhandler::get_omx_cbacks ()
{
  return &cbacks_;
}

void graph::cbackhandler::event_handler (OMX_HANDLETYPE hComponent,
                                         OMX_EVENTTYPE eEvent, OMX_U32 nData1,
                                         OMX_U32 nData2, OMX_PTR pEventData)
{
  const_cast<graph *>(p_graph_)->omx_evt (
      omx_event_info (hComponent, eEvent, nData1, nData2, pEventData));
}

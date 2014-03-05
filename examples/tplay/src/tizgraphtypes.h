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
 * @file   tizgraphtypes.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Various types used throughout the application
 *
 *
 */

#ifndef TIZGRAPHTYPES_H
#define TIZGRAPHTYPES_H

#include <set>
#include <vector>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#include <OMX_Core.h>

typedef std::map < OMX_HANDLETYPE, std::string > omx_hdl2name_map_t;
typedef std::vector < std::string > omx_comp_name_lst_t;
typedef std::vector < OMX_HANDLETYPE > omx_comp_handle_lst_t;
typedef std::vector < std::string > omx_comp_role_lst_t;
typedef std::vector < OMX_EVENTTYPE > omx_event_lst_t;
typedef std::vector < OMX_STATETYPE > omx_state_lst_t;
typedef std::vector < std::string > uri_lst_t;
typedef std::set < std::string > file_extension_lst_t;

namespace tiz
{
  class probe;
  namespace graph
  {
    class graph;
    class config;
    class srvconfig;
    struct omx_event_info;
  }
}
typedef boost::shared_ptr<tiz::probe> tizprobe_ptr_t;
typedef std::vector<tiz::graph::omx_event_info> omx_event_info_lst_t;
typedef boost::shared_ptr<tiz::graph::graph> tizgraph_ptr_t;
typedef std::map<std::string, tizgraph_ptr_t> tizgraph_ptr_map_t;
typedef boost::shared_ptr<tiz::graph::config> tizgraphconfig_ptr_t;
typedef boost::shared_ptr<tiz::graph::srvconfig> tizstreamsrvconfig_ptr_t;

#endif // TIZGRAPHTYPES_H

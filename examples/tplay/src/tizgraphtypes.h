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
 * @brief  OpenMAX IL graph configuration base class
 *
 *
 */

#ifndef TIZGRAPHTYPES_H
#define TIZGRAPHTYPES_H

#include <OMX_Core.h>
#include <vector>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

typedef std::vector < std::string > component_names_t;
typedef std::vector < OMX_HANDLETYPE > component_handles_t;
typedef std::vector < std::string > component_roles_t;
typedef std::vector < OMX_EVENTTYPE > component_events_t;
typedef std::map < OMX_HANDLETYPE, std::string > handle_to_name_t;
typedef std::vector < std::string > uri_list_t;
typedef std::vector < std::string > extension_list_t;

class tizgraph;
typedef boost::shared_ptr<tizgraph> tizgraph_ptr_t;
typedef std::map<std::string, tizgraph_ptr_t> tizgraph_ptr_map_t;

class tizgraphmgr;
typedef class tizgraphmgr tizgraphmgr_t;
typedef boost::shared_ptr<tizgraphmgr> tizgraphmgr_ptr_t;

class tizgraphconfig;
typedef boost::shared_ptr<tizgraphconfig> tizgraphconfig_ptr_t;

class tizstreamsrvconfig;
typedef boost::shared_ptr<tizstreamsrvconfig> tizstreamsrvconfig_ptr_t;


#endif // TIZGRAPHTYPES_H

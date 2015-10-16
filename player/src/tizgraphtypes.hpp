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

/**```````````````````````````````````
 * @file   tizgraphtypes.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Various types used throughout the application
 *
 *
 */

#ifndef TIZGRAPHTYPES_HPP
#define TIZGRAPHTYPES_HPP

#include <set>
#include <vector>
#include <deque>
#include <string>
#include <map>

#include <boost/shared_ptr.hpp>

#include <OMX_Core.h>

typedef std::map< OMX_HANDLETYPE, std::string > omx_hdl2name_map_t;
typedef std::vector< std::string > omx_comp_name_lst_t;
typedef std::vector< OMX_HANDLETYPE > omx_comp_handle_lst_t;
typedef std::vector< std::string > omx_comp_role_lst_t;
typedef std::vector< int > omx_comp_role_pos_lst_t;
typedef std::vector< OMX_EVENTTYPE > omx_event_lst_t;
typedef std::vector< OMX_STATETYPE > omx_state_lst_t;
typedef std::vector< std::string > uri_lst_t;
typedef std::set< std::string > file_extension_lst_t;
typedef std::map< std::string, std::string > track_metadata_map_t;

namespace tiz
{
  class probe;
  class playlist;
  namespace graph
  {
    class graph;
    class config;
    class httpservconfig;
    class spotifyconfig;
    class gmusicconfig;
    class scloudconfig;
    struct omx_event_info;
  }
}
typedef boost::shared_ptr< tiz::probe > tizprobe_ptr_t;
typedef std::vector< tiz::graph::omx_event_info > omx_event_info_lst_t;
typedef boost::shared_ptr< tiz::graph::graph > tizgraph_ptr_t;
typedef std::map< std::string, tizgraph_ptr_t > tizgraph_ptr_map_t;
typedef boost::shared_ptr< tiz::graph::config > tizgraphconfig_ptr_t;
typedef boost::shared_ptr< tiz::graph::httpservconfig > tizhttpservconfig_ptr_t;
typedef boost::shared_ptr< tiz::graph::spotifyconfig > tizspotifyconfig_ptr_t;
typedef boost::shared_ptr< tiz::graph::gmusicconfig > tizgmusicconfig_ptr_t;
typedef boost::shared_ptr< tiz::graph::scloudconfig > tizscloudconfig_ptr_t;
typedef tiz::playlist tizplaylist_t;
typedef boost::shared_ptr< tiz::playlist > tizplaylist_ptr_t;

#endif  // TIZGRAPHTYPES_HPP
